#include "acl_redis.h"
#include "common/config.h"

#include <sstream>

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
    oss << Config::getInstance()->redis_ip_ << ":" << Config::getInstance()->redis_port_;

    cluster_ = std::make_shared<acl::redis_client_cluster>();
    cluster_->set(oss.str().c_str(),
                  Config::getInstance()->redis_max_conns_,
                  Config::getInstance()->redis_conn_timeout_,
                  Config::getInstance()->redis_rw_timeout_);
    cluster_->set_password("default", Config::getInstance()->redis_password_.c_str());
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
    cmd.set_cluster(cluster_.get(), Config::getInstance()->redis_max_conns_);
    int res = cmd.hset(key.c_str(), field.c_str(), value.c_str());
    cmd.clear();
    return res;
}

int ACLRedis::hdel(const std::string &key, const std::string &field)
{
    if (!init_)
        return false;
    acl::redis_hash cmd;
    cmd.set_cluster(cluster_.get(), Config::getInstance()->redis_max_conns_);
    int res = cmd.hdel(key.c_str(), field.c_str());
    cmd.clear();
    return res;
}
