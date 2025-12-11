#include <arpa/inet.h>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Core.hpp"
#include "cache/StatsLogger.hpp"

int main() {
  try {
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

    // create cache
    DnsCache cache(60, 86400); // runs every minute
    cache.startCleanup();

    StatsLogger cacheStatsLogger(120, cache); // runs every 2 mins
    cacheStatsLogger.startLogger();

    std::cout << "DNS Server listening on 0.0.0.0:2053" << std::endl;
    std::cout << "Background threads started" << std::endl;
    std::cout << "Press Ctrl+C to shutdown" << std::endl;

    // handle queries
    while (true) {
      try {
        handleQuery(sockfd, cache);
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
