#ifndef SECURITY_UTILS_HPP
#define SECURITY_UTILS_HPP

#include <cstdint>
#include <random>
#include <stdexcept>

#include "../tracking/TransactionTracker.hpp"

class SecurityUtils {
private:
  static size_t collisions;
  static std::mt19937 gen;
  static std::random_device rd;
  static std::uniform_int_distribution<uint16_t> dist;

public:
  static uint16_t generateTransactionId(TransactionTracker &tracker) {
    for (int attempt = 0; attempt < 5; attempt++) {
      uint16_t id = dist(gen);
      if (!tracker.checkTxnId(id)) {
        return id;
      }
      collisions++;
    }
    throw std::runtime_error("too many txnId collisions occurred");
  }
};

inline size_t SecurityUtils::collisions = 0;
inline std::random_device SecurityUtils::rd;
inline std::mt19937 SecurityUtils::gen(rd());
inline std::uniform_int_distribution<uint16_t> SecurityUtils::dist(1, 65535);

#endif // SECURITY_UTILS_HPP
