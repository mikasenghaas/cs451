#include "perfect_link.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

PerfectLink::PerfectLink(const struct sockaddr_in &local_addr): local_addr{local_addr} {
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
}

PerfectLink::~PerfectLink() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

void PerfectLink::send(const PerfectLink::Message &msg) {
    // Setup message
    const char* data = msg.data.c_str();
    size_t data_len = strlen(data);
    int flags = 0;
    const struct sockaddr* recv_addr_ptr = reinterpret_cast<const struct sockaddr*>(&msg.addr);
    socklen_t recv_addr_len = sizeof(msg.addr);

    // Send message
    ssize_t sent = sendto(sockfd, data, data_len, flags, recv_addr_ptr, recv_addr_len);
    
    // Check if message was sent successfully
    if (sent < 0) {
        throw std::runtime_error("Failed to send data");
    } 
}

PerfectLink::Message PerfectLink::receive() {
    // Setup buffer
    const size_t buffer_size = 1024;
    unsigned char *buffer = new unsigned char[buffer_size];
    
    // Setup sender address
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    // Bind socket if not already bound
    int bound = bind(sockfd, reinterpret_cast<struct sockaddr*>(&local_addr), sizeof(local_addr));
    if (bound < 0) {
        delete[] buffer;
        throw std::runtime_error("Failed to bind receiving socket");
    }

    // Receive message
    ssize_t recv_len = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                               reinterpret_cast<struct sockaddr*>(&sender_addr),
                               &sender_addr_len);
    
    if (recv_len < 0) {
        delete[] buffer;
        throw std::runtime_error("Failed to receive data");
    }

    // Null terminate the received data
    buffer[recv_len] = '\0';
    
    // Construct message with received data and sender address
    Message msg{
        std::string(reinterpret_cast<char*>(buffer)),
        sender_addr
    };
    
    // Clean up
    delete[] buffer;
              
    return msg;
}