#pragma once

#include <optional>
#include <string>

namespace converse {
namespace server {
class Address {
   public:
    Address(const std::string &host, int port);
    std::string host;
    int port;

    operator std::string() const;
    bool operator<(const Address &rhs) const;
    bool operator==(const Address &rhs) const;
};

class Server {
   public:
    Server(const std::string &name, const Address &my_address,
           const std::optional<Address> &join_address);
};

}  // namespace server
}  // namespace converse
