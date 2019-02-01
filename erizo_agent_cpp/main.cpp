#include <map>

#include <unistd.h>
#include <signal.h>

#include <json/json.h>

#include "common/utils.h"
#include "common/config.h"
#include "core/erizo_agent.h"
#include "redis/acl_redis.h"

DEFINE_FUNC_LOGGER("Main")
static bool run = true;

static void sigint_handler(int signo)
{
    run = false;
}

int main()
{
    srand(time(0));
    signal(SIGINT, sigint_handler);
    if (Utils::initPath())
    {
        ELOG_ERROR("working path initialize failed");
        return 1;
    }

    if (Config::getInstance()->init("config.json"))
    {
        ELOG_ERROR("load configure file failed");
        return 1;
    }

    if (ACLRedis::getInstance()->init())
    {
        ELOG_ERROR("acl-redis initialize failed");
        return 1;
    }

    if (ErizoAgent::getInstance()->init())
    {
        ELOG_ERROR("erizo-agent initialize failed");
        return 1;
    }

    while (run)
    {
        if (ErizoAgent::getInstance()->dispatch())
        {
            ELOG_ERROR("erizo-agent dispatch failed");
            return 1;
        }
    }

    ErizoAgent::getInstance()->close();
    ACLRedis::getInstance()->close();
    return 0;
}