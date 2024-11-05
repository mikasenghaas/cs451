#pragma once

#include "address.hpp"

class Host
{
private:
    size_t id;
    Address address;

public:
    Host() : id(0), address(Address()) {}
    Host(size_t id, Address address) : id(id), address(address) {}

    size_t get_id() const { return id; }
    Address get_address() const { return address; }
};