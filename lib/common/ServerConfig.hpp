#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <array>
#include <cstdint>

struct Server {
  std::array<uint8_t, 4> s_addr;
  uint16_t s_port;
};

#endif // SERVER_CONFIG_HPP
