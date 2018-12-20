#ifndef AMQP_HELPER_H
#define AMQP_HELPER_H

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <logger.h>

#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>

struct AMQPData
{
  std::string exchange;
  std::string queuename;
  std::string binding_key;
  std::string msg;
};

class AMQPHelper
{
  DECLARE_LOGGER();

public:
  AMQPHelper();
  ~AMQPHelper();

  int init(const std::string &exchange, const std::string &binding_key, const std::function<void(const std::string &msg)> &func);
  void close();
  void addCallback(const AMQPData &data);

private:
  int checkError(amqp_rpc_reply_t x);
  int callback(const std::string &exchange, const std::string &queuename, const std::string &binding_key, const std::string &send_msg);

private:
  bool init_;
  std::atomic<bool> run_;
  amqp_connection_state_t conn_;
  std::unique_ptr<std::thread> recv_thread_;
  std::unique_ptr<std::thread> send_thread_;

  std::mutex mux_;
  std::condition_variable cond_;
  std::queue<AMQPData> queue_;
};

#endif