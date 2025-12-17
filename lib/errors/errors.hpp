#ifndef CUSTOM_ERRORS_HPP
#define CUSTOM_ERRORS_HPP

#include <stdexcept>

class TimeoutException : public std::runtime_error {
public:
  explicit TimeoutException(const std::string &msg) : std::runtime_error(msg) {}
};

// ---------------- SECURITY ----------------
class SecurityException : public std::runtime_error {
  public:
    explicit SecurityException(const std::string &msg) : std::runtime_error(msg) {}
};

#endif // CUSTOM_ERRORS_HPP
