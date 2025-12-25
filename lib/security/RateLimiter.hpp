#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>

struct RateLimitConfig {
  uint32_t maxQueriesPerWindow = 100;
  uint32_t windowSeconds = 1;
};

class RateLimiter {
private:
  std::mutex mtx;

  RateLimitConfig config;

  struct ClientRecord {
    std::deque<std::chrono::time_point<std::chrono::steady_clock>> queryTime;
    uint64_t totalQueries = 0;
    uint64_t rateLimitedQueries = 0;
    std::chrono::time_point<std::chrono::steady_clock> lastInteracted;
  };

  std::unordered_map<std::string, ClientRecord> clients;

  void cleanupClients();

  void
  cleanupOldEntries(ClientRecord &record,
                    std::chrono::time_point<std::chrono::steady_clock> now);

public:
  RateLimiter(RateLimitConfig cfg) : config(cfg) {}

  bool allowQuery(const std::string &clientIp);

  // stats
  size_t getClientCount() const;
  uint64_t getTotalRateLimited() const;
  void printStats() const;
};

inline void RateLimiter::cleanupClients() {
};

// TODO: Implement this
inline void RateLimiter::printStats() const {};

inline uint64_t RateLimiter::getTotalRateLimited() const {
  uint64_t totalRateLimitedQueries = 0;
  for (const auto &client : this->clients) {
    totalRateLimitedQueries += client.second.rateLimitedQueries;
  }

  return totalRateLimitedQueries;
};

inline size_t RateLimiter::getClientCount() const {
  return this->clients.size();
};

inline bool RateLimiter::allowQuery(const std::string &clientIp) {
  std::lock_guard<std::mutex> lock(this->mtx);

  auto now = std::chrono::steady_clock::now();

  // Get reference to client record (creates new one if doesn't exist)
  ClientRecord &record = this->clients[clientIp];

  // Remove queries outside the window
  this->cleanupOldEntries(record, now);

  if (record.queryTime.size() >= config.maxQueriesPerWindow) {
    record.rateLimitedQueries++;
    record.lastInteracted = now;
    return false; // limit
  }

  // allow
  record.queryTime.push_back(now);
  record.totalQueries++;
  record.lastInteracted = now;
  return true;
}

inline void RateLimiter::cleanupOldEntries(
    ClientRecord &record,
    std::chrono::time_point<std::chrono::steady_clock> now) {
  auto windowStart = now - std::chrono::seconds(this->config.windowSeconds);
  while (!record.queryTime.empty() && record.queryTime.front() < windowStart) {
    record.queryTime.pop_front();
  }
}

#endif // RATE_LIMITER_HPP
