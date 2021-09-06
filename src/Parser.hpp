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
#include "Server.hpp"
#include "ServerBlock.hpp"
class Parser {
private:
    std::vector<ServerBlock> blocks;
public:
    class ParserNotValidException:std::exception {
        std::string _message;
    public:
        ParserNotValidException() {
        }
        ParserNotValidException(std::string message) {
            this->_message = message;
        }
        virtual const char* what() const throw() {
            return (_message.c_str());
        }
        virtual ~ParserNotValidException() throw() {
        };
    };
    class NoValidServerBlockExeption:std::exception{};
//    class ParserNotValidException:std::exception{};
//    class ParserNotValidException:std::exception{};
//    class ParserNotValidException:std::exception{};
    Parser(char *confFileName, Server *server);
    ~Parser();
    static bool check_block(ServerBlock & block);
    std::string getfilename(std::string server_name, int port, std::string request, bool &isErrorPage, std::string &cgi, bool &isLegit, int requestType, int &code, int &maxSize, std::string directory, bool chunked, Location *locMethod);
    size_t getBlocksCount();
    void checkAcceptedMethod(std::set<Method> &methods, int requestType, bool &isLegit, int &code);
    const std::vector<ServerBlock> &getBlocks();
    void caseWaitForLocationParams(ServerBlock &block, Location &loc, std::string &str);
    void caseWaitForServerParams(ServerBlock &block, std::string &str);
};
#endif //C_SOCKET_SERVER_GROUP__PARSER_HPP
