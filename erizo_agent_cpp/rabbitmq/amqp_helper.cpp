#include "amqp_helper.h"

#include <unistd.h>

#include "common/config.h"

DEFINE_LOGGER(AMQPHelper, "AMQPHelper");
static Config *config = Config::getInstance();

AMQPHelper::AMQPHelper() : init_(false),
                           run_(false),
                           conn_(nullptr),
                           recv_thread_(nullptr),
                           send_thread_(nullptr)
{
}

AMQPHelper::~AMQPHelper() {}

int AMQPHelper::callback(const std::string &exchange, const std::string &queuename, const std::string &binding_key, const std::string &send_msg)
{
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;
    props.correlation_id = amqp_cstring_bytes("1");
    props.reply_to = amqp_bytes_malloc_dup(amqp_cstring_bytes(queuename.c_str()));
    if (props.reply_to.bytes == NULL)
    {
        ELOG_ERROR("Out of memory while copying queue name");
        return 1;
    }

    amqp_basic_publish(conn_, 1, amqp_cstring_bytes(exchange.c_str()),
                       amqp_cstring_bytes(binding_key.c_str()), 0, 0,
                       &props, amqp_cstring_bytes(send_msg.c_str()));
    amqp_bytes_free(props.reply_to);
    return 0;
}

void AMQPHelper::addCallback(const std::string &queuename, const std::string &binding_key, const std::string &send_msg)
{
    std::unique_lock<std::mutex> lock(mux_);
    queue_.push({Config::getInstance()->uniquecast_exchange_, queuename, binding_key, send_msg});
    cond_.notify_one();
}

int AMQPHelper::checkError(amqp_rpc_reply_t x)
{
    switch (x.reply_type)
    {
    case AMQP_RESPONSE_NORMAL:
        return 0;

    case AMQP_RESPONSE_NONE:
        ELOG_ERROR("missing RPC reply type!");
        break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        ELOG_ERROR("%s", amqp_error_string2(x.library_error));
        break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id)
        {
        case AMQP_CONNECTION_CLOSE_METHOD:
        {
            amqp_connection_close_t *m =
                (amqp_connection_close_t *)x.reply.decoded;
            ELOG_ERROR("server connection error %uh, message: %.*s",
                       m->reply_code, (int)m->reply_text.len,
                       (char *)m->reply_text.bytes);
            break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD:
        {
            amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
            ELOG_ERROR("server channel error %uh, message: %.*s",
                       m->reply_code, (int)m->reply_text.len,
                       (char *)m->reply_text.bytes);
            break;
        }
        default:
            ELOG_ERROR("unknown server error, method id 0x%08X",
                       x.reply.id);
            break;
        }
        break;
    }
    return 1;
}

int AMQPHelper::init(const std::string &exchange, const std::string &binding_key, const std::function<void(const std::string &msg)> &func)
{
    if (init_)
    {
        ELOG_WARN("AMQPHepler duplicate initialize,just return!!!");
        return 0;
    }

    amqp_rpc_reply_t res;
    conn_ = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(conn_);
    if (!socket)
    {
        ELOG_ERROR("Creating TCP socket failed");
        return 1;
    }

    if (amqp_socket_open(socket, config->rabbitmq_hostname_.c_str(), config->rabbitmq_port_) != AMQP_STATUS_OK)
    {
        ELOG_ERROR("Opening TCP socket failed");
        return 1;
    }

    res = amqp_login(conn_, "/", 0, 131072, 0,
                     AMQP_SASL_METHOD_PLAIN, config->rabbitmq_username_.c_str(),
                     config->rabbitmq_passwd_.c_str());
    if (checkError(res))
    {
        ELOG_ERROR("Logging in failed");
        return 1;
    }

    amqp_channel_open(conn_, 1);
    res = amqp_get_rpc_reply(conn_);
    if (checkError(res))
    {
        ELOG_ERROR("Opening channel failed");
        return 1;
    }

    amqp_queue_declare_ok_t *r = amqp_queue_declare(
        conn_, 1, amqp_empty_bytes, 0, 0, 1, 1, amqp_empty_table);
    res = amqp_get_rpc_reply(conn_);
    if (checkError(res))
    {
        ELOG_ERROR("Declaring queue failed");
        return 1;
    }

    amqp_bytes_t queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL)
    {
        ELOG_ERROR("Out of memory while copying queue name");
        return 1;
    }

    amqp_queue_bind(conn_, 1, queuename, amqp_cstring_bytes(exchange.c_str()),
                    amqp_cstring_bytes(binding_key.c_str()), amqp_empty_table);
    res = amqp_get_rpc_reply(conn_);
    if (checkError(res))
    {
        ELOG_ERROR("Binding queue failed");
        return 1;
    }

    amqp_basic_consume(conn_, 1, queuename, amqp_empty_bytes, 0, 1, 0,
                       amqp_empty_table);
    res = amqp_get_rpc_reply(conn_);
    if (checkError(res))
    {
        ELOG_ERROR("Consuming failed");
        return 1;
    }

    run_ = true;
    recv_thread_ = std::unique_ptr<std::thread>(new std::thread([&, func]() {
        while (run_)
        {
            amqp_rpc_reply_t res;
            amqp_envelope_t envelope;
            struct timeval timeout;

            amqp_maybe_release_buffers(conn_);

            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            res = amqp_consume_message(conn_, &envelope, &timeout, 0);
            if (AMQP_RESPONSE_NORMAL != res.reply_type)
            {
                if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION && res.library_error == AMQP_STATUS_TIMEOUT)
                    continue;
                ELOG_DEBUG("Amqp consumer thread quit...");
                return;
            }

            std::string msg((const char *)envelope.message.body.bytes, envelope.message.body.len);
            func(msg);
            amqp_destroy_envelope(&envelope);
        }
    }));

    send_thread_ = std::unique_ptr<std::thread>(new std::thread([&]() {
        while (run_)
        {
            std::unique_lock<std::mutex> lock(mux_);

            if (!queue_.empty())
            {
                AMQPData data = queue_.front();
                queue_.pop();
                callback(data.exchange, data.queuename, data.binding_key, data.msg);
            }
            else
            {
                cond_.wait(lock);
            }
        }
    }));

    init_ = true;

    return 0;
}

void AMQPHelper::close()
{
    if (!init_)
    {
        ELOG_WARN("AMQPHepler didn't initialize,can't close!!!");
        return;
    }

    run_ = false;
    recv_thread_->join();
    cond_.notify_all();
    send_thread_->join();

    amqp_channel_close(conn_, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn_);

    while (!queue_.empty())
        queue_.pop();

    init_ = false;
    conn_ = nullptr;
    recv_thread_.reset();
    recv_thread_ = nullptr;
    send_thread_.reset();
    send_thread_ = nullptr;
}
