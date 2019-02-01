#ifndef AMQP_HELPER_H
#define AMQP_HELPER_H

#include <memory>
#include <functional>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "common/logger.h"

class AMQPHelper
{
  DECLARE_LOGGER();

public:
  AMQPHelper();
  ~AMQPHelper();

  int init(const std::string &binding_key,
           const std::function<void(const std::string &)> &func);
  void close();
  int dispatch();
  int sendMessage(const std::string &queuename,
                  const std::string &binding_key,
                  const std::string &send_msg);

private:
  int checkError(amqp_rpc_reply_t x);

private:
  std::function<void(const std::string &)> func_;
  amqp_connection_state_t conn_;
  bool init_;
};

#endif