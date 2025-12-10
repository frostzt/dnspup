#ifndef QUERYTYPE_HPP
#define QUERYTYPE_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <variant>

struct Unknown {
  uint16_t value;
};

// A - Alias
struct A {}; // 1

// NS - Name Server
struct NS {}; // 2

// CNAME - Canonical Name
struct CNAME {}; // 5

// MX - Mail eXchange
struct MX {}; // 15

// IPv6 Alias
struct AAAA {}; // 28

using QueryType = std::variant<Unknown, A, NS, CNAME, MX, AAAA>;

inline QueryType fromNumberToQueryType(uint16_t num) {
  switch (num) {
  case 1:
    return A{};
  case 2:
    return NS{};
  case 5:
    return CNAME{};
  case 15:
    return MX{};
  case 28:
    return AAAA{};
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
        } else if constexpr (std::is_same_v<T, NS>) {
          std::cout << "NS";
        } else if constexpr (std::is_same_v<T, CNAME>) {
          std::cout << "CNAME";
        } else if constexpr (std::is_same_v<T, MX>) {
          std::cout << "MX";
        } else if constexpr (std::is_same_v<T, AAAA>) {
          std::cout << "AAAA";
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
        } else if constexpr (std::is_same_v<T, NS>) {
          return 2;
        } else if constexpr (std::is_same_v<T, CNAME>) {
          return 5;
        } else if constexpr (std::is_same_v<T, MX>) {
          return 15;
        } else if constexpr (std::is_same_v<T, AAAA>) {
          return 28;
        } else if constexpr (std::is_same_v<T, Unknown>) {
          return arg.value;
        }
        return 0;
      },
      qt);
}

#endif // QUERYTYPE_HPP
