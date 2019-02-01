#ifndef ACL_REDIS_H
#define ACL_REDIS_H

#include <vector>
#include <string>
#include <memory>

namespace acl
{
class redis_client_cluster;
}

class ACLRedis
{
public:
  static ACLRedis *getInstance();
  ~ACLRedis();

  int init();
  void close();

  int hset(const std::string &key, const std::string &field, const std::string &value);
  int hdel(const std::string &key, const std::string &field);
  int hvals(const std::string &key, std::vector<std::string> &fields, std::vector<std::string> &values);

private:
  ACLRedis();

private:
  std::shared_ptr<acl::redis_client_cluster> cluster_;
  bool init_;
  static ACLRedis *instance_;
};

#endif