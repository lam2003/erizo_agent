#include "redis_helper.h"

#include <boost/asio/ip/address.hpp>

#include "common/config.h"

DEFINE_LOGGER(RedisHelper, "RedisHelper");

RedisHelper::RedisHelper() : redis_(nullptr), init_(false) {}

RedisHelper::~RedisHelper() {}

int RedisHelper::init(const std::string &redis_ip, const unsigned short redis_port, const std::string &password)
{
    if (init_)
    {
        ELOG_WARN("RedisHelper duplicate initialize!!!");
        return 0;
    }

    boost::asio::ip::address address = boost::asio::ip::address::from_string(redis_ip);
    boost::asio::ip::tcp::endpoint endpoint(address, redis_port);
    boost::system::error_code ec;

    redis_ = std::make_shared<redisclient::RedisSyncClient>(ios_);
    redis_->connect(endpoint, ec);
    if (ec)
    {
        ELOG_ERROR("Connect to redis failed:%s", ec.message());
        return 1;
    }

    redisclient::RedisValue result;
    result = redis_->command("AUTH", {password});
    if (result.isError())
    {
        ELOG_ERROR("Auth failed");
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

redisclient::RedisValue RedisHelper::command(const std::string &cmd,
                                             const std ::deque<redisclient::RedisBuffer> &buffer)
{
    if (!init_)
    {
        ELOG_ERROR("RedisHelper not initialize");
        return 1;
    }

    std::unique_lock<std::mutex>(mux_);
    return redis_->command(cmd, buffer);
}