#include "port_manager.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

DEFINE_LOGGER(PortManager, "PortManager");

PortManager *PortManager::instance_ = nullptr;
PortManager *PortManager::getInstance()
{
    if (!instance_)
        instance_ = new PortManager;
    return instance_;
}

PortManager::PortManager()
{
}

int PortManager::checkPort(unsigned short port)
{
    char buf[256] = {0};
    char cmd[256] = {0};
    FILE *fp = NULL;

    snprintf(cmd, 256, "netstat -naut | grep -i \"%d\" | wc -l", port);
    if ((fp = popen(cmd, "r")) == nullptr)
    {
        ELOG_ERROR("popen failed");
        return 1;
    }
    if (fgets(buf, sizeof(buf) / sizeof(buf[0]), fp) == nullptr)
    {
        ELOG_ERROR("fgets error");
        pclose(fp);
        return 1;
    }
    int num = atoi(buf);
    return num != 0 ? 1 : 0;
}

int PortManager::allocPort(unsigned short &port)
{
    int ret;
    int try_time = 10;
    do
    {
        port = Config::getInstance()->min_bridge_port;
        port += rand() % (Config::getInstance()->max_bridge_port - Config::getInstance()->min_bridge_port);
        ret = checkPort(port);
        try_time--;
    } while (ret && try_time);
    return ret;
}