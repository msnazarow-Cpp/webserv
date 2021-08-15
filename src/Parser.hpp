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
#include "ServerBlock.hpp"
#include "Location.hpp"

class Parser {
private:
    std::vector<ServerBlock> blocks;
public:
    class ParserNotValidException:std::exception{};
    Parser(char *confFileName);
    std::string getfilename(std::string server_name, int port, std::string request);
};



#endif //C_SOCKET_SERVER_GROUP__PARSER_HPP
