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
public:
    class ParserNotValidException:std::exception{};
    Parser(char *confFileName, Server *server);
    static bool check_block(ServerBlock & block);
    std::string getfilename(std::string server_name, int port, std::string request, bool &isErrorPage, std::string &cgi, bool &isLegit, int requestType, int &code, int &maxSize, std::string directory, bool chunked, Location *locMethod);
    size_t getBlocksCount();
};


#include "Server.hpp"
#include "ServerBlock.hpp"
#endif //C_SOCKET_SERVER_GROUP__PARSER_HPP
