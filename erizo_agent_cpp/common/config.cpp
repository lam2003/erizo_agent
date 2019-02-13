#include "config.h"

#include <fstream>

DEFINE_LOGGER(Config, "Config");

Config *Config::instance_ = nullptr;

Config::Config()
{
    rabbitmq_username = "linmin";
    rabbitmq_passwd = "linmin";
    rabbitmq_hostname = "127.0.0.1";
    rabbitmq_port = 5672;
    uniquecast_exchange = "erizo_uniquecast_exchange";
    boardcast_exchange = "erizo_boardcast_exchange";

    redis_ip = "127.0.0.1";
    redis_port = 6379;
    redis_passwd = "cathy978";
    redis_conn_timeout = 10;
    redis_rw_timeout = 10;
    redis_max_conns = 100;

    erizo_path = "/test/cpp/erizo_cpp/bin/erizo_cpp";
    server_field = "default_area";
    update_interval = 5000;
    default_process_num = 2;
    min_idle_process_num = 2;
    max_process_num = 5;

    min_bridge_port = 20000;
    max_bridge_port = 30000;
    bridge_ip = "172.19.5.28";
}

Config *Config::getInstance()
{
    if (instance_ == nullptr)
        instance_ = new Config;
    return instance_;
}

Config::~Config()
{
}

int Config::init(const std::string &config_file)
{
    std::ifstream ifs(config_file, std::ios::binary);
    if (!ifs.is_open())
    {
        ELOG_ERROR("open %s failed", config_file);
        return 1;
    }

    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    if (!reader.parse(ifs, root))
    {
        ELOG_ERROR("parse %s failed", config_file);
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
        ELOG_ERROR("rabbitmq config check error");
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
        redis["password"].type() != Json::stringValue ||
        !redis.isMember("conn_timeout") ||
        redis["conn_timeout"].type() != Json::intValue ||
        !redis.isMember("rw_timeout") ||
        redis["rw_timeout"].type() != Json::intValue ||
        !redis.isMember("max_conns") ||
        redis["max_conns"].type() != Json::intValue)
    {
        ELOG_ERROR("redis config check error");
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
        agent["update_interval"].type() != Json::intValue ||
        !agent.isMember("default_process_num") ||
        agent["default_process_num"].type() != Json::intValue ||
        !agent.isMember("min_idle_process_num") ||
        agent["min_idle_process_num"].type() != Json::intValue ||
        !agent.isMember("max_process_num") ||
        agent["max_process_num"].type() != Json::intValue)
    {
        ELOG_ERROR("agent config check error");
        return 1;
    }

    Json::Value bridge = root["bridge"];
    if (!root.isMember("bridge") ||
        bridge.type() != Json::objectValue ||
        !bridge.isMember("min_port") ||
        bridge["min_port"].type() != Json::intValue ||
        !bridge.isMember("max_port") ||
        bridge["max_port"].type() != Json::intValue ||
        !bridge.isMember("ip") ||
        bridge["ip"].type() != Json::stringValue)
    {
        ELOG_ERROR("bridge config check error");
        return 1;
    }

    rabbitmq_hostname = rabbitmq["host"].asString();
    rabbitmq_port = rabbitmq["port"].asInt();
    rabbitmq_username = rabbitmq["username"].asString();
    rabbitmq_passwd = rabbitmq["password"].asString();
    uniquecast_exchange = rabbitmq["uniquecast_exchange"].asString();
    boardcast_exchange = rabbitmq["boardcast_exchange"].asString();

    redis_ip = redis["ip"].asString();
    redis_port = redis["port"].asInt();
    redis_passwd = redis["password"].asString();
    redis_conn_timeout = redis["conn_timeout"].asInt();
    redis_rw_timeout = redis["rw_timeout"].asInt();
    redis_max_conns = redis["max_conns"].asInt();

    erizo_path = agent["erizo_path"].asString();
    server_field = agent["area"].asString();
    update_interval = agent["update_interval"].asInt();
    default_process_num = agent["default_process_num"].asInt();
    min_idle_process_num = agent["min_idle_process_num"].asInt();
    max_process_num = agent["max_process_num"].asInt();

    min_bridge_port = bridge["min_port"].asInt();
    max_bridge_port = bridge["max_port"].asInt();
    bridge_ip = bridge["ip"].asString();

    return 0;
}
