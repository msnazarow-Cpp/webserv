#include <fcntl.h>
#include <sys/wait.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "Server.hpp"

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
    } catch (Parser::ParserNotValidException &e) {
        std::cout << "Exception during config parsing. Server stopped." << std::endl;
        exit (1);
    }

    try {
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
    isWorking = true;
    if (signal(SIGQUIT, signal_handler) == SIG_ERR)
    {
        std::cout << "Error on signal init. Server stopped." << std::endl;
        isWorking = false;
    }
    else
        std::cout << "Server is working..." << std::endl;
    while (isWorking)
    {
        //std::cout << "CLIENT TOTAL = " << Client::count << " | ACTIVE: " << Client::active << "\n";
        //std::cout << "Server waiting...\n";

        try {
            server->refillSets();
            int ret = server->selector();
            if (ret < 0)
            {
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
        catch (const std::bad_alloc& ex)
        {
            server->cleaner();
        }
    }
    std::cout << "Server is shutting down..." << std::endl;
    delete server;
    delete parser;
    exit(0);
}

#pragma clang diagnostic pop
