#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

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
} // namespace stringutils

#endif // STRING_UTILS_HPP
