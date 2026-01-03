#ifndef DNSPUP_THREAD_POOL_HPP
#define DNSPUP_THREAD_POOL_HPP

#include <atomic>
#include <exception>
#include <iostream>
#include <thread>
#include <vector>

#include "WorkQueue.hpp"

class ThreadPool {
private:
  std::vector<std::jthread> workers;
  ThreadSafeQueue<std::function<void()>> workQueue;
  std::atomic<bool> __is_thread_running__{true};
  std::atomic<uint64_t> currentActiveTasks{0};

  void workerThread() {
    while (this->__is_thread_running__) {
      std::function<void()> task;
      if (workQueue.pop(task)) {
        currentActiveTasks++;
        try {
          task();
        } catch (const std::exception &e) {
          std::cerr << "Task exception: " << e.what() << std::endl;
        }
        currentActiveTasks--;
      }
    }
  }

public:
  ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; i++) {
      workers.emplace_back(&ThreadPool::workerThread, this);
    }
  }

  ~ThreadPool() { shutdown(); }

  template <typename Func> void enqueue(Func &&task) {
    this->workQueue.push(std::forward<Func>(task));
  }

  void shutdown() {
    this->__is_thread_running__ = false;
    workQueue.shutdownQueue();

    for (auto &worker : this->workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  size_t queueSize() const { return this->workQueue.size(); }

  uint64_t activeTasks() const {
    return this->currentActiveTasks.load();
  }
};

#endif // DNSPUP_THREAD_POOL_HPP
