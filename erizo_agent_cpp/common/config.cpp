#include "config.h"

#include <fstream>

DEFINE_LOGGER(Config, "Config");

Config *Config::instance_ = nullptr;

Config::Config()
{
    rabbitmq_username_ = "linmin";
    rabbitmq_passwd_ = "linmin";
    rabbitmq_hostname_ = "localhost";
    rabbitmq_port_ = 5672;
    uniquecast_exchange_ = "erizo_uniquecast_exchange";
    boardcast_exchange_ = "erizo_boardcast_exchange";

    redis_ip_ = "127.0.0.1";
    redis_port_ = 6379;
    redis_passwd_ = "";

    agent_ip_ = "172.19.5.28";
    erizo_path = "/test/cpp/erizo_cpp/bin/erizo_cpp";
}

Config *Config::getInstance()
{
    if (instance_ == nullptr)
        instance_ = new Config;
    return instance_;
}

Config::~Config()
{
    if (instance_ != nullptr)
    {
        delete instance_;
        instance_ = nullptr;
    }
}

int Config::init(const std::string &config_file)
{
    std::ifstream ifs(config_file, std::ios::binary);
    if (!ifs.is_open())
    {
        ELOG_ERROR("Open %s failed", config_file);
        return 1;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root))
    {
        ELOG_ERROR("Parse %s failed", config_file);
        return 1;
    }

    if(!root.isMember("rabbitmq") || root["rabbitmq"].type() != Json::objectValue) {
        ELOG_ERROR("Rabbitmq config check error");
        return 1;
    }

    Json::Value rabbitmq = root["rabbitmq"];
    if (!rabbitmq.isMember("host") ||
        rabbitmq["host"].type() != Json::stringValue ||
        !rabbitmq.isMember("port") ||
        rabbitmq["port"].type() != Json::intValue ||
        !rabbitmq.isMember("username") ||
        rabbitmq["username"].type() != Json::stringValue ||
        !rabbitmq.isMember("password") ||
        rabbitmq["password"].type() != Json::stringValue ||
        !rabbitmq.isMember("boardcast_exchange") ||
        rabbitmq["boardcast_exchange"].type() != Json::stringValue ||
        !rabbitmq.isMember("uniquecast_exchange") ||
        rabbitmq["uniquecast_exchange"].type() != Json::stringValue)
    {
        ELOG_ERROR("Rabbitmq config check error");
        return 1;
    }

    rabbitmq_hostname_ = rabbitmq["host"].asString();
    rabbitmq_port_ = rabbitmq["port"].asInt();
    rabbitmq_username_ = rabbitmq["username"].asString();
    rabbitmq_passwd_ = rabbitmq["password"].asString();
    uniquecast_exchange_ = rabbitmq["uniquecast_exchange"].asString();
    boardcast_exchange_ = rabbitmq["boardcast_exchange"].asString();

    if(!root.isMember("redis") || !root["redis"].isObject()) {
        ELOG_ERROR("redis config check error");
        return 1;
    }
    
    Json::Value redis = root["redis"];
    if(!redis.isMember("ip") || !redis["ip"].isString() || 
       !redis.isMember("port") || !redis["port"].isUInt() ||
       !redis.isMember("password") || !redis["password"].isString()) {
        ELOG_ERROR("redis config check error");
        return 1;
    }
    redis_ip_ = redis["ip"].asString();
    redis_port_ = redis["port"].asUInt();
    redis_passwd_ = redis["password"].asString();

    Json::Value agent = root["agent"];
    if (!root.isMember("agent") ||
        agent.type() != Json::objectValue ||
        !agent.isMember("ip") ||
        agent["ip"].type() != Json::stringValue ||
        agent.isMember("area_type") || 
        agent["area_type"].type() != Json::intValue || 
        !agent.isMember("erizo_path") ||
        agent["erizo_path"].type() != Json::stringValue)
    {
        ELOG_ERROR("Agent config check error");
        return 1;
    }

    agent_ip_ = agent["ip"].asString();
    area_type_ = agent["area_type"].asInt();
    erizo_path = agent["erizo_path"].asString();

    return 0;
}
