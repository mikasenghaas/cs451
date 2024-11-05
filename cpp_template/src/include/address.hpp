#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <functional>

class Address
{
public:
    uint32_t ip;
    uint16_t port;

    // Default constructor
    Address() : ip(0), port(0) {}

    // Constructor from sockaddr_in
    Address(sockaddr_in ipv4) : Address((ntohl(ipv4.sin_addr.s_addr)), ntohs(ipv4.sin_port)) {}

    // Constructor from string
    Address(const std::string &ip, uint16_t port)
    {
        std::string ip_str = ip == "localhost" ? "127.0.0.1" : ip;
        this->ip = ntohl(inet_addr(ip_str.c_str()));
        this->port = port;
    }

    // Constructor from uint32_t and uint16_t
    Address(uint32_t ip, uint16_t port) : ip(ip), port(port) {}

    std::string to_string() const
    {
        char source_buffer[16];
        in_addr net_ip;
        net_ip.s_addr = htonl(this->ip);
        std::string result = std::string(inet_ntoa(net_ip));
        result.append(":");
        result.append(std::to_string(static_cast<int>(this->port)));

        return result;
    }

    sockaddr_in to_sockaddr() const
    {
        sockaddr_in address;
        memset(reinterpret_cast<void *>(&address), 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(this->ip);
        address.sin_port = htons(port);

        return address;
    }

    uint64_t to_uint64_t() const { return (this->ip << 16) + this->port; }

    // Add this operator
    bool operator==(const Address &other) const
    {
        return ip == other.ip && port == other.port;
    }

    // If you're using std::map or std::set, also add this
    bool operator<(const Address &other) const
    {
        return to_uint64_t() < other.to_uint64_t();
    }
};

namespace std
{
    template <>
    struct hash<Address>
    {
        size_t operator()(const Address &addr) const
        {
            // Combine ip and port using a simple hash function
            return std::hash<uint64_t>{}(addr.to_uint64_t());
        }
    };
}