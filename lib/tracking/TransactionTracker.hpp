#ifndef TRANSACTION_TRACKER_HPP
#define TRANSACTION_TRACKER_HPP

#include <chrono>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../QueryType.hpp"
#include "../common/ServerConfig.hpp"

struct Transaction {
  uint16_t id;
  std::string qname;
  QueryType qtype;
  Server server;
  std::chrono::time_point<std::chrono::steady_clock> sentAt;

  bool isExpired(uint32_t timeoutMs) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - sentAt);
    return elapsed.count() > timeoutMs;
  }
};

class TransactionTracker {
private:
  std::unordered_map<uint16_t, Transaction> inFlight;
  mutable std::mutex mtx;

public:
  uint16_t registerTxn(uint16_t id, const std::string &qname, QueryType qtype,
                       const Server &server);
  bool checkTxnId(uint16_t) const;
  void removeTxn(uint16_t id);
  void cleanup(uint32_t timeoutMs);
};

inline bool TransactionTracker::checkTxnId(uint16_t txnId) const {
  std::lock_guard<std::mutex> lock(this->mtx);
  return this->inFlight.contains(txnId);
}

inline uint16_t TransactionTracker::registerTxn(uint16_t id,
                                                const std::string &qname,
                                                QueryType qtype,
                                                const Server &server) {
  std::unique_lock<std::mutex> lock(this->mtx);

  // construct txn
  Transaction thisTxn;
  thisTxn.id = id;
  thisTxn.qname = qname;
  thisTxn.qtype = qtype;
  thisTxn.server = server;
  thisTxn.sentAt = std::chrono::steady_clock::now();

  // insert the txn
  this->inFlight[id] = thisTxn;
  return thisTxn.id;
}

inline void TransactionTracker::removeTxn(uint16_t id) {
  std::unique_lock<std::mutex> lock(this->mtx);
  if (this->inFlight.contains(id)) {
    this->inFlight.erase(id);
  }
}

inline void TransactionTracker::cleanup(uint32_t timeoutMs) {
  std::unique_lock<std::mutex> lock(this->mtx);
  for (auto it = this->inFlight.begin(); it != this->inFlight.end();) {
    if (it->second.isExpired(timeoutMs)) {
      it = this->inFlight.erase(it);
    } else {
      ++it;
    }
  }
}

#endif // TRANSACTION_TRACKER_HPP
