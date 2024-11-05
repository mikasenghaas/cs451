#pragma once

#include "address.hpp"

/**
 * @brief Host
 *
 * @details A host is associated with an ID (1, ..., 128) and an IPV4 address
*/
class Host
{
private:
    size_t id{0}; // Host ID (1, ..., 128)
    Address address; // IPV4 address

public:
    // Default constructor
    Host() = default;

    // Main constructor
    Host(size_t id, Address address) : id(id), address(address) {}

    // Getters
    size_t get_id() const { return id; }
    Address get_address() const { return address; }

    // String representation
    std::string to_string() const {return "Host " + std::to_string(id) + " at " + address.to_string(); }
    friend std::ostream &operator<<(std::ostream &os, const Host &host) { return os << host.to_string(); }
};
