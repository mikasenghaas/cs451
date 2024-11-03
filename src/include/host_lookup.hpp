#pragma once

#include <arpa/inet.h>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <sstream>

#include "host.hpp"
#include "address.hpp"

class HostLookup
{
private:
    std::unordered_map<Address, size_t> address_to_host;
    std::unordered_map<size_t, Address> host_to_address;

public:
    HostLookup(Hosts hosts)
    {
        for (auto host : hosts.get_hosts())
        {
            this->address_to_host[host.get_address()] = host.get_id();
            this->host_to_address[host.get_id()] = host.get_address();
        }
    }

    size_t get_host_id(Address address)
    {
        auto it = address_to_host.find(address);
        if (it == address_to_host.end())
        {
            throw std::out_of_range("Address not found");
        }
        return it->second;
    }

    Address get_address(size_t host_id)
    {
        auto it = host_to_address.find(host_id);
        if (it == host_to_address.end())
        {
            throw std::out_of_range("Host ID not found");
        }
        return it->second;
    }
};
