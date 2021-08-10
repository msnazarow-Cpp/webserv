#ifndef PORT_HPP
#define PORT_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "Exception.hpp"
#include "ServerBlock.hpp"

class Port{
private:
    std::vector<int *> clients;
    std::map<std::string, ServerBlock *> servers;
    int descriptor;
    
public:
    Port(int port)
    {
        descriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (descriptor < 0)
            throw Exception("Socket creation exception");
        sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htons(INADDR_ANY);
        addr.sin_port = htons(port);
        if (bind(descriptor, (sockaddr *)&addr, sizeof(addr)) < 0)
            throw Exception("Bind to port/ip exception");
        if (listen(descriptor, SOMAXCONN) < 0)
            throw Exception("Listening exception");
    }
    
    void addClient(int &client)
    {
        clients.push_back(&client);
    }
    
    int &getDescriptor()
    {
        return (descriptor);
    }
    
    void addServerBlock(ServerBlock &block)
    {
        servers.insert(std::pair<std::string, ServerBlock *>(block.getHost(), &block));
    }
    
    std::map<std::string, ServerBlock *> &getMap()
    {
        return (servers);
    }
};

#endif