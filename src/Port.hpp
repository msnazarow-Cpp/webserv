#ifndef PORT_HPP
#define PORT_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "Exception.hpp"
#include "ServerBlock.hpp"

class Port{
private:
    std::vector<int *> clients;
    std::map<std::string, ServerBlock> servers;
    int descriptor;
    int port;
    
public:
    Port(int port);
    void addClient(int &client);
    int &getDescriptor();
    void addServerBlock(ServerBlock block);
    std::map<std::string, ServerBlock> &getMap();
    int getPort();
};

#endif
