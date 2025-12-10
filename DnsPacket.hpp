#ifndef DNSPACKET_HPP
#define DNSPACKET_HPP

#include <algorithm>
#include <array>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "BytePacketBuffer.hpp"
#include "DnsHeader.hpp"
#include "DnsQuestion.hpp"
#include "DnsRecord.hpp"

class DnsPacket {
private:
  std::vector<std::pair<std::string_view, std::string_view>>
  getNs(const std::string &qname) const;

public:
  DnsHeader header;
  std::vector<DnsQuestion> questions;
  std::vector<DnsRecord> answers;
  std::vector<DnsRecord> authorities;
  std::vector<DnsRecord> resources;

  DnsPacket() = default;

  static DnsPacket fromBuffer(BytePacketBuffer &buffer);

  void write(BytePacketBuffer &);

  std::optional<std::array<uint8_t, 4>> getRandomA();

  std::optional<std::array<uint8_t, 4>>
  getResolvedNs(const std::string &qname) const;

  std::optional<std::string> getUnresolvedNs(const std::string &qname) const;

  friend std::ostream &operator<<(std::ostream &stream,
                                  const DnsPacket &packet) {
    // print header
    std::cout << packet.header << "\n\n";

    // print questions
    for (const auto &question : packet.questions) {
      std::cout << question;
    }

    // print answers
    std::cout << "\n\n----- [[ANSWERS; LENGTH=" << packet.answers.size()
              << "]] -----\n";
    for (const auto &answer : packet.answers) {
      std::visit([](const auto &r) { std::cout << r << std::endl; }, answer);
    }

    return stream;
  }
};

inline std::optional<std::array<uint8_t, 4>> DnsPacket::getRandomA() {
  for (const auto &answer : this->answers) {
    if (auto *arecord = std::get_if<ARecord>(&answer)) {
      return arecord->addr;
    }
  }
  return std::nullopt;
}

inline std::vector<std::pair<std::string_view, std::string_view>>
DnsPacket::getNs(const std::string &qname) const {
  std::vector<std::pair<std::string_view, std::string_view>> result;

  for (const auto &record : this->authorities) {
    if (auto *nsrecord = std::get_if<NSRecord>(&record)) {
      // only includes if qname ends with domain
      // domain = google.com; qname = www.google.com
      if (qname.ends_with(nsrecord->domain)) {
        result.emplace_back(nsrecord->domain, nsrecord->host);
      }
    }
  }

  return result;
}

inline std::optional<std::array<uint8_t, 4>>
DnsPacket::getResolvedNs(const std::string &qname) const {
  // get ns for this query
  auto nameservers = this->getNs(qname);

  // pull domain and host and look for a records
  for (const auto &[domain, host] : nameservers) {
    // search resources for an A Record matching this host
    for (const auto &resource : this->resources) {
      // check if its an A record with matching domain
      if (auto *arecord = std::get_if<ARecord>(&resource)) {
        // check if the domain matches
        if (arecord->domain == host) {
          return arecord->addr;
        }
      }
    }
  }

  return std::nullopt;
}

inline std::optional<std::string>
DnsPacket::getUnresolvedNs(const std::string &qname) const {
  auto nameservers = this->getNs(qname);

  if (!nameservers.empty()) {
    return std::string(nameservers[0].second);
  }

  return std::nullopt;
}

inline void DnsPacket::write(BytePacketBuffer &buffer) {
  this->header.questions = static_cast<uint16_t>(this->questions.size());
  this->header.answers = static_cast<uint16_t>(this->answers.size());
  this->header.authoritativeEntries =
      static_cast<uint16_t>(this->authorities.size());
  this->header.resourceEntries = static_cast<uint16_t>(this->resources.size());

  this->header.write(buffer);

  for (auto &question : this->questions) {
    question.write(buffer);
  }

  for (auto &rec : this->answers) {
    writeDnsRecord(rec, buffer);
  }

  for (auto &rec : this->authorities) {
    writeDnsRecord(rec, buffer);
  }

  for (auto &rec : this->resources) {
    writeDnsRecord(rec, buffer);
  }
}

inline DnsPacket DnsPacket::fromBuffer(BytePacketBuffer &buffer) {
  DnsPacket result;
  result.header.read(buffer);

  // read questions
  for (size_t i = 0; i < result.header.questions; i++) {
    result.questions.push_back(DnsQuestion::read(buffer));
  }

  // read answers
  for (size_t i = 0; i < result.header.answers; i++) {
    result.answers.push_back(readDnsRecord(buffer));
  }

  // read authorities
  for (size_t i = 0; i < result.header.authoritativeEntries; i++) {
    result.authorities.push_back(readDnsRecord(buffer));
  }

  // read resources
  for (size_t i = 0; i < result.header.resourceEntries; i++) {
    result.resources.push_back(readDnsRecord(buffer));
  }

  return result;
}

#endif // DNSPACKET_HPP
