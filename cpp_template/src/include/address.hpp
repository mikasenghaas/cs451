#pragma once

/**
 * @brief IPv4 Address
 *
 * @details This class encapsulates an IPv4 address and port number, providing
 * conversion utilities between different address representations including:
 * - String format (e.g. "127.0.0.1:8000")
 * - Network byte order (sockaddr_in)
 * - Host byte order (uint32_t ip, uint16_t port)
*/
class Address
{
public:
    uint32_t ip{0}; // IP address in network byte order (4B)
    uint16_t port{0}; // Port number (2B + 2B padding)

    // Default constructor
    Address() = default;

    // Main constructor
    Address(uint32_t ip, uint16_t port) : ip(ip), port(port) {}

    // Constructor from IPv4 string and port
    Address(const std::string &ip, uint16_t port)
    {
        std::string ip_str = ip == "localhost" ? "127.0.0.1" : ip;
        this->ip = ntohl(inet_addr(ip_str.c_str()));
        this->port = port;
    }

    // Convert to socket address format
    sockaddr_in to_sockaddr() const
    {
        sockaddr_in address;
        memset(reinterpret_cast<void *>(&address), 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(this->ip);
        address.sin_port = htons(port);

        return address;
    }

    // String representation
    std::string to_string() const
    {
        in_addr net_ip;
        net_ip.s_addr = htonl(ip);
        return std::string(inet_ntoa(net_ip)) + ":" + std::to_string(static_cast<int>(port));
    }
    friend std::ostream &operator<<(std::ostream &os, const Address &address) { return os << address.to_string(); }

};