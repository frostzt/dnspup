#ifndef NETWORK_CONFIG_HPP
#define NETWORK_CONFIG_HPP

#include <cstdint>

class NetworkConfig {
public:
  // Socket config
  uint32_t recvTimeoutMs = 2000;
  uint32_t sendTimeoutMs = 1000;
  uint32_t connectTimeoutMs = 5000;

  // Retries
  int maxRetries = 3;
  uint32_t initialRetryDelayMs = 100;
  double backoffMultiplier = 2.0;

  NetworkConfig(uint32_t recvTimeoutMs_ = 2000, uint32_t sendTimeoutMs_ = 1000,
                uint32_t connectTimeoutMs_ = 5000, int maxRetries_ = 3,
                uint32_t initialRetryDelayMs_ = 100, double backoffMultiplier_ = 2.0)
      : // Socket
        recvTimeoutMs(recvTimeoutMs_), sendTimeoutMs(sendTimeoutMs_),
        connectTimeoutMs(connectTimeoutMs_),

        // Retries
        maxRetries(maxRetries_), initialRetryDelayMs(initialRetryDelayMs_),
        backoffMultiplier(backoffMultiplier_) {}
};

#endif // NETWORK_CONFIG_HPP
