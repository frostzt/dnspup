/**
 * Author: frostzt
 *
 * This file defines the main Cache interface and implementation
 **/

#ifndef DNS_CACHE_HPP
#define DNS_CACHE_HPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <list>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../QueryType.hpp"
#include "CacheEntry.hpp"
#include "CacheStats.hpp"

class DnsCache {
private:
  // reader-writer lock
  mutable std::shared_mutex mtx;

  // thread mgmt
  std::thread cleanupThread;
  std::atomic<bool> _thread__cleanup_running{false};

  // cleanup loop
  void _thread__cleanup();

  // Storage: map<cacheKey, vector<CacheEntry>>
  std::unordered_map<std::string, std::vector<CacheEntry>> cache;
  std::unordered_map<std::string, NSCacheEntry> nsCache;
  std::unordered_map<std::string, NegativeCacheEntry> negativeCache;

  // config
  uint32_t minTTL = 60;
  uint32_t maxTTL = 86400;

  // stats
  CacheStats stats;

  // Helper: generate cache key
  std::string makeCacheKey(const std::string &qname, QueryType qtype);

  // Helper: extract TTL from DnsRecord variant
  uint32_t extractTTL(const DnsRecord &record);

  // Helper: enforce TTL bounds
  uint32_t enforceTTLBounds(uint32_t ttl);

  // Helper: remove expired entries from a bucket
  void removeExpiredEntries(std::vector<CacheEntry> &entries);

  // -------- LRU TRACKING --------
  std::list<std::string> lruList; // dll
  std::unordered_map<std::string, std::list<std::string>::iterator>
      lruMap; // quick lookup
  size_t maxEntries = 10000;
  size_t maxNsEntries = 1000;

public:
  DnsCache(uint32_t minTTL_ = 60, uint32_t maxTTL_ = 86400)
      : minTTL(minTTL_), maxTTL(maxTTL_) {}

  // stop the thread
  ~DnsCache() { this->stopCleanup(); }

  // start the cleanup thread
  void startCleanup() {
    if (this->_thread__cleanup_running) {
      return;
    }

    this->_thread__cleanup_running = true;
    this->cleanupThread = std::thread(&DnsCache::_thread__cleanup, this);
    std::cout << "[Cache] Cleanup thread started" << std::endl;
  }

  // stop the cleanup thread
  void stopCleanup() {
    if (!this->_thread__cleanup_running) {
      return;
    }

    this->_thread__cleanup_running = false;
    if (this->cleanupThread.joinable()) {
      this->cleanupThread.join();
    }

    std::cout << "[Cache] Cleanup thread stopped" << std::endl;
  }

  // Lookup cache records
  std::optional<std::vector<DnsRecord>> lookup(const std::string &qname,
                                               QueryType qtype);
  std::optional<std::array<uint8_t, 4>> lookupNS(const std::string &domain);

  // Insert records into cache
  void insert(const std::string &qname, QueryType qtype,
              const std::vector<DnsRecord> &records);
  void insertNS(const std::string &domain, const std::array<uint8_t, 4> &ip,
                uint32_t ttl);
  void insertNegative(const std::string &qname, QueryType qtype,
                      ResultCode rescode, uint32_t ttl);

  // Manual cleanup
  void cleanupExpired();

  // Stats access
  const CacheStats &getStats() const;
  void printStats() const;

  // -------- LRU OPS --------
  void evictLRU();
  void updateLRU(const std::string &key);
};

inline void DnsCache::_thread__cleanup() {
  while (this->_thread__cleanup_running) {
    // sleep for 60 sec
    std::this_thread::sleep_for(std::chrono::seconds(60));

    if (!this->_thread__cleanup_running) {
      break;
    }

    // clean up expired entries
    this->cleanupExpired();

    std::cout << "[Cache] Cleanup Completed. "
              << "Current entries: " << this->cache.size() << std::endl;
  }
}

inline void DnsCache::updateLRU(const std::string &key) {
  auto node = this->lruMap.find(key);

  // Remove if the key exists
  if (node != this->lruMap.end()) {
    this->lruList.erase(node->second);
  }

  this->lruList.push_front(key);
  this->lruMap[key] = this->lruList.begin();
};

inline void DnsCache::evictLRU() {
  if (lruList.empty()) {
    return;
  }

  auto node = this->lruList.back();

  // remove from the cache map
  size_t numRecords = this->cache[node].size();
  this->cache.erase(node);
  this->lruList.pop_back();
  this->lruMap.erase(node);

  this->stats.evictions++;
  this->stats.currentEntries -= numRecords;
}

inline void DnsCache::removeExpiredEntries(std::vector<CacheEntry> &entries) {
  auto origSize = entries.size();

  entries.erase(
      std::remove_if(entries.begin(), entries.end(),
                     [](const CacheEntry &entry) { return entry.isExpired(); }),
      entries.end());

  size_t removed = origSize - entries.size();
  if (removed > 0) {
    stats.expirations += removed;
    stats.currentEntries -= removed;
  }
}

inline std::string DnsCache::makeCacheKey(const std::string &qname,
                                          QueryType qtype) {
  auto qtypenum = fromQueryTypeToNumber(qtype);

  std::string result;
  result.reserve(qname.size() + 1 + 10);

  for (char c : qname) {
    result += std::tolower(static_cast<unsigned char>(c));
  }

  result += ':';
  result += std::to_string(qtypenum);
  return result;
}

inline uint32_t DnsCache::enforceTTLBounds(uint32_t ttl) {
  if (ttl == 0)
    return 0;
  if (ttl < minTTL)
    return minTTL;
  if (ttl > maxTTL)
    return maxTTL;
  return ttl;
}

inline uint32_t DnsCache::extractTTL(const DnsRecord &record) {
  return std::visit([](const auto &r) { return r.ttl; }, record);
}

inline std::optional<std::vector<DnsRecord>>
DnsCache::lookup(const std::string &qname, QueryType qtype) {
  std::shared_lock<std::shared_mutex> lock(this->mtx);

  const std::string key = this->makeCacheKey(qname, qtype);

  // perform lookup on -ve cache
  auto negIt = this->negativeCache.find(key);
  if (negIt != this->negativeCache.end()) {
    if (!negIt->second.isExpired()) {
      this->stats.negHits++;
      return std::vector<DnsRecord>{}; // empty to signal cached negative
                                       // response
    }
    negativeCache.erase(negIt);
  }

  // perform lookup on cache
  auto it = this->cache.find(key);
  if (it == this->cache.end()) {
    // cache miss
    stats.misses++;
    return std::nullopt;
  }

  // remove expired entires
  this->removeExpiredEntries(it->second);

  // if all entries expired remove the bucket (key)
  if (it->second.empty()) {
    this->cache.erase(it);
    stats.misses++;
    return std::nullopt;
  }

  // cache hit!
  // update lru
  this->updateLRU(key);
  std::vector<DnsRecord> records;
  for (auto &entry : it->second) {
    entry.hitCount++;

    DnsRecord recordCopy = entry.record;
    uint32_t remainingTTL = entry.remainingTTL();

    std::visit([remainingTTL](auto &r) { r.ttl = remainingTTL; }, recordCopy);

    records.push_back(recordCopy);
  }

  stats.hits++;
  return records;
}

inline std::optional<std::array<uint8_t, 4>>
DnsCache::lookupNS(const std::string &domain) {
  std::shared_lock<std::shared_mutex> lock(this->mtx);

  auto it = this->nsCache.find(domain);
  if (it == this->nsCache.end()) {
    // ns cache miss
    this->stats.nsMisses++;
    return std::nullopt;
  }

  NSCacheEntry &entry = it->second;

  // check if expired
  if (entry.isExpired()) {
    this->nsCache.erase(it);
    this->stats.nsMisses++;
    return std::nullopt;
  }

  // NS cache hit
  entry.hitCount++;
  this->stats.nsHits++;
  return entry.ip;
}

inline void DnsCache::insert(const std::string &qname, QueryType qtype,
                             const std::vector<DnsRecord> &records) {
  std::unique_lock<std::shared_mutex> lock(this->mtx);

  if (records.empty()) {
    return;
  }

  std::string key = this->makeCacheKey(qname, qtype);
  auto now = std::chrono::steady_clock::now();

  std::vector<CacheEntry> entries;
  for (const auto &record : records) {
    uint32_t ttl = this->extractTTL(record);
    uint32_t enforcedTTL = this->enforceTTLBounds(ttl);

    // skip if ttl is 0
    if (enforcedTTL == 0) {
      continue;
    }

    CacheEntry entry;
    entry.record = record;
    entry.insertedAt = now;
    entry.expiresAt = now + std::chrono::seconds(enforcedTTL);
    entry.originalTTL = enforcedTTL;
    entry.hitCount = 0;

    entries.push_back(entry);
  }

  while (this->cache.size() >= this->maxEntries && !this->cache.empty()) {
    evictLRU();
  }

  if (!entries.empty()) {
    cache[key] = std::move(entries);
    stats.inserts++;
    stats.currentEntries += entries.size();
    this->updateLRU(key);
  }
}

inline void DnsCache::insertNS(const std::string &domain,
                               const std::array<uint8_t, 4> &ip, uint32_t ttl) {
  std::unique_lock<std::shared_mutex> lock(this->mtx);

  uint32_t enforcedTTL = this->enforceTTLBounds(ttl);
  if (enforcedTTL == 0) {
    return;
  }

  auto now = std::chrono::steady_clock::now();

  NSCacheEntry entry;
  entry.ip = ip;
  entry.insertedAt = now;
  entry.expiresAt = now + std::chrono::seconds(enforcedTTL);
  entry.originalTTL = enforcedTTL;
  entry.hitCount = 0;

  if (this->nsCache.size() >= this->maxNsEntries) {
    return;
  }

  nsCache[domain] = entry;
  this->stats.nsInserts++;
}

inline void DnsCache::insertNegative(const std::string &qname, QueryType qtype,
                                     ResultCode rescode, uint32_t ttl) {
  std::unique_lock<std::shared_mutex> lock(this->mtx);

  uint32_t enforcedTTL = std::min(ttl, 600u);
  enforcedTTL = std::max(enforcedTTL, 60u);

  std::string key = this->makeCacheKey(qname, qtype);
  auto now = std::chrono::steady_clock::now();

  NegativeCacheEntry entry;
  entry.resCode = rescode;
  entry.insertedAt = now;
  entry.expiresAt = now + std::chrono::seconds(enforcedTTL);
  entry.originalTTL = enforcedTTL;
  entry.hitCount = 0;

  negativeCache[key] = entry;
  this->stats.negInserts++;
}

inline void DnsCache::cleanupExpired() {
  std::unique_lock<std::shared_mutex> lock(this->mtx);

  for (auto it = cache.begin(); it != cache.end();) {
    this->removeExpiredEntries(it->second);
    if (it->second.empty()) {
      auto node = this->lruMap.find(it->first);

      // Remove if the key exists
      if (node != this->lruMap.end()) {
        this->lruList.erase(node->second);
      }

      this->lruMap.erase(it->first);
      it = cache.erase(it);
    } else {
      ++it;
    }
  }

  // Cleanup NS Cache
  for (auto it = nsCache.begin(); it != nsCache.end();) {
    if (it->second.isExpired()) {
      it = nsCache.erase(it);
    } else {
      ++it;
    }
  }

  // Cleanup -tive Cache
  for (auto it = negativeCache.begin(); it != negativeCache.end();) {
    if (it->second.isExpired()) {
      it = negativeCache.erase(it);
    } else {
      ++it;
    }
  }
}

inline const CacheStats &DnsCache::getStats() const {
  std::shared_lock<std::shared_mutex> lock(this->mtx);
  return this->stats;
}

inline void DnsCache::printStats() const { stats.print(); }

#endif // DNS_CACHE_HPP
