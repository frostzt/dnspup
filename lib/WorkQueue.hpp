#ifndef DNSPUP_WORKQUEUE_HPP
#define DNSPUP_WORKQUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T> class ThreadSafeQueue {
private:
  mutable std::mutex mtx;
  std::condition_variable cv;

  std::queue<T> queue;
  bool shutdown = false;

public:
  void push(T item) {
    {
      std::lock_guard<std::mutex> lock(this->mtx);
      queue.push(std::move(item));
    }

    cv.notify_one();
  }

  bool pop(T &item) {
    std::unique_lock<std::mutex> lock(this->mtx);
    cv.wait(lock, [this] { return !queue.empty() || shutdown; });

    if (shutdown && queue.empty()) {
      return false;
    }

    item = std::move(queue.front());
    queue.pop();
    return true;
  }

  void shutdownQueue() {
    {
      std::lock_guard<std::mutex> lock(this->mtx);
      shutdown = true;
    }

    cv.notify_all();
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(this->mtx);
    return queue.size();
  }
};

#endif // DNSPUP_WORKQUEUE_HPP
