#include <exception>
#include <fstream>
#include <iostream>

#include "BytePacketBuffer.hpp"
#include "DnsPacket.hpp"

int main() {
  try {
    std::ifstream file("response_packet.txt", std::ios::binary);
    if (!file) {
      std::cerr << "filed to open file" << std::endl;
      return 1;
    }

    // create buffer and read file
    BytePacketBuffer buffer;
    file.read(reinterpret_cast<char *>(buffer.buf), sizeof(buffer.buf));
    if (!file && !file.eof()) {
      std::cerr << "filed to read file" << std::endl;
      return 1;
    }

    // parse packet
    DnsPacket packet = DnsPacket::fromBuffer(buffer);

    std::cout << packet;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
