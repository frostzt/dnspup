#ifndef DNSQUESTION_HPP
#define DNSQUESTION_HPP

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

#include "BytePacketBuffer.hpp"
#include "QueryType.hpp"

class DnsQuestion {
public:
  std::string name;
  QueryType qtype;

  DnsQuestion(std::string name, QueryType qtype)
      : name(std::move(name)), qtype(qtype) {}

  static DnsQuestion read(BytePacketBuffer &buffer);

  void write(BytePacketBuffer &);

  friend std::ostream &operator<<(std::ostream &stream,
                                  const DnsQuestion &question) {
    std::cout << "----- [[Question]] -----" << "\n"
              << "Name: " << question.name << " Type: ";
    printQueryType(question.qtype);
    std::cout << "\n";
    return stream;
  }
};

inline void DnsQuestion::write(BytePacketBuffer &buffer) {
  buffer.writeQName(this->name);

  auto typenum = fromQueryTypeToNumber(this->qtype);
  buffer.writeU16(typenum);
  buffer.writeU16(1);
}

inline DnsQuestion DnsQuestion::read(BytePacketBuffer &buffer) {
  std::string name;
  buffer.readQName(name);

  QueryType qtype = fromNumberToQueryType(*buffer.readU16());
  auto _ = *buffer.readU16();

  return DnsQuestion(std::move(name), qtype);
}

#endif // DNSQUESTION_HPP
