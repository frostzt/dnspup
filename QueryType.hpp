#ifndef QUERYTYPE_HPP
#define QUERYTYPE_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <variant>

struct Unknown {
  uint16_t value;
};

struct A {}; // 1

using QueryType = std::variant<Unknown, A>;

inline QueryType fromNumberToQueryType(uint16_t num) {
  switch (num) {
  case 1:
    return A{};
  default:
    return Unknown{num};
  }
}

inline void printQueryType(QueryType qt) {
  std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, A>) {
          std::cout << "A";
        } else if constexpr (std::is_same_v<T, Unknown>) {
          std::cout << "UNKNOWN";
        }
      },
      qt);
}

inline uint16_t fromQueryTypeToNumber(QueryType qt) {
  return std::visit(
      [](auto &&arg) -> uint16_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, A>) {
          return 1;
        } else if constexpr (std::is_same_v<T, Unknown>) {
          return arg.value;
        }
        return 0;
      },
      qt);
}

#endif // QUERYTYPE_HPP
