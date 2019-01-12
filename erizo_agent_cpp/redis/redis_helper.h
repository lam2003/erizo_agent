#ifndef REDIS_HELPER_H
#define REDIS_HELPER_H

#include <memory>
#include <string>

#include <boost/asio/io_service.hpp>
#include <redisclient/redissyncclient.h>
#include <logger.h>

#include "model/erizo_agent.h"
#include "model/erizo.h"

class RedisHelper
{
  DECLARE_LOGGER();

public:
  ~RedisHelper();
  static RedisHelper *getInstance();

  int init();
  void close();

  int addErizoAgent(const std::string &area, const ErizoAgent &agent);
  int removeErizoAgent(const std::string &area, const std::string &agent_id);

  int addErizo(const std::string &agent_id, const Erizo &erizo);
  int getErizo(const std::string &agent_id, const std::string &room_id, Erizo &erizo);
  int removeErizo(const std::string &agent_id, const std::string &room_id);

private:
  RedisHelper();

private:
  boost::asio::io_service ios_;
  std::shared_ptr<redisclient::RedisSyncClient> redis_;
  bool init_;

  static RedisHelper *instance_;
};
#endif