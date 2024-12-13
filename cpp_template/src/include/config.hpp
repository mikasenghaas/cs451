#pragma once

#include <string>
#include <vector>
#include <set>

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
    int num_proposals; // Proposal count
    int max_proposal_size; // Maximum number of elements per proposal
    int max_distinct_elements; // Maximum number of distinct elements
    std::vector<std::set<int>> proposals; // List of proposal sets

public:

    LatticeAgreementConfig(const std::string &file_name)
    {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + file_name);
        }
        
        // Read config values
        if (!(file >> num_proposals >> max_proposal_size >> max_distinct_elements)) {
            throw std::runtime_error("Failed to read value from config file");
        }

        // Read proposals
        std::string line;
        std::getline(file, line);
        for (int i=0; i<num_proposals; i++) {
            std::getline(file, line);
            std::set<int> proposal;
            std::istringstream iss(line);
            int value;
            while (iss >> value) {
                proposal.insert(value);
            }
            proposals.push_back(proposal);
        }
    }

    int get_num_proposals() { return num_proposals; }
    int get_max_proposal_size() { return max_proposal_size; }
    int get_max_distinct_elements() { return max_proposal_size; }
    std::vector<std::set<int>> get_proposals() { return proposals; }
};