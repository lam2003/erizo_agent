#ifndef REDIS_HELPER_H
#define REDIS_HELPER_H

#include <string>
#include <vector>

#include "core/erizo_agent.h"
#include "model/erizo.h"

class RedisHelper
{
public:
  static int addErizoAgent(const std::string &area, const ErizoAgent &agent);
  static int removeErizoAgent(const std::string &area, const std::string &agent_id);

  static int addErizo(const std::string &agent_id, const Erizo &erizo);
  static int removeErizo(const std::string &agent_id, const std::string &erizo_id);
  static int getAllErizo(const std::string &agent_id, std::vector<Erizo> &erizos);
};
#endif