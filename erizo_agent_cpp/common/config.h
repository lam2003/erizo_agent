#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <json/json.h>
#include <logger.h>

class Config
{
  DECLARE_LOGGER();

public:
  static Config *getInstance();
  ~Config();
  int init(const std::string &config_file);

public:
  // RabbitMQ config
  std::string rabbitmq_username_;
  std::string rabbitmq_passwd_;
  std::string rabbitmq_hostname_;
  unsigned short rabbitmq_port_;
  std::string uniquecast_exchange_;
  std::string boardcast_exchange_;

  //redis
  std::string redis_ip_;
  unsigned short redis_port_;
  std::string redis_password_;

  //Agent config
  std::string erizo_path_;
  std::string area_;
  int update_interval_;

private:
  Config();

private:
  static Config *instance_;
};

#endif