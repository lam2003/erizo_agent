#include "redis_helper.h"

#include "common/config.h"

DEFINE_LOGGER(RedisHelper, "RedisHelper");

RedisHelper *RedisHelper::instance_ = nullptr;

RedisHelper *RedisHelper::getInstance()
{
    if (!instance_)
        instance_ = new RedisHelper;
    return instance_;
}

RedisHelper::RedisHelper() : redis_(nullptr), init_(false) {}

RedisHelper::~RedisHelper() {}

int RedisHelper::init()
{
    if (init_)
        return 0;

    boost::asio::ip::address address = boost::asio::ip::address::from_string(Config::getInstance()->redis_ip_);
    boost::asio::ip::tcp::endpoint endpoint(address, Config::getInstance()->redis_port_);
    boost::system::error_code ec;

    redis_ = std::make_shared<redisclient::RedisSyncClient>(ios_);
    redis_->connect(endpoint, ec);
    if (ec)
    {
        ELOG_ERROR("redis connect failed:%s", ec.message());
        return 1;
    }

    redisclient::RedisValue result;
    result = redis_->command("AUTH", {Config::getInstance()->redis_password_});
    if (result.isError())
    {
        ELOG_ERROR("redis auth failed");
        return 1;
    }
    init_ = true;

    return 0;
}

void RedisHelper::close()
{
    if (!init_)
        return;
    redis_->disconnect();
    redis_.reset();
    redis_ = nullptr;
    init_ = false;
}

int RedisHelper::addErizoAgent(const std::string &area, const ErizoAgent &agent)
{
    redisclient::RedisValue val;
    val = redis_->command("HSET", {area, agent.id, agent.toJSON()});
    if (!val.isOk())
        return 1;
    return 0;
}

int RedisHelper::removeErizoAgent(const std::string &area, const std::string &agent_id)
{
    redisclient::RedisValue val;
    val = redis_->command("HDEL", {area, agent_id});
    if (!val.isOk())
        return 1;
    return 0;
}

// int RedisHelper::addErizo(const std::string &agent_id, const Erizo &erizo)
// {
//     redisclient::RedisValue val;
//     val = redis_->command("HSET", {agent_id, erizo.room_id, erizo.toJSON()});
//     if (!val.isOk())
//         return 1;
//     return 0;
// }

// int RedisHelper::getErizo(const std::string &agent_id, const std::string &room_id, Erizo &erizo)
// {
//     redisclient::RedisValue val;
//     val = redis_->command("HGET", {agent_id, room_id});
//     if (!val.isOk() || !val.isString())
//         return 1;
//     if (Erizo::fromJSON(val.toString(), erizo))
//         return 1;
//     return 0;
// }

// int RedisHelper::removeErizo(const std::string &agent_id, const std::string &room_id)
// {
//     redisclient::RedisValue val;
//     val = redis_->command("HDEL", {agent_id, room_id});
//     if (!val.isOk())
//         return 1;
//     return 0;
// }