#include <arpa/inet.h>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "BytePacketBuffer.hpp"
#include "DnsPacket.hpp"
#include "DnsQuestion.hpp"

int main() {
  try {
    auto qname = "fofofoofof.com";
    auto qtype = A{};

    // create udp socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // bind to local address
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(43210);
    if (bind(sockfd, (const struct sockaddr *)&local_addr, sizeof(local_addr)) <
        0) {
      std::cerr << "bind failed";
      return 1;
    }

    // build DNS Packet
    DnsPacket packet;
    packet.header.id = 9475;
    packet.header.questions = 1;
    packet.header.recursionDesired = true;
    packet.questions.push_back(DnsQuestion(qname, qtype));

    // write this packet to buffer
    BytePacketBuffer reqBuffer;
    packet.write(reqBuffer);

    // Send this to DNS Server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &server_addr.sin_addr);

    // send this data
    sendto(sockfd, reqBuffer.buf, reqBuffer.currentPosition(), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    // receive response
    BytePacketBuffer resBuffer;
    recvfrom(sockfd, resBuffer.buf, 512, 0, nullptr, nullptr);

    // parse and print
    DnsPacket resPacket = DnsPacket::fromBuffer(resBuffer);

    std::cout << resPacket;

    close(sockfd);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
