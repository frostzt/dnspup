#ifndef RETRY_POLICY_HPP
#define RETRY_POLICY_HPP

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "config/NetworkConfig.hpp"
#include "errors/errors.hpp"

class RetryPolicy {
private:
  NetworkConfig networkConfig;

public:
  RetryPolicy(const NetworkConfig& networkConfig_) : networkConfig(networkConfig_) {}

  template <typename Func>
  auto executeWithRetry(Func func) -> decltype(func()) {
    auto delayMs = this->networkConfig.initialRetryDelayMs;

    for (auto attempt = 0; attempt < this->networkConfig.maxRetries;
         attempt++) {
      try {
        return func();
      } catch (const TimeoutException &e) {
        if (attempt == this->networkConfig.maxRetries - 1) {
          throw; // last attempt failed
        }

        std::cerr << "Attempt " << (attempt + 1) << " timed out, retrying in "
                  << delayMs << "ms..." << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
	delayMs *= this->networkConfig.backoffMultiplier;
      }
    }

    throw std::runtime_error("all retry attempts failed");
  }
};

#endif // RETRY_POLICY_HPP
