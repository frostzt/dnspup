#ifndef THREAD_SAFE_CACHE_HPP
#define THREAD_SAFE_CACHE_HPP

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

#include "DnsCache.hpp"

class ThreadSafeCache {
private:
  DnsCache cache;
  mutable std::shared_mutex mtx;

public:
  // stop the thread
  ~ThreadSafeCache() { this->stopCleanup(); }

  std::optional<std::vector<DnsRecord>> lookup(const std::string &qname,
                                               QueryType qtype) {
    std::shared_lock<std::shared_mutex> lock(this->mtx);
    return this->cache.lookup(qname, qtype);
  }

  std::optional<std::array<uint8_t, 4>> lookupNS(const std::string &domain) {
    std::shared_lock<std::shared_mutex> lock(this->mtx);
    return this->cache.lookupNS(domain);
  }

  void insert(const std::string &qname, QueryType qtype,
              const std::vector<DnsRecord> &records) {
    std::unique_lock<std::shared_mutex> lock(this->mtx);
    this->cache.insert(qname, qtype, records);
  }

  void insertNS(const std::string &domain, const std::array<uint8_t, 4> &ip,
                uint32_t ttl) {
    std::unique_lock<std::shared_mutex> lock(this->mtx);
    this->cache.insertNS(domain, ip, ttl);
  }

  void insertNegative(const std::string &qname, QueryType qtype,
                      ResultCode rescode, uint32_t ttl) {
    std::unique_lock<std::shared_mutex> lock(this->mtx);
    this->cache.insertNegative(qname, qtype, rescode, ttl);
  }

  void startCleanup() { this->cache.startCleanup(); }

  void stopCleanup() { this->cache.stopCleanup(); }

  void printStats() { this->cache.printStats(); }
};

#endif // THREAD_SAFE_CACHE_HPP
