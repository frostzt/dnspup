/**
 * Author: frostzt
 *
 * This file contains cache statistics tracking
 **/

#ifndef CACHE_STATS_HPP
#define CACHE_STATS_HPP

#include <cstdint>
#include <iostream>

/**
 * CacheStats - Tracks cache performance metrics
 *
 * This is a simple data structure that tracks hit/miss rates,
 * insertions, evictions, etc. Can be easily extended for monitoring.
 **/
struct CacheStats {
  uint64_t hits = 0;
  uint64_t misses = 0;
  uint64_t inserts = 0;
  uint64_t evictions = 0;
  uint64_t expirations = 0;

  // ns cache stats
  uint64_t nsHits = 0;
  uint64_t nsMisses = 0;
  uint64_t nsInserts = 0;

  // -ve cache
  uint64_t negHits = 0;
  uint64_t negMisses = 0;
  uint64_t negInserts = 0;

  size_t currentEntries = 0;
  size_t maxEntries = 0;

  /**
   * Calculate cache hit rate as a percentage
   **/
  double hitRate() const;

  /**
   * Calculate ns cache hit rate as a percentage
   **/
  double nsHitRate() const;

  /**
   * Print cache statistics to stdout
   **/
  void print() const;

  /**
   * Reset all statistics to zero
   **/
  void reset();
};

inline double CacheStats::hitRate() const {
  uint64_t total = hits + misses;
  if (total == 0)
    return 0.0;
  return (static_cast<double>(hits) / total) * 100.0;
}

inline double CacheStats::nsHitRate() const {
  uint64_t total = nsHits + nsMisses;
  if (total == 0)
    return 0.0;
  return (static_cast<double>(nsHits) / total) * 100.0;
}

inline void CacheStats::reset() {
  hits = 0;
  misses = 0;
  inserts = 0;
  evictions = 0;
  expirations = 0;
  currentEntries = 0;
}

inline void CacheStats::print() const {
  std::cout << "\n=== Cache Statistics ===\n";
  std::cout << "Hits: " << hits << "\n";
  std::cout << "Misses: " << misses << "\n";
  std::cout << "Hit Rate: " << hitRate() << "%\n";
  std::cout << "Inserts: " << inserts << "\n";
  std::cout << "Evictions: " << evictions << "\n";
  std::cout << "Expirations: " << expirations << "\n";
  std::cout << "Current Entries: " << currentEntries << "\n";
  std::cout << "Max Entries: " << maxEntries << "\n";
  std::cout << "\n---- NS Cache --\n";
  std::cout << "NS Hits: " << nsHits << "\n";
  std::cout << "NS Misses: " << nsMisses << "\n";
  std::cout << "NS Inserts: " << nsInserts << "\n";
  std::cout << "NS Hit Rate: " << nsHitRate() << "%\n";
  std::cout << "========================\n\n";
}

#endif // CACHE_STATS_HPP
