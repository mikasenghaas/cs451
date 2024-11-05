#pragma once

#include <fstream>
#include <iostream>

class Config
{
private:
    uint64_t message_count;
    uint64_t receiver_id;

public:
    Config(const std::string &file_name)
    {
        std::ifstream file(file_name);
        file >> message_count >> receiver_id;
    }

    uint64_t get_receiver_id()
    {
        return receiver_id;
    }

    uint64_t get_message_count()
    {
        return message_count;
    }
};