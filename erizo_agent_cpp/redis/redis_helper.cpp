#include "redis_helper.h"

int RedisHelper::addErizoAgent(const std::string &area, const ErizoAgent &agent)
{
    std::string key = "erizo_agent_" + area;
    if (ACLRedis::getInstance()->hset(key, agent.id, agent.toJSON()) == -1)
        return 1;
    return 0;
}

int RedisHelper::removeErizoAgent(const std::string &area, const std::string &agent_id)
{
    std::string key = "erizo_agent_" + area;
    if (ACLRedis::getInstance()->hdel(key, agent_id) == -1)
        return 1;
    return 0;
}

int RedisHelper::addErizo(const Erizo &erizo)
{
    if (ACLRedis::getInstance()->hset("erizo", erizo.id, erizo.toJSON()) == -1)
        return 1;
    return 0;
}

int RedisHelper::removeErizo(const std::string &erizo_id)
{
    if (ACLRedis::getInstance()->hdel("erizo", erizo_id) == -1)
        return 1;
    return 0;
}