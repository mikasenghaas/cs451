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

class Hosts
{
private:
    std::vector<Host> hosts;

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
};
