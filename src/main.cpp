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

//clang++ main.cpp -D IP=\"192.168.24.34\" ServerBlock.cpp Location.cpp Parser.cpp -o ft

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int Client::count = 0;
int Client::active = Client::count;
int main (int argc, char *argv[])
{
    char *arg;
    if (argc == 1)
    {
        //std::string tmp("./default.conf"); //TODO Зачем здесь переменная если она нигде не используется? - ниже используется
        arg = const_cast<char *>("./default.conf");
        struct stat info;
        if(!(!stat(arg, &info) && !S_ISDIR(info.st_mode)))
        {
            std::cout << "Please, provide config-file as argument or place 'default.conf' near executable." << std::endl;
            exit(2); //TODO  Чтобы санитайзеры не регистрировали утечку, используй exit для завершения программы, а не return
        }
    }
    else
       arg = argv[1];
    
    //std::cout << "arg = " << arg << "\n";
    
    Server *server = new Server();
    Parser *parser;
    try {
        parser = new Parser(arg, server);
        server->setParser(parser);
    } catch (Exception &e) {
        std::cout << e.what() << std::endl << "Exception during config parsing. Server stopped." << std::endl;
        exit (1);
    }
    if (!parser->getBlocksCount())
    {
        std::cout << "There are no active servers. Program stopped." << std::endl;
        exit (1);
    }
    while (1)
    {
        std::cout << "CLIENT TOTAL = " << Client::count << " | ACTIVE: " << Client::active << "\n";
        //std::cout << "Server waiting...\n";

        try {
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
            {
                //std::cout << "\nCHECK REMOVALS\n";
                //server->remove();
                std::cout << "SELECT TIMEOUT\n";
                continue ;
            }
            //std::cout << "\nCHECK PORTS\n";
            server->handleConnections();
            //std::cout << "\nCHECK CLIENTS\n";
            server->readRequests();
            //std::cout << "\nCHECK REMOVALS\n";
            //server->remove();
            //std::cout << "\nCHECK ANSWERS\n";
            server->sendAnswer();
            //std::cout << "\nCHECK REMOVALS\n";
            //server->remove();
        }
        catch (const std::bad_alloc& ex)
        {
            server->cleaner();
        }
    }
}

#pragma clang diagnostic pop
