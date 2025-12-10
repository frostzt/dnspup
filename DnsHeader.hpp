#ifndef DNSHEADER_HPP
#define DNSHEADER_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "BytePacketBuffer.hpp"
#include "ResultCode.hpp"

class DnsHeader {
public:
  uint16_t id;

  bool recursionDesired;
  bool truncatedMessage;
  bool authoritativeAnswer;
  uint8_t opcode;
  bool response;

  ResultCode rescode;
  bool checkingDisabled;
  bool authedData;
  bool z;
  bool recursionAvailable;

  uint16_t questions;
  uint16_t answers;
  uint16_t authoritativeEntries;
  uint16_t resourceEntries;

  DnsHeader()
      : id(0), recursionDesired(false), truncatedMessage(false),
        authoritativeAnswer(false), opcode(0), response(false),
        rescode(ResultCode::NOERROR), checkingDisabled(false),
        authedData(false), z(false), recursionAvailable(false), questions(0),
        answers(0), authoritativeEntries(0), resourceEntries(0) {}

  void read(BytePacketBuffer &);

  void write(BytePacketBuffer &);

  friend std::ostream &operator<<(std::ostream &stream,
                                  const DnsHeader &header) {
    std::cout << "----- [[DNS Header]] -----\n";
    std::cout << "id: " << header.id << "\n";
    std::cout << "recursion_desired: " << header.recursionDesired << "\n";
    std::cout << "truncated_message: " << header.truncatedMessage << "\n";
    std::cout << "authoritative_answer: " << header.authoritativeAnswer << "\n";
    std::cout << "opcode: " << header.opcode << "\n";
    std::cout << "response: " << header.response << "\n";
    std::cout << "rescode: " << header.rescode << "\n";
    std::cout << "checking_disabled: " << header.checkingDisabled << "\n";
    std::cout << "authed_data: " << header.authedData << "\n";
    std::cout << "z: " << header.z << "\n";
    std::cout << "recursion_available: " << header.recursionAvailable << "\n";
    std::cout << "questions: " << header.questions << "\n";
    std::cout << "answers: " << header.answers << "\n";
    std::cout << "authoritative_entries: " << header.authoritativeEntries
              << "\n";
    std::cout << "resource_entries: " << header.resourceEntries << "\n";
    return stream;
  }
};

inline void DnsHeader::read(BytePacketBuffer &buffer) {
  this->id = *buffer.readU16();

  auto flags = buffer.readU16();
  if (!flags.has_value()) {
    throw std::runtime_error("failed to read flags");
  }

  uint8_t a = (*flags >> 8);
  uint8_t b = (*flags & 0xFF);

  this->recursionDesired = (a & (1 << 0)) > 0;
  this->truncatedMessage = (a & (1 << 1)) > 0;
  this->authoritativeAnswer = (a & (1 << 2)) > 0;
  this->opcode = (a >> 3) & 0x0F;
  this->response = (a & (1 << 7)) > 0;

  this->rescode = resultCodeFromNum(b & 0x0F);
  this->checkingDisabled = (b & (1 << 4)) > 0;
  this->authedData = (b & (1 << 5)) > 0;
  this->z = (b & (1 << 6)) > 0;
  this->recursionAvailable = (b & (1 << 7)) > 0;

  this->questions = *buffer.readU16();
  this->answers = *buffer.readU16();
  this->authoritativeEntries = *buffer.readU16();
  this->resourceEntries = *buffer.readU16();
}

inline void DnsHeader::write(BytePacketBuffer &buffer) {
  // write the id
  buffer.writeU16(this->id);

  // write the flags
  buffer.writeU8((static_cast<uint8_t>(this->recursionDesired)) |
                 (static_cast<uint8_t>(this->truncatedMessage) << 1) |
                 (static_cast<uint8_t>(this->authoritativeAnswer) << 2) |
                 (this->opcode << 3) |
                 (static_cast<uint8_t>(this->response) << 7));

  buffer.writeU8((static_cast<uint8_t>(this->rescode)) |
                 (static_cast<uint8_t>(this->checkingDisabled) << 4) |
                 (static_cast<uint8_t>(this->authedData) << 5) |
                 (static_cast<uint8_t>(this->z) << 6) |
                 (static_cast<uint8_t>(this->recursionAvailable) << 7));


  buffer.writeU16(this->questions);
  buffer.writeU16(this->answers);
  buffer.writeU16(this->authoritativeEntries);
  buffer.writeU16(this->resourceEntries);
}

#endif // DNSHEADER_HPP
