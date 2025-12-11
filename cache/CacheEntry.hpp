/**
 * Author: frostzt
 *
 * This file contains the the Cache Entry structure with the TTL
 **/

#ifndef CACHE_ENTRY_HPP
#define CACHE_ENTRY_HPP

#include <chrono>
#include <iostream>

#include "../DnsRecord.hpp"
#include "../ResultCode.hpp"

/**
 * CacheEntry represents a cache entry that supports all the general
 * querytype ie. A, AAAA, CNAME and other set of records supported by
 * Dnspup
 **/
struct CacheEntry {
  // the actual dns record
  DnsRecord record;

  std::chrono::time_point<std::chrono::steady_clock> insertedAt;
  std::chrono::time_point<std::chrono::steady_clock> expiresAt;
  uint32_t originalTTL;
  uint32_t hitCount;

  /**
   * Returns true if `this` cache entry is expired
   **/
  bool isExpired() const;

  /**
   * Returns the remaining TTL for this entry
   **/
  uint32_t remainingTTL() const;

  friend std::ostream &operator<<(std::ostream &stream, const CacheEntry ce) {
    std::cout << "[[CacheEntry]]\n" << "\tTTL: " << ce.remainingTTL() << "\n";
    return stream;
  }
};

inline bool CacheEntry::isExpired() const {
  auto now = std::chrono::steady_clock::now();
  return now >= this->expiresAt;
}

inline uint32_t CacheEntry::remainingTTL() const {
  auto now = std::chrono::steady_clock::now();
  if (now >= this->expiresAt) {
    return 0;
  }

  auto remaining = this->expiresAt - now;

  uint32_t ttlSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(remaining).count();
  return ttlSeconds;
}

/**
 * NSCacheEntry represents a cache entry that stores all the Nameserver queries
 * and the data we recieve while we perform nameserver lookups
 **/
struct NSCacheEntry {
  // store ip address
  std::array<uint8_t, 4> ip;

  std::chrono::time_point<std::chrono::steady_clock> insertedAt;
  std::chrono::time_point<std::chrono::steady_clock> expiresAt;
  uint32_t originalTTL;
  uint32_t hitCount;

  /**
   * Returns true if `this` cache entry is expired
   **/
  bool isExpired() const;

  /**
   * Returns the remaining TTL for this entry
   **/
  uint32_t remainingTTL() const;
};

inline bool NSCacheEntry::isExpired() const {
  auto now = std::chrono::steady_clock::now();
  return now >= this->expiresAt;
}

inline uint32_t NSCacheEntry::remainingTTL() const {
  auto now = std::chrono::steady_clock::now();
  if (now >= this->expiresAt) {
    return 0;
  }

  auto remaining = this->expiresAt - now;

  uint32_t ttlSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(remaining).count();
  return ttlSeconds;
}

/**
 * NegativeCacheEntry represents a cache entry that stores all the queries
 * that failed to resolve due to either a NXDOMAIN or SERVFAIL
 **/
struct NegativeCacheEntry {
  // store ip address
  ResultCode resCode;

  std::chrono::time_point<std::chrono::steady_clock> insertedAt;
  std::chrono::time_point<std::chrono::steady_clock> expiresAt;
  uint32_t originalTTL;
  uint32_t hitCount;

  /**
   * Returns true if `this` cache entry is expired
   **/
  bool isExpired() const;
};

inline bool NegativeCacheEntry::isExpired() const {
  auto now = std::chrono::steady_clock::now();
  return now >= this->expiresAt;
}

#endif // CACHE_ENTRY_HPP
