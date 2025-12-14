#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace stringutils {
inline std::vector<std::string> splitString(const std::string &str,
                                            char delim) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, delim)) {
    tokens.push_back(token);
  }
  return tokens;
}

inline std::string ipv4ToString(const std::array<uint8_t, 4> &addr) {
  return std::to_string(addr[0]) + "." + std::to_string(addr[1]) + "." +
         std::to_string(addr[2]) + "." + std::to_string(addr[3]);
}

inline std::optional<std::array<uint8_t, 4>>
parseIpv4(const std::string &ipStr) {
  std::array<uint8_t, 4> result;
  int octet;
  int count = 0;

  std::istringstream iss(ipStr);
  std::string token;

  while (std::getline(iss, token, '.')) {
    if (count >= 4)
      return std::nullopt; // too many octets

    try {
      octet = std::stoi(token);
      if (octet < 0 || octet > 255)
        return std::nullopt;
      result[count++] = static_cast<uint8_t>(octet);
    } catch (...) {
      return std::nullopt;
    }
  }

  if (count != 4)
    return std::nullopt;
  return result;
}

inline std::string ipv6ToString(const std::array<uint8_t, 16> &addr) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  for (size_t i = 0; i < 16; i += 2) {
    if (i > 0)
      ss << ":";
    uint16_t segment = (static_cast<uint16_t>(addr[i]) << 8 | addr[i + 1]);
    ss << std::setw(4) << segment;
  }

  return ss.str();
}
} // namespace stringutils

#endif // STRING_UTILS_HPP
