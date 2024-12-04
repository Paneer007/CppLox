#ifndef cpplox_channel_h
#define cpplox_channel_h

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#include "value.hpp"

class ThreadChannel
{
  std::queue<Value> buffer;
  int mutex_id;
  bool closed;

public:
  ThreadChannel();
  ~ThreadChannel();
  void initChannel();

  void send(Value value);
  std::optional<Value> receive();

  void close();
};

#endif