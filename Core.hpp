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
#include "StringUtils.hpp"
#include "cache/DnsCache.hpp"

struct Server {
  std::array<uint8_t, 4> s_addr;
  uint16_t s_port;
};

inline DnsPacket lookup(std::string &qname, QueryType qtype,
                        Server serverConf) {
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
  serverAddr.sin_port = htons(serverConf.s_port);
  serverAddr.sin_addr.s_addr =
      htonl((static_cast<uint32_t>(serverConf.s_addr[0]) << 24) |
            (static_cast<uint32_t>(serverConf.s_addr[1]) << 16) |
            (static_cast<uint32_t>(serverConf.s_addr[2]) << 8) |
            static_cast<uint32_t>(serverConf.s_addr[3]));

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

inline DnsPacket recursiveLookup(std::string &qname, QueryType qtype,
                                 DnsCache &cache) {
  // check main cache
  auto cached = cache.lookup(qname, qtype);
  if (cached.has_value()) {
    std::cout << "Cache HIT: " << qname << std::endl;
    DnsPacket response;

    if (cached->empty()) {
      response.header.rescode = ResultCode::NXDOMAIN;
    } else {
      response.answers = *cached;
      response.header.rescode = ResultCode::NOERROR;
    }

    return response;
  }

  std::cout << "Cache MISS: " << qname << std::endl;

  // try to find cached ns for this domain
  std::optional<std::array<uint8_t, 4>> ns = std::nullopt;
  std::string domain = qname;
  while (true) {
    ns = cache.lookupNS(domain);
    if (ns.has_value()) {
      std::cout << "NS Cache HIT for domain " << domain << " -> "
                << stringutils::ipv4ToString(*ns) << std::endl;
      break;
    }

    // move to parent domain
    size_t dot = domain.find('.');
    if (dot == std::string::npos) {
      break;
    }
    domain = domain.substr(dot + 1);
  }

  // if no cached ns found start from root
  if (!ns.has_value()) {
    // we'll always start with *a.root-servers.net*
    ns = stringutils::parseIpv4("198.41.0.4");
    std::cout << "Starting from root server" << std::endl;
  }

  while (true) {
    std::cout << "attempting lookup of " << fromQueryTypeToNumber(qtype) << " "
              << qname << " with ns " << stringutils::ipv4ToString(*ns) << "\n";

    auto nscopy = *ns;

    Server server{nscopy, 53};
    auto response = lookup(qname, qtype, server);

    // if entries in answer section and no errors we are done
    if (!response.answers.empty() &&
        response.header.rescode == ResultCode::NOERROR) {
      cache.insert(qname, qtype, response.answers);
      return response;
    }

    // exit if NXDOMAIN
    if (response.header.rescode == ResultCode::NXDOMAIN) {
      cache.insertNegative(qname, qtype, ResultCode::NXDOMAIN, 300);
      return response;
    }

    // exit if SERVFAIL
    if (response.header.rescode == ResultCode::SERVFAIL) {
      cache.insertNegative(qname, qtype, ResultCode::SERVFAIL, 300);
      return response;
    }

    // cache NS records from authority section
    auto nameservers = response.getNs(qname);
    for (const auto &[domain, host] : nameservers) {
      // look for glue records (a records for ns in addtional section)
      for (const auto &resource : response.resources) {
        if (auto *arecord = std::get_if<ARecord>(&resource)) {
          if (arecord->domain == host) {
            // cache this ns
            std::string domainStr(domain);
            cache.insertNS(domainStr, arecord->addr, arecord->ttl);
            std::cout << "Cached NS: " << domainStr << " -> "
                      << stringutils::ipv4ToString(arecord->addr) << std::endl;
          }
        }
      }
    }

    auto resolvedNs = response.getResolvedNs(qname);
    if (resolvedNs.has_value()) {
      ns = resolvedNs;
      continue;
    }

    std::string newNsServer;
    auto unresolvedNs = response.getUnresolvedNs(qname);
    if (!unresolvedNs.has_value()) {
      return response;
    }

    std::string newNsName = *unresolvedNs;

    DnsPacket recursiveResponse = recursiveLookup(newNsName, A{}, cache);

    auto newNs = recursiveResponse.getRandomA();
    if (newNs.has_value()) {
      ns = newNs;
      continue;
    } else {
      return response;
    }
  }
}

inline void handleQuery(int sockfd, DnsCache &cache) {
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
      DnsPacket result = recursiveLookup(question.name, question.qtype, cache);

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
