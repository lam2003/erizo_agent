#ifndef REDIS_HELPER_H
#define REDIS_HELPER_H

#include <string>

#include "acl_redis.h"
#include "model/erizo_agent.h"
#include "model/erizo.h"

class RedisHelper
{
public:
  static int addErizoAgent(const std::string &area, const ErizoAgent &agent);
  static int removeErizoAgent(const std::string &area, const std::string &agent_id);
  static int addErizo(const Erizo &erizo);
  static int removeErizo(const std::string &erizo_id);
};
#endif