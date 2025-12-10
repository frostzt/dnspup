# DnsPup

A toy implementation of a DNS Server from scratch.

## How to run this?

1. Clone the project and do `make` to build the binary.
2. Once the binary is build simple run the binary by `./bin/dns_resolver`
3. The DNS resolver now runs on PORT `2053` and you can
start resolving your DNS queries.

Example `dig` using `dnspup`

`dig @localhost -p 2053 google.com A`
