#ifndef DNSRECORD_HPP
#define DNSRECORD_HPP

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

#include "BytePacketBuffer.hpp"
#include "QueryType.hpp"

struct UnknownRecord {
  std::string domain;
  uint16_t qtype;
  uint16_t dataLength;
  uint32_t ttl;
};

struct ARecord {
  std::string domain;
  std::array<uint8_t, 4> addr;
  uint32_t ttl;
};

inline std::string ipv4ToString(const std::array<uint8_t, 4>& addr) {
  return std::to_string(addr[0]) + "." +
         std::to_string(addr[1]) + "." +
         std::to_string(addr[2]) + "." +
         std::to_string(addr[3]);
}

inline std::ostream& operator<<(std::ostream& stream, const ARecord& record) {
  stream << "A Record { domain: " << record.domain 
         << ", addr: " << ipv4ToString(record.addr)
         << ", ttl: " << record.ttl << " }";
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const UnknownRecord& record) {
  stream << "Unknown Record { domain: " << record.domain 
         << ", qtype: " << record.qtype 
         << ", data_len: " << record.dataLength 
         << ", ttl: " << record.ttl << " }";
  return stream;
}

using DnsRecord = std::variant<UnknownRecord, ARecord>;

inline std::array<uint8_t, 4> ipv4FromUInt32(uint32_t rawAddr) {
  return {static_cast<uint8_t>((rawAddr >> 24) & 0xFF),
          static_cast<uint8_t>((rawAddr >> 16) & 0xFF),
          static_cast<uint8_t>((rawAddr >> 8) & 0xFF),
          static_cast<uint8_t>(rawAddr & 0xFF)};
}

inline DnsRecord readDnsRecord(BytePacketBuffer &buffer) {
  std::string domain;
  buffer.readQName(domain);

  uint16_t qtypeNum = *buffer.readU16();
  QueryType qtype = fromNumberToQueryType(qtypeNum);
  buffer.readU16(); // class field
  uint32_t ttl = *buffer.readU32();
  uint16_t dataLength = *buffer.readU16();

  return std::visit(
      [&](auto &&arg) -> DnsRecord {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, A>) {
          uint32_t rawAddr = *buffer.readU32();
          std::array<uint8_t, 4> addr = ipv4FromUInt32(rawAddr);
          return ARecord{std::move(domain), addr, ttl};
        } else if constexpr (std::is_same_v<T, Unknown>) {
          buffer.step(dataLength);
          return UnknownRecord{std::move(domain), qtypeNum, dataLength, ttl};
        }

        throw std::runtime_error("Unexpected QueryType variant");
      },
      qtype);
}

#endif // DNSRECORD_HPP
