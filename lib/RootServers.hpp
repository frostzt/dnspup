#ifndef ROOT_SERVERS_HPP
#define ROOT_SERVERS_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct RootServer {
  std::string hostname;
  std::array<uint8_t, 4> ipv4address;

  // Metrics
  double avgLatency = 0.0;
  uint64_t hits = 0;
  uint64_t timeoutCounts = 0;
};

inline RootServer RootServerA{"a.root-servers.net", {198, 41, 0, 4}};
inline RootServer RootServerB{"b.root-servers.net", {170, 247, 170, 2}};
inline RootServer RootServerC{"c.root-servers.net", {192, 33, 4, 12}};
inline RootServer RootServerD{"d.root-servers.net", {199, 7, 91, 13}};
inline RootServer RootServerE{"e.root-servers.net", {192, 203, 230, 10}};
inline RootServer RootServerF{"f.root-servers.net", {192, 5, 5, 241}};
inline RootServer RootServerG{"g.root-servers.net", {192, 112, 36, 4}};
inline RootServer RootServerH{"h.root-servers.net", {198, 97, 190, 53}};
inline RootServer RootServerI{"i.root-servers.net", {192, 36, 148, 17}};
inline RootServer RootServerJ{"j.root-servers.net", {192, 58, 128, 30}};
inline RootServer RootServerK{"k.root-servers.net", {193, 0, 14, 129}};
inline RootServer RootServerL{"l.root-servers.net", {199, 7, 83, 42}};
inline RootServer RootServerM{"m.root-servers.net", {202, 12, 27, 33}};

inline std::vector<std::string> ROOT_SERVERS_STRING_LIST = {
    "198.41.0.4",     // a.root-servers.net
    "170.247.170.2",  // b.root-servers.net
    "192.33.4.12",    // c.root-servers.net
    "199.7.91.13",    // d.root-servers.net
    "192.203.230.10", // e.root-servers.net
    "192.5.5.241",    // f.root-servers.net
    "192.112.36.4",   // g.root-servers.net
    "198.97.190.53",  // h.root-servers.net
    "192.36.148.17",  // i.root-servers.net
    "192.58.128.30",  // j.root-servers.net
    "193.0.14.129",   // k.root-servers.net
    "199.7.83.42",    // l.root-servers.net
    "202.12.27.33"    // m.root-servers.net
};

class RootServerRepository {
public:
  static std::vector<RootServer> servers;
};

inline std::vector<RootServer> RootServerRepository::servers = {
    RootServerA, RootServerB, RootServerC, RootServerD, RootServerE,
    RootServerF, RootServerG, RootServerH, RootServerI, RootServerJ,
    RootServerK, RootServerL, RootServerM,
};

#endif // ROOT_SERVERS_HPP
