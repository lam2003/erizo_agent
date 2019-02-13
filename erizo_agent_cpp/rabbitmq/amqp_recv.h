#ifndef AMQP_RECV_H
#define AMQP_RECV_H

#include <memory>
#include <functional>

#include "common/logger.h"

class AMQPCli;

class AMQPRecv
{
  DECLARE_LOGGER();

public:
  AMQPRecv();
  ~AMQPRecv();

  int init(const std::string &binding_key,
           const std::function<void(const std::string &)> &func);
  void close();
  int dispatch();
  int sendMessage(const std::string &queuename,
                  const std::string &binding_key,
                  const std::string &send_msg);

private:
  std::function<void(const std::string &)> func_;
  std::unique_ptr<AMQPCli> amqp_cli_;
  bool init_;
};

#endif