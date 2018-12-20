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

    agent_ip_ = "172.19.5.28";
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
    if (rabbitmq.isNull() ||
        rabbitmq.type() != Json::objectValue ||
        rabbitmq["host"].isNull() ||
        rabbitmq["host"].type() != Json::stringValue ||
        rabbitmq["port"].isNull() ||
        rabbitmq["port"].type() != Json::intValue ||
        rabbitmq["username"].isNull() ||
        rabbitmq["username"].type() != Json::stringValue ||
        rabbitmq["password"].isNull() ||
        rabbitmq["password"].type() != Json::stringValue)
    {
        ELOG_ERROR("Rabbitmq config check error");
        return 1;
    }

    rabbitmq_hostname_ = rabbitmq["host"].asString();
    rabbitmq_port_ = rabbitmq["port"].asInt();
    rabbitmq_username_ = rabbitmq["username"].asString();
    rabbitmq_passwd_ = rabbitmq["password"].asString();

    Json::Value agent = root["agent"];
    if (rabbitmq.isNull() ||
        rabbitmq.type() != Json::objectValue ||
        agent["ip"].isNull() ||
        agent["ip"].type() != Json::stringValue)
    {
        ELOG_ERROR("Agent config check error");
        return 1;
    }

    agent_ip_ = agent["ip"].asString();

    return 0;
}
