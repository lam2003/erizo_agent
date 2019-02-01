#include "erizo_agent.h"

#include "rabbitmq/amqp_helper.h"
#include "redis/redis_helper.h"
#include "common/port_manager.h"
#include "common/utils.h"
#include "common/config.h"

DEFINE_LOGGER(ErizoAgent, "ErizoAgent");

ErizoAgent *ErizoAgent::instance_ = nullptr;
ErizoAgent::~ErizoAgent()
{
}

ErizoAgent::ErizoAgent()
{
    id_ = "";
    last_update_ = 0;
    idle_process_num_ = 0;
    amqp_uniquecast_ = nullptr;
    init_ = false;
}

ErizoAgent *ErizoAgent::getInstance()
{
    if (instance_ == nullptr)
        instance_ = new ErizoAgent;
    return instance_;
}

int ErizoAgent::init()
{
    if (init_)
        return 0;

    id_ = "ea_" + Utils::getUUID();
    int default_process_num = Config::getInstance()->default_process_num;
    while (default_process_num--)
        newErizoProcess();

    amqp_uniquecast_ = std::make_shared<AMQPHelper>();
    if (amqp_uniquecast_->init(id_, [this](const std::string &msg) {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(msg, root))
            {
                ELOG_ERROR("json parse root failed,dump %s", msg);
                return;
            }
            if (!root.isMember("corrID") ||
                root["corrID"].type() != Json::intValue ||
                !root.isMember("replyTo") ||
                root["replyTo"].type() != Json::stringValue ||
                !root.isMember("data") ||
                root["data"].type() != Json::objectValue)
            {
                ELOG_ERROR("json parse [corrID/replyTo/data] failed,dump %s", msg);
                return;
            }

            int corrid = root["corrID"].asInt();
            std::string reply_to = root["replyTo"].asString();
            Json::Value data = root["data"];
            if (!data.isMember("method") ||
                data["method"].type() != Json::stringValue)
            {
                ELOG_ERROR("json parse method failed,dump %s", msg);
                return;
            }

            Json::Value reply_data = Json::nullValue;
            std::string method = data["method"].asString();
            if (!method.compare("getErizo"))
            {
                reply_data = getErizo(data);
            }
            else
            {
                ELOG_ERROR("unknow method:%s,dump %s", method, msg);
                return;
            }

            Json::Value reply;
            reply["corrID"] = corrid;
            if (reply_data == Json::nullValue)
            {
                reply_data["status"] = "failed";
            }
            else
            {
                reply_data["status"] = "success";
            }
            reply["data"] = reply_data;
            Json::FastWriter writer;
            std::string reply_msg = writer.write(reply);
            amqp_uniquecast_->sendMessage(reply_to,
                                          reply_to,
                                          reply_msg);
        }))
    {
        ELOG_ERROR("amqp initialize failed");
        return 1;
    }

    init_ = true;
    return 0;
}

void ErizoAgent::close()
{
    if (!init_)
        return;

    amqp_uniquecast_->close();
    amqp_uniquecast_.reset();
    amqp_uniquecast_ = nullptr;

    RedisHelper::removeErizoAgent(Config::getInstance()->server_field, id_);
    for (auto it = pid_erizo_mapping_.begin(); it != pid_erizo_mapping_.end(); it++)
        RedisHelper::removeErizo(id_, it->second.id);
    pid_erizo_mapping_.clear();
    roomid_erizo_mapping_.clear();

    id_ = "";
    last_update_ = 0;
    idle_process_num_ = 0;

    init_ = false;
}

int ErizoAgent::dispatch()
{
    uint64_t update_interval = (uint64_t)Config::getInstance()->update_interval;
    uint64_t now = Utils::getSystemMs();

    if (now - last_update_ > update_interval)
    {
        last_update_ = now;
        RedisHelper::addErizoAgent(Config::getInstance()->server_field, *this);
    }
    if (amqp_uniquecast_->dispatch())
    {
        ELOG_ERROR("amqp dispatch failed");
        return 1;
    }

    checkIfSubProcessQuit();

    int min_idle_process_num = Config::getInstance()->min_idle_process_num;
    int max_process_num = Config::getInstance()->max_process_num;
    if (idle_process_num_ < min_idle_process_num && (int)pid_erizo_mapping_.size() < max_process_num)
    {
        int need_process = min_idle_process_num - idle_process_num_;
        while (need_process--)
            newErizoProcess();
    }

    return 0;
}

void ErizoAgent::checkIfSubProcessQuit()
{
    pid_t pid = waitpid(-1, nullptr, WNOHANG);
    if (pid <= 0)
        return;

    auto it = pid_erizo_mapping_.find(pid);
    if (it != pid_erizo_mapping_.end())
    {
        Erizo erizo = pid_erizo_mapping_[pid];
        RedisHelper::removeErizo(id_, erizo.id);
        pid_erizo_mapping_.erase(pid);
        roomid_erizo_mapping_.erase(erizo.room_id);
        ELOG_INFO("erizo process %d quit", pid);
    }
}

int ErizoAgent::newErizoProcess()
{
    uint16_t port;
    if (PortManager::getInstance()->allocPort(port))
    {
        ELOG_ERROR("allocate port failed");
        return 1;
    }

    Erizo erizo;
    erizo.id = "ez_" + Utils::getUUID();
    erizo.agent_id = id_;
    erizo.bridge_ip = Config::getInstance()->bridge_ip;
    erizo.bridge_port = port;
    erizo.room_id = "";

    pid_t pid = fork();
    if (pid == 0)
    {
        std::ostringstream oss;
        oss << port;
        if (execlp(Config::getInstance()->erizo_path.c_str(),
                   "erizo_cpp",
                   erizo.agent_id.c_str(),
                   erizo.id.c_str(),
                   erizo.bridge_ip.c_str(),
                   oss.str().c_str(),
                   0) < 0)
        {
            ELOG_ERROR("execlp failed");
            exit(1);
        }
    }
    else if (pid < 0)
    {
        ELOG_ERROR("fork failed");
        return 1;
    }

    if (RedisHelper::addErizo(id_, erizo))
    {
        ELOG_ERROR("add erizo to redis failed");
        return 1;
    }

    pid_erizo_mapping_[pid] = erizo;
    idle_process_num_++;
    return 0;
}

Json::Value ErizoAgent::getErizo(const Json::Value &root)
{
    if (!root.isMember("roomID") ||
        root["roomID"].type() != Json::stringValue)
    {
        ELOG_ERROR("json parse roomid failed,dump %s", Utils::dumpJson(root));
        return Json::nullValue;
    }

    std::string room_id = root["roomID"].asString();

    Erizo erizo;
    auto it = roomid_erizo_mapping_.find(room_id);
    if (it == roomid_erizo_mapping_.end())
    {
        if (!idle_process_num_)
        {
            ELOG_ERROR("get erizo failed,not idle erizo process");
            return Json::nullValue;
        }

        std::vector<Erizo> erizos;
        if (RedisHelper::getAllErizo(id_, erizos))
        {
            ELOG_ERROR("getall erizo from redis failed");
            return Json::nullValue;
        }

        auto it = std::find_if(erizos.begin(), erizos.end(), [](const Erizo &erizo) {
            return erizo.room_id == "";
        });

        if (it != erizos.end())
        {
            erizo = *it;
            erizo.room_id = room_id;
            roomid_erizo_mapping_[room_id] = erizo;
            idle_process_num_--;
            if (RedisHelper::addErizo(id_, erizo))
            {
                ELOG_ERROR("add erizo to redis failed");
                return Json::nullValue;
            }
        }
        else
        {
            ELOG_ERROR("get erizo failed,not idle erizo process");
            return Json::nullValue;
        }
    }
    else
    {
        erizo = roomid_erizo_mapping_[room_id];
    }

    Json::Value data;
    data["erizoID"] = erizo.id;
    data["bridgeIP"] = erizo.bridge_ip;
    data["bridgePort"] = erizo.bridge_port;
    return data;
};
