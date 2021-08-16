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
    Port(int port): port(port)
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
    
    void addServerBlock(ServerBlock block)
    {
        for (std::set<std::string>::iterator it = block.server_name.begin(); it != block.server_name.end(); it++)
        {
            servers.insert(std::pair<std::string, ServerBlock>(*it, block));
            //std::cout << "Block " << *it << " has been added to map. Pars: " << block << "$$$\n";
        }
    }
    
    std::map<std::string, ServerBlock> &getMap()
    {
        return (servers);
    }
    
    int getPort()
    {
        return (port);
    }
};

#endif
