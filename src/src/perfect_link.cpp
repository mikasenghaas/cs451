#include "perfect_link.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <fstream>

PerfectLink::PerfectLink(LinkType type, const char* output_path, const struct sockaddr_in &local_addr): type{type}, local_addr{local_addr}, output_path{output_path} {
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Bind socket to local address
    const struct sockaddr* local_addr_ptr = reinterpret_cast<const struct sockaddr*>(&local_addr);
    if (bind(sockfd, local_addr_ptr, sizeof(local_addr)) < 0) {
        close(sockfd);
        throw std::runtime_error(std::string("Failed to bind socket: ") + strerror(errno));
    }
}

PerfectLink::~PerfectLink() {
    // Clear output
    for (auto& msg : output) {
        delete[] msg.payload;
    }
    output.clear();

    // Clear socket
    if (sockfd >= 0) {
        close(sockfd);
    }
}

void PerfectLink::send(const Message &msg, const struct sockaddr_in &recv_addr) {
    char buffer[BUFFER_SIZE];
    msg.serialize(buffer);
    
    ssize_t sent = sendto(sockfd, buffer, msg.serialized_size(), 0, 
                         reinterpret_cast<const struct sockaddr*>(&recv_addr), 
                         sizeof(recv_addr));
    if (sent < 0) {
        throw std::runtime_error("Failed to send data");
    }
    Message msg_copy = msg;
    msg_copy.payload = new uint8_t[msg.payload_size];
    memcpy(msg_copy.payload, msg.payload, msg.payload_size);
    output.push_back(msg_copy);
}

void PerfectLink::receive() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (true) {
        // Set up for select()
        fd_set readfds;
        struct timeval tv;
        
        // Initialize the file descriptor set
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        // Check if there's data to read
        int ready = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        
        if (ready > 0 && FD_ISSET(sockfd, &readfds)) {
            ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                reinterpret_cast<struct sockaddr*>(&sender_addr), &sender_addr_len);
            
            if (bytes_received > 0) {
                Message msg;
                msg.deserialize(buffer);
                Message msg_copy = msg;
                msg_copy.payload = new uint8_t[msg.payload_size];
                memcpy(msg_copy.payload, msg.payload, msg.payload_size);
                output.push_back(msg_copy);
            }
        }
    }
}

void PerfectLink::write_output() {
    // Open output file
    std::ofstream outfile(output_path);
    if (!outfile.is_open()) {
        throw std::runtime_error("Failed to open output file");
    }

    // Write each message to the output file
    for (const auto& msg : output) {
        // TODO: Don't assume type
        size_t message;
        memcpy(&message, msg.payload, msg.payload_size);
        if (type == SENDER) {
            outfile << "b " << message << std::endl;
        } else {
            outfile << "d " << msg.send_id << " " << message << std::endl;
        }
    }

    // Close output file
    outfile.close();
}