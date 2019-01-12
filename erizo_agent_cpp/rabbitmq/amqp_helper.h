#ifndef AMQP_HELPER_H
#define AMQP_HELPER_H

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <logger.h>

#include <memory>
#include <functional>

class AMQPHelper
{
  DECLARE_LOGGER();

public:
  ~AMQPHelper();
  static AMQPHelper *getInstance();
  int init(const std::string &exchange,
           const std::string &binding_key,
           const std::function<void(const std::string &)> &func);
  int dispatch();
  int sendMessage(const std::string &exchange,
                  const std::string &queuename,
                  const std::string &binding_key,
                  const std::string &send_msg);
  void close();

private:
  AMQPHelper();
  int checkError(amqp_rpc_reply_t x);

private:
  std::function<void(const std::string &)> func_;
  amqp_connection_state_t conn_;
  bool init_;

  static AMQPHelper *instance_;
};

#endif