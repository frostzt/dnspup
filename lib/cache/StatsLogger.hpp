#ifndef STATS_LOGGER_HPP
#define STATS_LOGGER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "DnsCache.hpp"

class StatsLogger {
private:
  // thread mgmt
  std::jthread loggerThread;
  std::atomic<bool> _thread__running{false};
  std::condition_variable cv;
  std::mutex cvMtx;

  DnsCache &dnsCache;
  size_t interval;

  void printStats();

public:
  explicit StatsLogger(size_t interval_, DnsCache &cache)
      : dnsCache(cache), interval(interval_) {};

  ~StatsLogger() { stopLogger(); }

  void startLogger() {
    if (this->_thread__running) {
      return;
    }

    this->_thread__running = true;
    this->loggerThread = std::jthread(&StatsLogger::printStats, this);
    std::cout << "[StatsLogger] Stats Logger thread started" << std::endl;
  }

  void stopLogger() {
    if (!this->_thread__running) {
      return;
    }

    this->_thread__running = false;
    cv.notify_all();
    if (this->loggerThread.joinable()) {
      this->loggerThread.join();
    }

    std::cout << "[StatsLogger] Stats Logger thread stopped" << std::endl;
  }
};

inline void StatsLogger::printStats() {
  while (this->_thread__running) {
    std::unique_lock<std::mutex> lock(cvMtx);
    cv.wait_for(lock, std::chrono::seconds(this->interval),
                [this]() { return !this->_thread__running; });

    if (!this->_thread__running) {
      break;
    }

    this->dnsCache.printStats();
  }
}

#endif // STATS_LOGGER_HPP
