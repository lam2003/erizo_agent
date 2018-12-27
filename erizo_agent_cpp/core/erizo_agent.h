#ifndef ERIZO_AGENT_H
#define ERIZO_AGENT_H
#include <thread>
#include <condition_variable>
#include <mutex>

#include <logger.h>
#include <json/json.h>

#include "rabbitmq/amqp_helper.h"
#include "common/semaphore.h"
#include "redis/redis_helper.h"
#include "semaphore.h"

class ErizoAgent
{
  DECLARE_LOGGER();

  struct Erizo
  {
    int pid;
    std::string id;
    std::string room_id;
    operator Json::Value();
  };

public:
  ErizoAgent();
  ~ErizoAgent();

  int init();
  void close();
  void removeErizo(pid_t child_id);
private:
  void getErizoAgents(const Json::Value &root);
  void getErizo(const Json::Value &root);

  int bootErizoProcessManager();
  operator Json::Value();
  void stopHeartBeat();
  void startHeartBeat();
private:
  bool init_;
  std::string id_;
  int area_type_;
  std::shared_ptr<AMQPHelper> amqp_broadcast_;
  std::shared_ptr<AMQPHelper> amqp_uniquecast_;
  std::shared_ptr<RedisHelper> redis_;

  int manager_pid_;
  int pipe_[2];
  Semaphore sem_;

  std::map<std::string, Erizo> erizos_map_;
  sem_t heartbeat_exit_sem_;
  std::shared_ptr<std::thread> heartbeat_thread_;
};

#endif