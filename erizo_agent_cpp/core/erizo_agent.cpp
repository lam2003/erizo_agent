#include "erizo_agent.h"

#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>

#include "common/utils.h"
#include "common/config.h"

#define MANAGER_SEM_KEY 1231
#define MAIN_SEM_KEY 1232

DEFINE_LOGGER(ErizoAgent, "ErizoAgent");

ErizoAgent::ErizoAgent() : init_(false),
                           id_(""),
                           amqp_broadcast_(nullptr),
                           amqp_uniquecast_(nullptr)
{
}

ErizoAgent::~ErizoAgent()
{
}

int ErizoAgent::init()
{
    if (init_)
    {
        ELOG_WARN("ErizoAgent duplicate initialize,just reture!!!");
        return 0;
    }

    if (sem_.init(MANAGER_SEM_KEY))
    {
        ELOG_ERROR("Manager semaphore initialize failed");
        return 1;
    }


    if (bootErizoProcessManager())
    {
        ELOG_ERROR("Boot erizo process manager failed");
        return 1;
    }

    id_ = Utils::getUUID();
    amqp_broadcast_ = std::make_shared<AMQPHelper>();
    if (amqp_broadcast_->init("broadcastExchange", "ErizoAgent", [&](const std::string &msg) {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(msg, root))
                return;

            Json::Value data = root["data"];
            if (data.isNull() ||
                data.type() != Json::objectValue ||
                data["method"].isNull() ||
                data["method"].type() != Json::stringValue)
            {
                ELOG_ERROR("Unknow message:%s", msg);
                return;
            }

            std::string method = data["method"].asString();
            if (!method.compare("getErizoAgents"))
            {
                getErizoAgents(root);
            }
        }))
    {
        ELOG_ERROR("AMQP broadcast init failed");
        return 1;
    }

    std::string uniquecast_binding_key = "ErizoAgent_" + id_;
    amqp_uniquecast_ = std::make_shared<AMQPHelper>();
    if (amqp_uniquecast_->init("rpcExchange", uniquecast_binding_key, [&](const std::string &msg) {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(msg, root))
                return;

            Json::Value data = root["data"];
            if (data.isNull() ||
                data.type() != Json::objectValue ||
                data["method"].isNull() ||
                data["method"].type() != Json::stringValue)
            {
                ELOG_ERROR("Unknow message:%s", msg);
                return;
            }

            std::string method = data["method"].asString();
            if (!method.compare("getErizo"))
            {
                getErizo(root);
            }
        }))
    {
        ELOG_ERROR("AMQP uniquecast init failed");
        return 1;
    }

    init_ = true;
    return 0;
}
void ErizoAgent::close()
{
    if (!init_)
    {
        ELOG_WARN("ErizoAgent didn't initialize,can't close!!!");
        return;
    }

    amqp_broadcast_->close();
    amqp_broadcast_.reset();
    amqp_broadcast_ = nullptr;

    amqp_uniquecast_->close();
    amqp_uniquecast_.reset();
    amqp_uniquecast_ = nullptr;

    id_ = "";
    init_ = false;
}

void ErizoAgent::getErizoAgents(const Json::Value &root)
{
    if (root["replyTo"].isNull() ||
        root["replyTo"].type() != Json::stringValue)
    {
        ELOG_ERROR("Message format error");
        return;
    }

    std::string reply_to = root["replyTo"].asString();
    Json::Value reply_data;
    reply_data["ip"] = Config::getInstance()->agent_ip_;
    reply_data["id"] = id_;
    reply_data["method"] = "getErizoAgents";

    Json::Value reply;
    reply["data"] = reply_data;
    Json::FastWriter writer;
    std::string msg = writer.write(reply);

    amqp_uniquecast_->addCallback({"rpcExchange", reply_to, reply_to, msg});
}

void ErizoAgent::getErizo(const Json::Value &root)
{
    if (root["replyTo"].isNull() ||
        root["replyTo"].type() != Json::stringValue ||
        root["corrID"].isNull() ||
        root["corrID"].type() != Json::intValue ||
        root["UUID"].isNull() ||
        root["UUID"].type() != Json::stringValue ||
        root["data"].isNull() ||
        root["data"].type() != Json::objectValue)
    {
        ELOG_ERROR("Message format error");
        return;
    }

    int corrid = root["corrID"].asInt();
    std::string reply_to = root["replyTo"].asString();
    std::string uuid = root["UUID"].asString();
    Json::Value data = root["data"];

    if (data["roomID"].isNull() || data["roomID"].type() != Json::stringValue)
    {
        ELOG_ERROR("Data format error");
        return;
    }

    std::string room_id = data["roomID"].asString();
    if (erizos_map_.find(room_id) == erizos_map_.end())
    {
        sem_.semaphoreV();

        char buf[2048];
        int len = read(pipe_[0], buf, sizeof(buf));
        buf[len] = '\0';

        Json::Value manager_msg;
        Json::Reader reader;
        if (!reader.parse(buf, manager_msg))
        {
            ELOG_ERROR("Erizo manager reply message parse failed");
            return;
        }

        if (manager_msg["ret"].isNull() ||
            manager_msg["ret"].type() != Json::intValue)
        {
            ELOG_ERROR("Erizo manager reply message format error");
            return;
        }

        int ret = manager_msg["ret"].asInt();
        if (ret)
        {
            ELOG_ERROR("Erizo new process boot failed");
            return;
        }
        else
        {
            if (manager_msg["erizoID"].isNull() ||
                manager_msg["erizoID"].type() != Json::stringValue ||
                manager_msg["pid"].isNull() ||
                manager_msg["pid"].type() != Json::intValue)
            {
                ELOG_ERROR("Erizo manager reply data format error");
                return;
            }
            std::string erizo_id = manager_msg["erizoID"].asString();
            int pid = manager_msg["pid"].asInt();
            erizos_map_[room_id] = {pid, erizo_id, room_id};
            ELOG_INFO("Erizo new process pid:%d eirzoId:%s roomId:%s ", pid, erizo_id, room_id);
        }
    }

    Json::Value reply_data;
    reply_data["id"] = erizos_map_[room_id].id;

    Json::Value reply;
    reply["corrID"] = corrid;
    reply["UUID"] = uuid;
    reply["data"] = reply_data;
    Json::FastWriter writer;
    std::string msg = writer.write(reply);

    amqp_uniquecast_->addCallback({"rpcExchange", reply_to, reply_to, msg});
}

void sigchld_handler(int signo)
{
    waitpid(-1, NULL, 0);
}

int ErizoAgent::bootErizoProcessManager()
{
    int ret;

    signal(SIGCHLD, sigchld_handler);

    ret = pipe(pipe_);
    if (ret < 0)
    {
        ELOG_ERROR("Create pipe failed");
        return 1;
    }

    manager_pid_ = fork();
    if (manager_pid_ == 0)
    {
        ::close(pipe_[0]);
        char buf[2048];

        while (true)
        {
            sem_.semaphoreP();
            std::string erizo_id = Utils::getUUID();
            int pid = fork();
            if (pid == 0)
            {
                ::close(pipe_[1]);
                if (execlp("erizo_cpp", "erizo_cpp", erizo_id, 0) < 0)
                {
                    ELOG_ERROR("Erizo process execute failed,exit");
                    exit(1);
                }
            }
            else if (pid > 0)
            {
                sprintf(buf, "{\"ret\":0,\"erizoID\":\"%s\",\"pid\":%d}", erizo_id.c_str(), pid);
                do
                {
                    ret = write(pipe_[1], buf, strlen(buf));
                } while (ret < 0 && errno == EINTR);
            }
            else
            {
                sprintf(buf, "{\"ret\":1}");
                do
                {
                    ret = write(pipe_[1], buf, strlen(buf));
                } while (ret < 0 && errno == EINTR);
            }
        }
    }
    else if (manager_pid_ > 0)
    {
        ::close(pipe_[1]);
    }
    else
    {
        ELOG_ERROR("Main process fork:%s", strerror(errno));
        return 1;
    }

    return 0;
}