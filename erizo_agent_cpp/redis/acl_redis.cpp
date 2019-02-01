#include "acl_redis.h"

#include <acl_cpp/lib_acl.hpp>
#include <sstream>

#include "common/config.h"

ACLRedis::ACLRedis() : cluster_(nullptr),
                       init_(false)
{
}

int ACLRedis::init()
{
    if (init_)
        return 0;
    acl::acl_cpp_init();
    std::ostringstream oss;
    oss << Config::getInstance()->redis_ip << ":" << Config::getInstance()->redis_port;

    cluster_ = std::make_shared<acl::redis_client_cluster>();
    cluster_->set(oss.str().c_str(),
                  Config::getInstance()->redis_max_conns,
                  Config::getInstance()->redis_conn_timeout,
                  Config::getInstance()->redis_rw_timeout);
    cluster_->set_password("default", Config::getInstance()->redis_passwd.c_str());
    init_ = true;
    return 0;
}

void ACLRedis::close()
{
    if (!init_)
        return;
    cluster_.reset();
    cluster_ = nullptr;
    init_ = false;
}

ACLRedis::~ACLRedis()
{
}

ACLRedis *ACLRedis::instance_ = nullptr;
ACLRedis *ACLRedis::getInstance()
{
    if (!instance_)
        instance_ = new ACLRedis;
    return instance_;
}

int ACLRedis::hset(const std::string &key, const std::string &field, const std::string &value)
{
    if (!init_)
        return false;
    acl::redis_hash cmd;
    cmd.set_cluster(cluster_.get(), Config::getInstance()->redis_max_conns);
    int res = cmd.hset(key.c_str(), field.c_str(), value.c_str());
    cmd.clear();
    return res;
}

int ACLRedis::hdel(const std::string &key, const std::string &field)
{
    if (!init_)
        return false;
    acl::redis_hash cmd;
    cmd.set_cluster(cluster_.get(), Config::getInstance()->redis_max_conns);
    int res = cmd.hdel(key.c_str(), field.c_str());
    cmd.clear();
    return res;
}

int ACLRedis::hvals(const std::string &key, std::vector<std::string> &fields, std::vector<std::string> &values)
{
    if (!init_)
        return false;
    acl::redis_hash cmd;
    cmd.set_cluster(cluster_.get(), Config::getInstance()->redis_max_conns);
    std::map<acl::string, acl::string> buf;
    int res = cmd.hgetall(key.c_str(), buf);
    cmd.clear();

    for (auto it = buf.begin(); it != buf.end(); it++)
    {
        fields.push_back(std::string(it->first.c_str()));
        values.push_back(std::string(it->second.c_str()));
    }
    return res;
}