#pragma once

#include <string>
#include <vector>
#include <set>

#include "types.hpp"

/**
 * @brief Configuration class for Perfect Link
 *
 * @details Reads the configuration file for milestone 1 and stores the message count and receiver ID
*/
class PerfectLinkConfig
{
private:
    int message_count;
    size_t receiver_id;

public:
    PerfectLinkConfig(const std::string &file_name)
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

/**
 * @brief Configuration class for FIFO-Order Uniform Reliable Broadcast (M2)
 * 
 * @details Reads the configuration file and stores the message count
*/
class FIFOUniformReliableBroadcastConfig
{
private:
    int message_count;

public:
    FIFOUniformReliableBroadcastConfig(const std::string &file_name)
    {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + file_name);
        }
        
        if (!(file >> message_count)) {
            throw std::runtime_error("Failed to read value from config file");
        }
        
        // Validate the read values
        if (message_count <= 0) {
            throw std::runtime_error("Invalid message count in config file");
        }
    }

    int get_message_count() const
    {
        return message_count;
    }
};

/**
 * @brief Configuration class for Lattice Agreement (M3)
 * 
 * @details Reads the configuration file and stores the 
*/
class LatticeAgreementConfig
{
private:
    std::ifstream file;
    size_t num_rounds; // Number of rounds
    size_t max_proposal_size; // Maximum number of elements per proposal
    size_t num_distinct_elements; // Maximum number of distinct elements
    std::vector<Proposal> proposals; // List of proposal sets

public:

    LatticeAgreementConfig(const std::string &file_name): file(file_name)
    {
        // Read config values
        if (!(file >> num_rounds >> max_proposal_size >> num_distinct_elements)) {
            throw std::runtime_error("Failed to read value from config file");
        }

        // Trash rest of line
        std::string trash;
        std::getline(file, trash);
    }

    size_t get_num_rounds() { return num_rounds; }
    size_t get_max_proposal_size() { return max_proposal_size; }
    size_t get_num_distinct_elements() { return num_distinct_elements; }
    Proposal get_next_proposal() {
        // Get line
        std::string line;
        std::getline(file, line);

        // Prepare proposal
        Proposal proposal;
        ProposalValue proposal_value;
        std::istringstream iss(line);
        ProposalValue value;
        while (iss >> value) {
            proposal.insert(value);
        }
        return proposal;
    }
};