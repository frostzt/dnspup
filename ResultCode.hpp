#ifndef RESULTCODE_HPP
#define RESULTCODE_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>

enum class ResultCode : uint8_t {
  NOERROR = 0,
  FORMERR = 1,
  SERVFAIL = 2,
  NXDOMAIN = 3,
  NOTIMP = 4,
  REFUSED = 5,
};

inline std::ostream &operator<<(std::ostream &stream,
                                const ResultCode resultCode) {
  switch (resultCode) {
  case ResultCode::NOERROR:
    stream << "NOERROR(0)";
    break;
  case ResultCode::FORMERR:
    stream << "FORMERR(1)";
    break;
  case ResultCode::SERVFAIL:
    stream << "SERVFAIL(2)";
    break;
  case ResultCode::NXDOMAIN:
    stream << "NXDOMAIN(3)";
    break;
  case ResultCode::NOTIMP:
    stream << "NOTIMP(4)";
    break;
  case ResultCode::REFUSED:
    stream << "REFUSED(5)";
    break;
  }
  return stream;
}

inline ResultCode resultCodeFromNum(uint8_t num) {
  switch (num) {
  case 1:
    return ResultCode::FORMERR;
  case 2:
    return ResultCode::SERVFAIL;
  case 3:
    return ResultCode::NXDOMAIN;
  case 4:
    return ResultCode::NOTIMP;
  case 5:
    return ResultCode::REFUSED;
  default:
    return ResultCode::NOERROR;
  }
}

#endif // RESULTCODE_HPP
