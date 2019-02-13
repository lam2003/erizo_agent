#ifndef AMQP_CLI_H
#define AMQP_CLI_H

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "common/logger.h"

class AMQPCli
{
  DECLARE_LOGGER();

public:
  AMQPCli();
  ~AMQPCli();

  int init(const std::string &exchange, const std::string &type = "direct", const std::string &binding_key = "");
  void close();

  amqp_connection_state_t getConnection();
  const std::string &getReplyTo();

private:
  int checkError(amqp_rpc_reply_t x);
  std::string stringifyBytes(amqp_bytes_t bytes);

private:
  std::string reply_to_;
  amqp_connection_state_t conn_;
  bool init_;
};

#endif