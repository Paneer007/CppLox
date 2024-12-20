#ifndef cpplox_channel_h
#define cpplox_channel_h

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#include "value.hpp"

template<typename T>
class Node
{
private:
public:
  T data;
  Node* next;
  Node(T value)
  {
    data = value;
    next = nullptr;
  }
};

template<typename T>
class Queue
{
private:
  Node<T>* front;
  Node<T>* rear;

public:
  Queue() { front = rear = nullptr; }
  ~Queue()
  {
    while (!isEmpty()) {
      dequeue();
    }
  }
  bool isEmpty() { return front == nullptr; }
  void enqueue(T value)
  {
    Node<T>* newNode = new Node<T>(value);
    if (rear == nullptr) {
      front = rear = newNode;
    } else {
      rear->next = newNode;
      rear = newNode;
    }
  }
  T dequeue()
  {
    if (isEmpty()) {
      return T();
    }
    Node<T>* temp = front;
    T value = temp->data;
    front = front->next;
    if (front == nullptr) {
      rear = nullptr;
    }
    delete temp;
    return value;
  }
};

class ThreadChannel
{
  Queue<Value> buffer;
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