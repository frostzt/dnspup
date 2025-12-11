#include <arpa/inet.h>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Core.hpp"

int main() {
  try {
    // create cache
    DnsCache cache(60, 86400);

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

    std::cout << "DNS Server listening on 0.0.0.0:2053" << std::endl;

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

    close(sockfd);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
