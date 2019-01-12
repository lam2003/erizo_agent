#include <map>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <json/json.h>

#include "common/utils.h"
#include "common/config.h"
#include "model/erizo.h"
#include "model/erizo_agent.h"
#include "rabbitmq/amqp_helper.h"
#include "redis/redis_helper.h"

static ErizoAgent erizo_agent;
static std::map<pid_t, Erizo> erizo_map1;
static std::map<std::string, Erizo> erizo_map2;
static bool run = true;

static void sigchld_handler(int signo)
{
    pid_t pid = waitpid(-1, NULL, 0);
    auto it = erizo_map1.find(pid);
    if (it != erizo_map1.end())
    {
        Erizo erizo = erizo_map1[pid];
        erizo_map1.erase(pid);
        erizo_map2.erase(erizo.room_id);
    }
}
static void sigint_handler(int signo)
{
    run = false;
}
static void sigterm_handler(int signo)
{
    run = false;
}

static pid_t newErizoProcess(const std::string &erizo_id)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execlp(Config::getInstance()->erizo_path_.c_str(), "erizo_cpp", erizo_agent.id.c_str(), erizo_id.c_str(), 0) < 0)
        {
            printf("execlp failed,sub process quit\n");
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
        std::string erizo_id = "ez_" + Utils::getUUID();
        pid_t pid = newErizoProcess(erizo_id);
        if (pid < 0)
            return Json::nullValue;

        erizo.id = erizo_id;
        erizo.room_id = room_id;
        erizo_map1[pid] = erizo;
        erizo_map2[room_id] = erizo;
    }
    else
    {
        erizo = erizo_map2[room_id];
    }

    Json::Value data;
    data["erizoID"] = erizo.id;
    return data;
};

int main()
{
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigterm_handler);
    erizo_agent.id = "ea_" + Utils::getUUID();

    Utils::initPath();
    Config::getInstance()->init("config.json");

    if (RedisHelper::getInstance()->init())
    {
        printf("redis init failed");
        return 1;
    }

    AMQPHelper::getInstance()->init(Config::getInstance()->uniquecast_exchange_, erizo_agent.id, [](const std::string &msg) {
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

        Json::Value reply;
        reply["corrID"] = corrid;
        reply["data"] = reply_data;
        Json::FastWriter writer;
        std::string reply_msg = writer.write(reply);
        AMQPHelper::getInstance()->sendMessage(Config::getInstance()->uniquecast_exchange_,
                                               reply_to,
                                               reply_to,
                                               reply_msg);
    });

    while (run)
    {
        uint64_t now = Utils::getCurrentMs();

        if (now - erizo_agent.last_update > (uint64_t)Config::getInstance()->update_interval_)
        {
            erizo_agent.last_update = now;
            erizo_agent.erizo_process_num = erizo_map1.size();
            RedisHelper::getInstance()->addErizoAgent(Config::getInstance()->area_, erizo_agent);
        }
        if (AMQPHelper::getInstance()->dispatch())
        {
            printf("amqp failed\n");
            break;
        }
    }
    AMQPHelper::getInstance()->close();
    RedisHelper::getInstance()->close();
}