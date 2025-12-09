#ifndef DNSPACKET_HPP
#define DNSPACKET_HPP

#include <iostream>
#include <vector>

#include "BytePacketBuffer.hpp"
#include "DnsHeader.hpp"
#include "DnsQuestion.hpp"
#include "DnsRecord.hpp"

class DnsPacket {
public:
  DnsHeader header;
  std::vector<DnsQuestion> questions;
  std::vector<DnsRecord> answers;
  std::vector<DnsRecord> authorities;
  std::vector<DnsRecord> resources;

  DnsPacket() = default;

  static DnsPacket fromBuffer(BytePacketBuffer &buffer);

  void write(BytePacketBuffer &);

  friend std::ostream &operator<<(std::ostream &stream,
                                  const DnsPacket &packet) {
    // print header
    std::cout << packet.header << "\n\n";

    // print questions
    for (const auto &question : packet.questions) {
      std::cout << question;
    }

    // print answers
    std::cout << "\n\n----- [[ANSWERS]] -----\n";
    for (const auto &answer : packet.answers) {
      std::visit([](const auto &r) { std::cout << r << std::endl; }, answer);
    }

    return stream;
  }
};

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
