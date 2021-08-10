#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "Server.hpp"

#define PORT1 1002
#define PORT2 1003

//clang++ main.cpp -o ft


int main (int argc, char *argv[])
{
    Server *server = new Server();
    
    
    Port *port_sock = new Port(PORT1);
    Port *port_sock2 = new Port(PORT2);
    server->addPort(port_sock);
    server->addPort(port_sock2);
    
    ServerBlock ser1("192.168.24.34", "./www/server1");
    ServerBlock ser2("am-c4.msk.21-school.ru", "./www/server2");
    ServerBlock ser3("127.0.0.1", "./www/server3");
    port_sock->addServerBlock(ser1);
    port_sock->addServerBlock(ser2);
    port_sock->addServerBlock(ser3);
    port_sock2->addServerBlock(ser2);
    

    while (1)
    {
        std::cout << "Server waiting...\n";
        server->refillSets();
        int ret = server->selector();
        std::cout << "RET = " << ret << "\n";
        if (ret <= 0)
        {
            std::cout << ret << ": Select error, skip cycle\n";
            continue ;
        }
        std::cout << "\nCHECK PORTS\n";
        server->handleConnections();
        std::cout << "\nCHECK CLIENTS\n";
        server->readRequests();
        //std::cout << "\nCHECK REMOVALS\n";
        //server->remove();
        std::cout << "\nCHECK ANSWERS\n";
        server->sendAnswer();
        std::cout << "\nCHECK REMOVALS\n";
        server->remove();
        
    }
}
