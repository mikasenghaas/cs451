#include "utils.hpp"

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <stdexcept>
#include <netdb.h>

void get_addr(const std::string &ip_or_hostname, unsigned short port, struct sockaddr_in &addr) {
    // Clear address structure
    std::memset(&addr, 0, sizeof(addr));

    // Setup server address structure
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Convert hostname to IP address
    struct hostent *host = gethostbyname(ip_or_hostname.c_str());
    if (host == nullptr) {
        throw std::runtime_error("Failed to resolve hostname");
    }

    std::memcpy(&addr.sin_addr, host->h_addr, host->h_length);
}