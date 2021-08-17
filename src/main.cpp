#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>



//#include <functional>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "Server.hpp"
#include "ServerBlock.hpp"

//clang++ main.cpp -std=c++11 ServerBlock.cpp Location.cpp Parser.cpp -o ft

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main (int argc, char *argv[])
{
    char *arg = NULL; //TODO Что тут происходит - непонятно ?
    if (argc == 1)
    {
        struct stat info;
        if(!(!stat("./default.conf", &info) && !S_ISDIR(info.st_mode)))
        {
            std::cout << "Please, provide config-file as argument or place 'default.conf' near executable." << std::endl;
            return (-1);
        }
    }
    else
       arg = argv[1];
    
    Server *server = new Server();
    Parser *parser;
    try {
        parser = new Parser(arg, server);
        server->setParser(parser);
    } catch (Exception &e) {
        std::cout << e.what() << std::endl << "Exception during config parsing. Server stopped." << std::endl;
        return (-1);
    }

    while (1)
    {
        //std::cout << "Server waiting...\n";
        server->refillSets();
        int ret = server->selector();
        //std::cout << "RET = " << ret << "\n";
        if (ret < 0)
        {
            std::cout << ret << ": Select error, skip cycle\n";
            server->cleaner();
            //usleep(100000);
            continue ;
        }
        if (!ret)
            continue ;
        //std::cout << "\nCHECK PORTS\n";
        server->handleConnections();
        //std::cout << "\nCHECK CLIENTS\n";
        server->readRequests();
        //std::cout << "\nCHECK REMOVALS\n";
        server->remove();
        //std::cout << "\nCHECK ANSWERS\n";
        server->sendAnswer();
        //std::cout << "\nCHECK REMOVALS\n";
        server->remove();
        
    }
}

#pragma clang diagnostic pop
