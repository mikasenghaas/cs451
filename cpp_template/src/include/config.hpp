#pragma once

/**
 * @brief Configuration class
 *
 * @details Reads the configuration file and stores the message count and receiver ID
*/
class Config
{
private:
    int message_count;
    size_t receiver_id;

public:
    Config(const std::string &file_name)
    {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + file_name);
        }
        
        if (!(file >> message_count >> receiver_id)) {
            throw std::runtime_error("Failed to read values from config file");
        }
        
        // Validate the read values
        if (message_count <= 0) {
            throw std::runtime_error("Invalid message count in config file");
        }
    }

    size_t get_receiver_id() const
    {
        return receiver_id;
    }

    int get_message_count() const
    {
        return message_count;
    }
};