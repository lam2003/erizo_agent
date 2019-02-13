#include "amqp_recv.h"

#include "amqp_cli.h"
#include "common/config.h"

#include <unistd.h>

DEFINE_LOGGER(AMQPRecv, "AMQPRecv");

AMQPRecv::AMQPRecv() : amqp_cli_(nullptr),
                       init_(false) {}

AMQPRecv::~AMQPRecv() {}

int AMQPRecv::init(const std::string &binding_key, const std::function<void(const std::string &msg)> &func)
{
    if (init_)
        return 0;

    amqp_cli_ = std::unique_ptr<AMQPCli>(new AMQPCli);
    if (amqp_cli_->init(Config::getInstance()->uniquecast_exchange, "direct", binding_key))
    {
        ELOG_ERROR("amqp-cli initialize failed");
        return 1;
    }

    func_ = func;
    init_ = true;
    return 0;
}

void AMQPRecv::close()
{
    if (!init_)
        return;

    amqp_cli_->close();
    amqp_cli_.reset();
    amqp_cli_ = nullptr;

    init_ = false;
}

int AMQPRecv::dispatch()
{
    if (!init_)
        return 0;

    amqp_connection_state_t conn = amqp_cli_->getConnection();
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;
    struct timeval timeout;

    amqp_maybe_release_buffers(conn);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    res = amqp_consume_message(conn, &envelope, &timeout, 0);
    if (AMQP_RESPONSE_NORMAL != res.reply_type)
    {
        if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION && res.library_error == AMQP_STATUS_TIMEOUT)
            return 0;
        ELOG_ERROR("amqp library error,code:%d", res.library_error);
        return 1;
    }

    std::string msg((const char *)envelope.message.body.bytes, envelope.message.body.len);
    func_(msg);
    amqp_destroy_envelope(&envelope);
    return 0;
}

int AMQPRecv::sendMessage(const std::string &queuename,
                          const std::string &binding_key,
                          const std::string &send_msg)
{
    if (!init_)
        return 0;

    amqp_connection_state_t conn = amqp_cli_->getConnection();
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;
    props.correlation_id = amqp_cstring_bytes("1");
    props.reply_to = amqp_bytes_malloc_dup(amqp_cstring_bytes(queuename.c_str()));
    if (props.reply_to.bytes == NULL)
    {
        ELOG_ERROR("out of memory while copying queue name");
        return 1;
    }

    amqp_basic_publish(conn, 1, amqp_cstring_bytes(Config::getInstance()->uniquecast_exchange.c_str()),
                       amqp_cstring_bytes(binding_key.c_str()), 0, 0,
                       &props, amqp_cstring_bytes(send_msg.c_str()));
    amqp_bytes_free(props.reply_to);
    return 0;
}