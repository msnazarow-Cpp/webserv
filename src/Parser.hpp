//
// Created by Shelley Gertrudis on 8/12/21.
//

#ifndef C_SOCKET_SERVER_GROUP__PARSER_HPP
#define C_SOCKET_SERVER_GROUP__PARSER_HPP

#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include "Location.hpp"

class Server;
class ServerBlock;
class Parser {
private:
    std::vector<ServerBlock> blocks;
    // Server *server;
public:
    class ParserNotValidException:std::exception{};
    Parser(char *confFileName, Server *server);
    bool check_block(ServerBlock & block);
    std::string getfilename(std::string server_name, int port, std::string request);
};


#include "Server.hpp"
#include "ServerBlock.hpp"
#endif //C_SOCKET_SERVER_GROUP__PARSER_HPP
