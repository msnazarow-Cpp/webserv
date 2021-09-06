#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "Server.hpp"
#include "Colors.h"

#ifndef FDCOUNT
    #define FDCOUNT 1024
#endif

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
//int Client::count = 0;
//int Client::active = Client::count;

bool isWorking;

void signal_handler(int num)
{
    if (num == SIGQUIT)
        isWorking = false;
}

int main (int argc, char *argv[])
{
    char *arg;
    if (argc == 1) {
        arg = const_cast<char *>("./default.conf");
        struct stat info;
        if(!(!stat(arg, &info) && !S_ISDIR(info.st_mode))) {
            std::cout << "Please, provide config-file as argument or place 'default.conf' near executable." << std::endl;
            exit(2);
        }
    }
    else
       arg = argv[1];

    Server *server = new Server();
    Parser *parser;
    try {
        parser = new Parser(arg, server);
        const std::vector<ServerBlock> &blocks = parser->getBlocks();
        for (size_t i = 0; i < blocks.size(); i++) {
            std::cout << GREEN << blocks[i] << DEFAULT << std::endl;
        }
    } catch (Parser::NoValidServerBlockExeption &e) {
        std::cout << RED << "Exception: Each server block in config is not valid\nPlease correct or use another config." << DEFAULT << std::endl;
        exit (1);
    } catch (Parser::ParserNotValidException &e) {
        std::cout << RED << "Exception: Config not valid: " << std::endl << e.what() << DEFAULT << std::endl;
        exit (1);
    }

    try {
        server->setParser(parser);
    } catch (Exception &e) {
        std::cout << e.what() << std::endl << "Exception during config parsing. Server stopped." << std::endl;
        exit (1);
    }
    if (!parser->getBlocksCount()) {
        std::cout << "There are no active servers. Program stopped." << std::endl;
        exit (1);
    }
    isWorking = true;
    if (signal(SIGQUIT, signal_handler) == SIG_ERR) {
        std::cout << "Error on signal init. Server stopped." << std::endl;
        isWorking = false;
    }
    else
        std::cout << "Server is working..." << std::endl;
    struct rlimit limits_old;
    if (!getrlimit(RLIMIT_NOFILE, &limits_old) && limits_old.rlim_cur < FDCOUNT) {
        struct rlimit limits_new;
        limits_new.rlim_cur = FDCOUNT;
        limits_new.rlim_max = FDCOUNT;
        setrlimit(RLIMIT_NOFILE, &limits_new);
    }

    while (isWorking) {
        //std::cout << "CLIENT TOTAL = " << Client::count << " | ACTIVE: " << Client::active << "\n";
        //std::cout << "Server waiting...\n";

        try {
            server->refillSets();
            int ret = server->selector();
            if (ret < 0) {
                //std::cout << ret << ": Select error, skip cycle\n";
                server->cleaner();
                continue ;
            }
            if (!ret)
                continue ;
            server->handleConnections();
            server->readRequests();
            server->sendAnswer();
        }
        catch (const std::bad_alloc& ex) {
            server->cleaner();
        }
    }
    std::cout << "Server is shutting down..." << std::endl;
    delete server;
    delete parser;
    setrlimit(RLIMIT_NOFILE, &limits_old);
    exit(0);
}

#pragma clang diagnostic pop
