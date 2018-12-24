#ifndef REDIS_HELPER_H
#define REDIS_HELPER_H

#include <memory>
#include <thread>
#include <mutex>

#include <logger.h>
#include <boost/asio/io_service.hpp>
#include <redisclient/redissyncclient.h>

class RedisHelper
{
  DECLARE_LOGGER();

public:
  RedisHelper();
  ~RedisHelper();

  int init(const std::string &redis_ip, const unsigned short redis_port, const std::string &password = "");
  void close();
  redisclient::RedisValue command(const std::string &cmd,
                                  const std ::deque<redisclient::RedisBuffer> &buffer);

private:
  std::shared_ptr<redisclient::RedisSyncClient> redis_;
  bool init_;
  std::mutex mux_;
  boost::asio::io_service ios_;
};

#endif
