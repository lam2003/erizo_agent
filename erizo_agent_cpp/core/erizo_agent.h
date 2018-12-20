#ifndef ERIZO_AGENT_H
#define ERIZO_AGENT_H

#include <condition_variable>
#include <mutex>

#include <logger.h>
#include <json/json.h>

#include "rabbitmq/amqp_helper.h"
#include "common/semaphore.h"

class ErizoAgent
{
  DECLARE_LOGGER();

  struct Erizo
  {
    int pid;
    std::string id;
    std::string room_id;
  };

public:
  ErizoAgent();
  ~ErizoAgent();

  int init();
  void close();

private:
  void getErizoAgents(const Json::Value &root);
  void getErizo(const Json::Value &root);

  int bootErizoProcessManager();

private:
  bool init_;
  std::string id_;
  std::shared_ptr<AMQPHelper> amqp_broadcast_;
  std::shared_ptr<AMQPHelper> amqp_uniquecast_;

  int manager_pid_;
  int pipe_[2];
  Semaphore sem_;

  std::map<std::string, Erizo> erizos_map_;
};

#endif