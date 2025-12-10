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
#include "StringUtils.hpp"

// ----- UNKNOWN RECORD ------
struct UnknownRecord {
  std::string domain;
  uint16_t qtype;
  uint16_t dataLength;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream,
                                const UnknownRecord &record) {
  stream << "Unknown Record { domain: " << record.domain
         << ", qtype: " << record.qtype << ", data_len: " << record.dataLength
         << ", ttl: " << record.ttl << " }";
  return stream;
}

// ----- A RECORD ------
struct ARecord {
  std::string domain;
  std::array<uint8_t, 4> addr;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream, const ARecord &record) {
  stream << "A Record { domain: " << record.domain
         << ", addr: " << stringutils::ipv4ToString(record.addr)
         << ", ttl: " << record.ttl << " }";
  return stream;
}

// ----- NS RECORD ------
struct NSRecord {
  std::string domain;
  std::string host;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream, const NSRecord &record) {
  stream << "NS Record { domain: " << record.domain << ", host: " << record.host
         << ", ttl: " << record.ttl << " }";
  return stream;
}

// ----- CNAME RECORD ------
struct CNAMERecord {
  std::string domain;
  std::string host;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream,
                                const CNAMERecord &record) {
  stream << "CNAME Record { domain: " << record.domain
         << ", host: " << record.host << ", ttl: " << record.ttl << " }";
  return stream;
}

// ----- MX RECORD ------
struct MXRecord {
  std::string domain;
  uint16_t priority;
  std::string host;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream, const MXRecord &record) {
  stream << "MX Record { domain: " << record.domain << ", host: " << record.host
         << ", priority: " << record.priority << ", ttl: " << record.ttl
         << " }";
  return stream;
}

// ----- AAAA RECORD ------
struct AAAARecord {
  std::string domain;
  std::array<uint8_t, 16> addr;
  uint32_t ttl;
};

inline std::ostream &operator<<(std::ostream &stream,
                                const AAAARecord &record) {
  stream << "AAAA Record { domain: " << record.domain
         << ", addr: " << stringutils::ipv6ToString(record.addr)
         << ", ttl: " << record.ttl << " }";
  return stream;
}

using DnsRecord = std::variant<UnknownRecord, ARecord, NSRecord, CNAMERecord,
                               MXRecord, AAAARecord>;

inline std::array<uint8_t, 4> ipv4FromUInt32(uint32_t rawAddr) {
  return {static_cast<uint8_t>((rawAddr >> 24) & 0xFF),
          static_cast<uint8_t>((rawAddr >> 16) & 0xFF),
          static_cast<uint8_t>((rawAddr >> 8) & 0xFF),
          static_cast<uint8_t>(rawAddr & 0xFF)};
}

inline size_t writeDnsRecord(DnsRecord record, BytePacketBuffer &buffer) {
  auto startPos = buffer.currentPosition();

  std::visit(
      [&](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, ARecord>) {
          auto arecord = std::get<ARecord>(record);

          buffer.writeQName(arecord.domain);
          buffer.writeU16(fromQueryTypeToNumber(A{}));
          buffer.writeU16(1); // class
          buffer.writeU32(arecord.ttl);
          buffer.writeU16(4);

          // write the address
          buffer.writeU8(arecord.addr[0]);
          buffer.writeU8(arecord.addr[1]);
          buffer.writeU8(arecord.addr[2]);
          buffer.writeU8(arecord.addr[3]);
        } else if constexpr (std::is_same_v<T, NSRecord>) {
          auto nsrecord = std::get<NSRecord>(record);

          buffer.writeQName(nsrecord.domain);
          buffer.writeU16(fromQueryTypeToNumber(NS{}));
          buffer.writeU16(1);
          buffer.writeU32(nsrecord.ttl);

          auto pos = buffer.currentPosition();
          buffer.writeU16(0);

          buffer.writeQName(nsrecord.host);

          auto size = buffer.currentPosition() - (pos + 2);
          buffer.setU16(pos, static_cast<uint16_t>(size));
        } else if constexpr (std::is_same_v<T, CNAMERecord>) {
          auto cnamerecord = std::get<CNAMERecord>(record);

          buffer.writeQName(cnamerecord.domain);
          buffer.writeU16(fromQueryTypeToNumber(CNAME{}));
          buffer.writeU16(1);
          buffer.writeU32(cnamerecord.ttl);

          auto pos = buffer.currentPosition();
          buffer.writeU16(0);

          buffer.writeQName(cnamerecord.host);

          auto size = buffer.currentPosition() - (pos + 2);
          buffer.setU16(pos, static_cast<uint16_t>(size));
        } else if constexpr (std::is_same_v<T, MXRecord>) {
          auto mxrecord = std::get<MXRecord>(record);

          buffer.writeQName(mxrecord.domain);
          buffer.writeU16(fromQueryTypeToNumber(MX{}));
          buffer.writeU16(1);
          buffer.writeU32(mxrecord.ttl);

          auto pos = buffer.currentPosition();
          buffer.writeU16(0);

          buffer.writeU16(mxrecord.priority);
          buffer.writeQName(mxrecord.host);

          auto size = buffer.currentPosition() - (pos + 2);
          buffer.setU16(pos, static_cast<uint16_t>(size));
        } else if constexpr (std::is_same_v<T, AAAARecord>) {
          auto aaaarecord = std::get<AAAARecord>(record);

          buffer.writeQName(aaaarecord.domain);
          buffer.writeU16(fromQueryTypeToNumber(AAAA{}));
          buffer.writeU16(1);
          buffer.writeU32(aaaarecord.ttl);
          buffer.writeU16(16);

          for (auto octet : aaaarecord.addr) {
            buffer.writeU8(octet);
          }
        } else if constexpr (std::is_same_v<T, UnknownRecord>) {
          std::cout << "Skipping record of type Unknown";
        };
      },
      record);

  return buffer.currentPosition() - startPos;
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
        } else if constexpr (std::is_same_v<T, NS>) {
          std::string ns;
          buffer.readQName(ns);
          return NSRecord{std::move(domain), std::move(ns), ttl};
        } else if constexpr (std::is_same_v<T, CNAME>) {
          std::string cname;
          buffer.readQName(cname);
          return CNAMERecord{std::move(domain), std::move(cname), ttl};
        } else if constexpr (std::is_same_v<T, MX>) {
          auto priority = *buffer.readU16();
          std::string mx;
          buffer.readQName(mx);
          return MXRecord{std::move(domain), priority, std::move(mx), ttl};
        } else if constexpr (std::is_same_v<T, AAAA>) {
          std::array<uint8_t, 16> addr;
          for (size_t i = 0; i < 16; i++) {
            addr[i] = *buffer.read();
          }
          return AAAARecord{std::move(domain), addr, ttl};
        } else if constexpr (std::is_same_v<T, Unknown>) {
          buffer.step(dataLength);
          return UnknownRecord{std::move(domain), qtypeNum, dataLength, ttl};
        }

        throw std::runtime_error("Unexpected QueryType variant");
      },
      qtype);
}

#endif // DNSRECORD_HPP
