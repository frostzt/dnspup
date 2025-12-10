#ifndef CORE_DNS_HPP
#define CORE_DNS_HPP

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "BytePacketBuffer.hpp"
#include "DnsPacket.hpp"
#include "DnsQuestion.hpp"
#include "QueryType.hpp"
#include "ResultCode.hpp"

inline DnsPacket lookup(std::string &qname, QueryType qtype) {
  // create udp socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // bind to local address
  struct sockaddr_in local_addr;
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port = htons(43210);
  if (bind(sockfd, (const struct sockaddr *)&local_addr, sizeof(local_addr)) <
      0) {
    close(sockfd);
    throw std::runtime_error("udp socket failed to bind");
  }

  // build a dns packet
  DnsPacket packet;
  packet.header.id = 7857;
  packet.header.questions = 1;
  packet.header.recursionDesired = true;
  packet.questions.push_back(DnsQuestion(qname, qtype));

  // write into buffer
  BytePacketBuffer reqBuffer;
  packet.write(reqBuffer);

  // create a server pointing to dns resolver
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(53);
  inet_pton(AF_INET, "8.8.8.8", &serverAddr.sin_addr);

  // send this to DNS resolver
  sendto(sockfd, reqBuffer.buf, reqBuffer.currentPosition(), 0,
         (struct sockaddr *)&serverAddr, sizeof(serverAddr));

  // receive response
  BytePacketBuffer resBuffer;
  recvfrom(sockfd, resBuffer.buf, 512, 0, nullptr, nullptr);

  // parse and print
  DnsPacket resPacket = DnsPacket::fromBuffer(resBuffer);

  close(sockfd);
  return resPacket;
}

inline void handleQuery(int sockfd) {
  // we receive a query
  BytePacketBuffer reqBuffer;
  struct sockaddr_in srcAddr;
  socklen_t srcAddrLen = sizeof(srcAddr);

  ssize_t bytesReceived = recvfrom(sockfd, reqBuffer.buf, 512, 0,
                                   (struct sockaddr *)&srcAddr, &srcAddrLen);
  if (bytesReceived < 0) {
    throw std::runtime_error("failed to receive packet");
  }

  // we parse request packet
  DnsPacket request = DnsPacket::fromBuffer(reqBuffer);

  // create response packet
  DnsPacket response;
  response.header.id = request.header.id;
  response.header.recursionDesired = true;
  response.header.recursionAvailable = true;
  response.header.response = true;

  // handle question
  if (!request.questions.empty()) {
    DnsQuestion question = request.questions.back();
    request.questions.pop_back();

    std::cout << "Received query: " << question << std::endl;

    // forward query and handle response
    try {
      DnsPacket result = lookup(question.name, question.qtype);

      response.questions.push_back(question);
      response.header.rescode = result.header.rescode;

      // answers
      for (const auto &rec : result.answers) {
        std::cout << "Answer: ";
        std::visit([](const auto &r) { std::cout << r << std::endl; }, rec);
        response.answers.push_back(rec);
      }

      // authorities
      for (const auto &rec : result.authorities) {
        std::cout << "Authority: ";
        std::visit([](const auto &r) { std::cout << r << std::endl; }, rec);
        response.authorities.push_back(rec);
      }

      // resources
      for (const auto &rec : result.resources) {
        std::cout << "Resource: ";
        std::visit([](const auto &r) { std::cout << r << std::endl; }, rec);
        response.resources.push_back(rec);
      }
    } catch (const std::exception &e) {
      std::cerr << "Lookup failed: " << e.what() << std::endl;
      response.header.rescode = ResultCode::SERVFAIL;
    }
  } else {
    response.header.rescode = ResultCode::FORMERR;
  }

  BytePacketBuffer resBuffer;
  response.write(resBuffer);

  ssize_t bytesSent = sendto(sockfd, resBuffer.buf, resBuffer.currentPosition(),
                             0, (struct sockaddr *)&srcAddr, sizeof(srcAddr));

  if (bytesSent < 0) {
    throw std::runtime_error("failed to send response");
  }
}

#endif // CORE_DNS_HPP
