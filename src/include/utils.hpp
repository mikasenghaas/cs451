#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

void get_addr(const std::string &ip_or_hostname, unsigned short port, struct sockaddr_in &addr);