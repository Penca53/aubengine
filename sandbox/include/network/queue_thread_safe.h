#pragma once

#include <deque>
#include <mutex>

// Queue
template <typename T>
class QueueThreadSafe {
 public:
  QueueThreadSafe() = default;
  QueueThreadSafe(const QueueThreadSafe<T>&) = delete;
  ~QueueThreadSafe() { Clear(); }

 public:
  // Returns and maintains item at front of Queue
  const T& Front() {
    std::scoped_lock lock(_muxQueue);
    return _deqQueue.front();
  }

  // Returns and maintains item at back of Queue
  const T& Back() {
    std::scoped_lock lock(_muxQueue);
    return _deqQueue.back();
  }

  // Removes and returns item from front of Queue
  T PopFront() {
    std::scoped_lock lock(_muxQueue);
    auto t = std::move(_deqQueue.front());
    _deqQueue.pop_front();
    return t;
  }

  // Removes and returns item from back of Queue
  T PopBack() {
    std::scoped_lock lock(_muxQueue);
    auto t = std::move(_deqQueue.back());
    _deqQueue.pop_back();
    return t;
  }

  // Adds an item to back of Queue
  void PushBack(const T& item) {
    std::scoped_lock lock(_muxQueue);
    _deqQueue.emplace_back(std::move(item));
    _cvBlocking.notify_one();
  }

  // Adds an item to front of Queue
  void PushFront(const T& item) {
    std::scoped_lock lock(_muxQueue);
    _deqQueue.emplace_front(std::move(item));
    _cvBlocking.notify_one();
  }

  // Returns true if Queue has no items
  bool Empty() {
    std::scoped_lock lock(_muxQueue);
    return _deqQueue.empty();
  }

  // Returns number of items in Queue
  size_t Size() {
    std::scoped_lock lock(_muxQueue);
    return _deqQueue.size();
  }

  // Clears Queue
  void Clear() {
    std::scoped_lock lock(_muxQueue);
    _deqQueue.clear();
  }

  void Wait() {
    std::unique_lock<std::mutex> ul(_muxQueue);
    while (_deqQueue.empty()) {
      _cvBlocking.wait(ul);
    }
  }

 private:
  std::mutex _muxQueue;
  std::deque<T> _deqQueue;
  std::condition_variable _cvBlocking;
};