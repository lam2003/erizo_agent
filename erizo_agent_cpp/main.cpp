#include <map>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <json/json.h>

#include "common/utils.h"
#include "common/config.h"
#include "common/port_manager.h"

#include "model/erizo.h"
#include "model/erizo_agent.h"

#include "rabbitmq/amqp_helper.h"
#include "redis/redis_helper.h"

DEFINE_FUNC_LOGGER("ErizoAgent")

static ErizoAgent erizo_agent;
static std::map<pid_t, Erizo> erizo_map1;
static std::map<std::string, Erizo> erizo_map2;
static bool run = true;

static void sigint_handler(int signo)
{
    ELOG_INFO("main process quit");
    run = false;
}

static void checkSubProcess()
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid <= 0)
        return;

    ELOG_INFO("sub process %d quit", pid);
    auto it = erizo_map1.find(pid);
    if (it != erizo_map1.end())
    {
        Erizo erizo = erizo_map1[pid];
        RedisHelper::removeErizo(erizo.id);
        erizo_map1.erase(pid);
        erizo_map2.erase(erizo.room_id);
    }
}

static pid_t newErizoProcess(const std::string &erizo_id, const std::string &bridge_ip, uint16_t bridge_port)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        std::ostringstream oss;
        oss << bridge_port;
        if (execlp(Config::getInstance()->erizo_path_.c_str(),
                   "erizo_cpp",
                   erizo_agent.id.c_str(),
                   erizo_id.c_str(),
                   bridge_ip.c_str(),
                   oss.str().c_str(),
                   0) < 0)
        {
            ELOG_ERROR("execlp failed,%s", strerror(errno));
            exit(1);
        }
    }
    return pid;
}

static Json::Value getErizo(const Json::Value &root)
{
    if (!root.isMember("roomID") ||
        root["roomID"].type() != Json::stringValue)
        return Json::nullValue;

    std::string room_id = root["roomID"].asString();

    Erizo erizo;
    auto it = erizo_map2.find(room_id);
    if (it == erizo_map2.end())
    {
        std::string bridge_ip = Config::getInstance()->bridge_ip_;
        uint16_t bridge_port;
        if (PortManager::getInstance()->allocPort(bridge_port))
        {
            ELOG_ERROR("allocate port failed");
            return Json::nullValue;
        }

        std::string erizo_id = "ez_" + Utils::getUUID();
        pid_t pid = newErizoProcess(erizo_id, bridge_ip, bridge_port);
        if (pid < 0)
        {
            ELOG_ERROR("fork sub process failed");
            return Json::nullValue;
        }

        ELOG_INFO("new sub process %d", pid);
        erizo.id = erizo_id;
        erizo.room_id = room_id;
        erizo.bridge_ip = bridge_ip;
        erizo.bridge_port = bridge_port;
        erizo.agent_id = erizo_agent.id;
        RedisHelper::addErizo(erizo);
        erizo_map1[pid] = erizo;
        erizo_map2[room_id] = erizo;
    }
    else
    {
        erizo = erizo_map2[room_id];
    }

    Json::Value data;
    data["erizoID"] = erizo.id;
    data["bridgeIP"] = erizo.bridge_ip;
    data["bridgePort"] = erizo.bridge_port;
    return data;
};

int main()
{
    srand(time(0));
    signal(SIGINT, sigint_handler);

    erizo_agent.id = "ea_" + Utils::getUUID();
    Utils::initPath();

    if (Config::getInstance()->init("config.json"))
    {
        ELOG_ERROR("load configure failed");
        return 1;
    }

    if (ACLRedis::getInstance()->init())
    {
        ELOG_ERROR("redis initialize failed");
        return 1;
    }

    if (AMQPHelper::getInstance()->init(Config::getInstance()->uniquecast_exchange_, erizo_agent.id, [](const std::string &msg) {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(msg, root))
                return;

            if (!root.isMember("corrID") ||
                root["corrID"].type() != Json::intValue ||
                !root.isMember("replyTo") ||
                root["replyTo"].type() != Json::stringValue ||
                !root.isMember("data") ||
                root["data"].type() != Json::objectValue)
                return;

            int corrid = root["corrID"].asInt();
            std::string reply_to = root["replyTo"].asString();
            Json::Value data = root["data"];
            if (!data.isMember("method") ||
                data["method"].type() != Json::stringValue)
                return;

            Json::Value reply_data = Json::nullValue;
            std::string method = data["method"].asString();
            if (!method.compare("getErizo"))
            {
                reply_data = getErizo(data);
            }

            if (reply_data == Json::nullValue)
            {
                ELOG_ERROR("handle message failed,dump:%s", msg);
                return;
            }

            Json::Value reply;
            reply["corrID"] = corrid;
            reply["data"] = reply_data;
            Json::FastWriter writer;
            std::string reply_msg = writer.write(reply);
            AMQPHelper::getInstance()->sendMessage(Config::getInstance()->uniquecast_exchange_,
                                                   reply_to,
                                                   reply_to,
                                                   reply_msg);
        }))
    {
        ELOG_ERROR("rabbitmq initialize failed");
        return 1;
    }

    while (run)
    {
        uint64_t now = Utils::getSystemMs();

        if (now - erizo_agent.last_update > (uint64_t)Config::getInstance()->update_interval_)
        {
            erizo_agent.last_update = now;
            erizo_agent.erizo_process_num = erizo_map1.size();
            RedisHelper::addErizoAgent(Config::getInstance()->area_, erizo_agent);
        }
        if (AMQPHelper::getInstance()->dispatch())
        {
            ELOG_ERROR("amqp dispatch failed");
            break;
        }
        checkSubProcess();
    }
    RedisHelper::removeErizoAgent(Config::getInstance()->area_, erizo_agent.id);
    for (auto it = erizo_map1.begin(); it != erizo_map1.end(); it++)
        RedisHelper::removeErizo(it->second.id);

    AMQPHelper::getInstance()->close();
    ACLRedis::getInstance()->close();
}