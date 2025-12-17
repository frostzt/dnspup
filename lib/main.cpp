#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Core.hpp"
#include "cache/StatsLogger.hpp"
#include "config/NetworkConfig.hpp"
#include "tracking/TransactionTracker.hpp"

std::atomic<bool> g_shutdown_requested{false};

void signalHandler(int signum) {
  std::cout << "\nShutdown signal received (" << signum << ")" << std::endl;
  g_shutdown_requested = true;
}

int main() {
  try {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // create network config
    NetworkConfig networkConfig;

    // bind udp socket to 2053
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
      std::cerr << "Failed to create socket" << std::endl;
      return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(2053);

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) <
        0) {
      std::cerr << "Failed to bind socket" << std::endl;
      close(sockfd);
      return 1;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // Set socket recieve timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      std::cerr << "Warning: Failed to set socket timeout" << std::endl;
    }

    // create cache
    DnsCache cache(60, 86400); // runs every minute
    cache.startCleanup();

    StatsLogger cacheStatsLogger(120, cache); // runs every 2 mins
    cacheStatsLogger.startLogger();

    // transaction tracker
    TransactionTracker tracker;

    std::cout << "DNS Server listening on 0.0.0.0:2053" << std::endl;
    std::cout << "Background threads started" << std::endl;
    std::cout << "Press Ctrl+C to shutdown" << std::endl;

    // handle queries
    while (!g_shutdown_requested) {
      try {
        handleQuery(sockfd, cache, networkConfig, tracker);
      } catch (const std::exception &e) {
        std::cerr << "An exception occured: " << e.what() << std::endl;
      }
    }

    // stats
    std::cout << "\nShutting down...\n";
    cache.printStats();

    // cleanup
    cache.stopCleanup();
    cacheStatsLogger.stopLogger();

    close(sockfd);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
