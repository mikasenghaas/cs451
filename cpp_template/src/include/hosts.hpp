#pragma once

#include <vector>
#include <unordered_map>

#include "host.hpp"
#include "address.hpp"

/**
 * @brief Hosts
 *
 * @details Reads the hosts file and stores the hosts and their addresses, also
 * provides a method to get the address of a host given its ID.
 */
class Hosts
{
private:
    std::vector<Host> hosts;
    std::unordered_map<size_t, Address> host_to_address;

public:
    // Constructor from hosts file (`id ip port`)
    Hosts(std::string file_name)
    {
        // Open file
        std::ifstream file(file_name);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open hosts file");
        }

        // Read file line by line
        std::string line;
        while (std::getline(file, line))
        {
            // Parse line
            std::istringstream iss(line);
            size_t id;
            std::string ip;
            uint16_t port;
            if (!(iss >> id >> ip >> port))
            {
                throw std::runtime_error("Failed to parse hosts file");
            }
            Address addr(ip, port);
            this->hosts.push_back(Host(id, addr));
            this->host_to_address[id] = addr;
        }
    }

    std::vector<Host> get_hosts()
    {
        return this->hosts;
    }

    size_t get_host_count()
    {
        return this->hosts.size();
    }

    Address get_address(size_t host_id)
    {
        if (this->host_to_address.find(host_id) == this->host_to_address.end()) {
            throw std::runtime_error("Host ID not found");
        }
        return this->host_to_address[host_id];
    }
};
