#ifndef BYTEPACKETBUFFER_HPP
#define BYTEPACKETBUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "StringUtils.hpp"

class BytePacketBuffer {
public:
  uint8_t buf[512];
  size_t currPos;

  // Initialize empty buffer with current position set as 0
  BytePacketBuffer() : buf{}, currPos(0) {}

  // current position within our buffer
  size_t currentPosition();

  // step forward in buffer position a specific number of steps
  void step(size_t);

  // change the buffer position
  void seek(size_t);

  // read a single byte and move the position one step forward
  std::optional<uint8_t> read();

  // get one byte
  std::optional<uint8_t> get(size_t);

  // get a range of bytes
  std::span<uint8_t> getRange(size_t, size_t);

  // read two bytes stepping two steps ahead
  std::optional<uint16_t> readU16();

  // read four bytes stepping four steps ahead
  std::optional<uint32_t> readU32();

  // read a qname
  void readQName(std::string &);

  // write 8 bits into buffer
  void write(uint8_t);

  // write 8 bits into buffer
  void writeU8(uint8_t);

  // write 16 bits into buffer
  void writeU16(uint16_t);

  // write 32 bits into buffer
  void writeU32(uint32_t);

  // write a qname
  void writeQName(std::string &);
};

inline size_t BytePacketBuffer::currentPosition() { return this->currPos; };

inline void BytePacketBuffer::step(size_t position) {
  this->currPos += position;
}

inline void BytePacketBuffer::seek(size_t position) {
  this->currPos = position;
}

inline std::optional<uint8_t> BytePacketBuffer::read() {
  if (this->currPos >= 512) {
    throw std::out_of_range("End of buffer");
  }
  return this->buf[this->currPos++];
}

inline void BytePacketBuffer::write(uint8_t value) {
  if (this->currPos >= 512) {
    throw std::out_of_range("end of buffer");
  }

  this->buf[this->currPos] = value;
  this->currPos += 1;
}

inline void BytePacketBuffer::writeU8(uint8_t value) { this->write(value); }

inline std::optional<uint8_t> BytePacketBuffer::get(size_t position) {
  if (position >= 512) {
    throw std::out_of_range("End of buffer");
  }

  return this->buf[position];
}

inline std::span<uint8_t> BytePacketBuffer::getRange(size_t start,
                                                     size_t length) {
  if (start + length > 512) {
    throw std::out_of_range("End of buffer");
  }

  return std::span<uint8_t>(&this->buf[start], length);
}

inline void BytePacketBuffer::writeU16(uint16_t value) {
  this->write(static_cast<uint8_t>(value >> 8));
  this->write(static_cast<uint8_t>(value & 0xFF));
}

inline std::optional<uint16_t> BytePacketBuffer::readU16() {
  auto high = this->read();
  if (!high.has_value()) {
    return std::nullopt;
  }

  auto low = this->read();
  if (!low.has_value()) {
    return std::nullopt;
  }

  return (static_cast<uint16_t>(*high) << 8) | static_cast<uint16_t>(*low);
}

inline void BytePacketBuffer::writeU32(uint32_t value) {
  this->write(static_cast<uint8_t>((value >> 24) & 0xFF));
  this->write(static_cast<uint8_t>((value >> 16) & 0xFF));
  this->write(static_cast<uint8_t>((value >> 8) & 0xFF));
  this->write(static_cast<uint8_t>(value & 0xFF));
}

inline std::optional<uint32_t> BytePacketBuffer::readU32() {
  uint32_t result = 0;
  for (size_t i = 0; i < 4; i++) {
    auto readVal = this->read();
    if (!readVal.has_value()) {
      return std::nullopt;
    }
    result |= static_cast<uint32_t>(*readVal) << (i * 8);
  }

  return result;
}

inline void BytePacketBuffer::writeQName(std::string &qname) {
  char delim = '.';
  std::vector<std::string> labels = stringutils::splitString(qname, delim);

  for (const auto &label : labels) {
    auto len = label.size();

    // https://datatracker.ietf.org/doc/html/rfc1035#section-2.3.1
    if (len > 63) {
      throw std::runtime_error("single label exceeds 63 characters of length");
    }

    // write the length byte
    this->writeU8(static_cast<uint8_t>(len));

    // write the label
    for (char c : label) {
      this->writeU8(static_cast<uint8_t>(c));
    }
  }

  // write the null ptr
  this->writeU8(0);
}

inline void BytePacketBuffer::readQName(std::string &outstr) {
  auto pos = this->currPos;

  const size_t maxJumps = 5;
  auto jumped = false;
  size_t jumpsPerformed = 0;

  auto delim = "";
  while (true) {
    if (jumpsPerformed > maxJumps) {
      throw std::runtime_error("limit of jumps exceeded maximum jumps");
    }

    auto len = this->get(pos);

    // if two MSB are set then its a jump
    if ((*len & 0xC0) == 0xC0) {
      if (!jumped) {
        this->seek(pos + 2);
      }

      auto nextByte = static_cast<uint16_t>(*this->get(pos + 1));
      auto offset = ((static_cast<uint16_t>(*len) ^ 0xC0) << 8) | nextByte;
      pos = static_cast<size_t>(offset);

      jumped = true;
      jumpsPerformed += 1;

      continue;
    } else {
      pos += 1;
      if (len == 0) {
        break;
      }

      outstr.append(delim);

      auto stringBuffer = this->getRange(pos, *len);
      outstr.append(reinterpret_cast<const char *>(stringBuffer.data()),
                    stringBuffer.size());

      delim = ".";
      pos += static_cast<size_t>(*len);
    }
  }

  if (!jumped) {
    this->seek(pos);
  }
}

#endif // BYTEPACKETBUFFER_HPP
