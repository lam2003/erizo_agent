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
    redis_password_ = "cathy978";

    erizo_path_ = "/test/cpp/erizo_cpp/bin/erizo_cpp";
    area_ = "hua_nan";
    update_interval_ = 10000;
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

    Json::Value rabbitmq = root["rabbitmq"];
    if (!root.isMember("rabbitmq") ||
        rabbitmq.type() != Json::objectValue ||
        !rabbitmq.isMember("host") ||
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

    Json::Value redis = root["redis"];
    if (!root.isMember("redis") ||
        redis.type() != Json::objectValue ||
        !redis.isMember("ip") ||
        redis["ip"].type() != Json::stringValue ||
        !redis.isMember("port") ||
        redis["port"].type() != Json::intValue ||
        !redis.isMember("password") ||
        redis["password"].type() != Json::stringValue)
    {
        ELOG_ERROR("Redis config check error");
        return 1;
    }

    Json::Value agent = root["agent"];
    if (!root.isMember("agent") ||
        agent.type() != Json::objectValue ||
        !agent.isMember("erizo_path") ||
        agent["erizo_path"].type() != Json::stringValue ||
        !agent.isMember("area") ||
        agent["area"].type() != Json::stringValue ||
        !agent.isMember("update_interval") ||
        agent["update_interval"].type() != Json::intValue)
    {
        ELOG_ERROR("Agent config check error");
        return 1;
    }

    rabbitmq_hostname_ = rabbitmq["host"].asString();
    rabbitmq_port_ = rabbitmq["port"].asInt();
    rabbitmq_username_ = rabbitmq["username"].asString();
    rabbitmq_passwd_ = rabbitmq["password"].asString();
    uniquecast_exchange_ = rabbitmq["uniquecast_exchange"].asString();
    boardcast_exchange_ = rabbitmq["boardcast_exchange"].asString();

    redis_ip_ = redis["ip"].asString();
    redis_port_ = redis["port"].asInt();
    redis_password_ = redis["password"].asString();

    erizo_path_ = agent["erizo_path"].asString();
    area_ = agent["area"].asString();
    update_interval_ = agent["update_interval"].asInt();

    return 0;
}
