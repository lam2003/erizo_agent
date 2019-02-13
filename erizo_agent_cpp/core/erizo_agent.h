#ifndef ERIZO_AGENT_H
#define ERIZO_AGENT_H

#include <string>
#include <memory>
#include <map>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <json/json.h>

#include "model/erizo.h"
#include "common/logger.h"

class AMQPRecv;
class Erizo;

class ErizoAgent
{
  DECLARE_LOGGER();

public:
  ~ErizoAgent();
  static ErizoAgent *getInstance();

  int init();
  void close();
  int dispatch();

  const std::string &getId() const
  {
    return id_;
  }
  std::string toJSON() const
  {
    Json::Value root;
    root["id"] = id_;
    root["last_update"] = last_update_;
    root["erizo_process_num"] = pid_erizo_mapping_.size();
    Json::FastWriter writer;
    return writer.write(root);
  }

private:
  ErizoAgent();

  void checkIfSubProcessQuit();
  int newErizoProcess();
  void notifyErizoProcessQuit(const Erizo &erizo);

  Json::Value getErizo(const Json::Value &root);

private:
  std::map<pid_t, Erizo> pid_erizo_mapping_;
  std::map<std::string, Erizo> roomid_erizo_mapping_;

  std::string id_;
  uint64_t last_update_;
  int idle_process_num_;
  std::shared_ptr<AMQPRecv> amqp_uniquecast_;
  bool init_;

  static ErizoAgent *instance_;
};

#endif