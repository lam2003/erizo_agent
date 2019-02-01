#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <json/json.h>

#include "common/logger.h"

class Config
{
  DECLARE_LOGGER();

public:
  static Config *getInstance();
  ~Config();
  int init(const std::string &config_file);

public:
  // RabbitMQ config
  std::string rabbitmq_username;
  std::string rabbitmq_passwd;
  std::string rabbitmq_hostname;
  unsigned short rabbitmq_port;
  std::string uniquecast_exchange;
  std::string boardcast_exchange;

  //redis
  std::string redis_ip;
  unsigned short redis_port;
  std::string redis_passwd;
  int redis_conn_timeout;
  int redis_rw_timeout;
  int redis_max_conns;

  //Agent config
  std::string erizo_path;
  std::string server_field;
  int update_interval;
  int default_process_num;
  int min_idle_process_num;
  int max_process_num;

  //bridge
  unsigned int min_bridge_port;
  unsigned int max_bridge_port;
  std::string bridge_ip;

private:
  Config();

private:
  static Config *instance_;
};

#endif