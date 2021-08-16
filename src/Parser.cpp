//
// Created by Shelley Gertrudis on 8/12/21.
//

#include <algorithm>
#include "Parser.hpp"
#include "Extention.hpp"
#include <sys/stat.h>

bool Parser::check_block(ServerBlock &block) {
    if (!block.locations.empty()){
        for (size_t i = 0; i < block.locations.size(); ++i) {
            if (block.locations[i].root.empty()){
                if (block.root.empty())
                    throw ParserNotValidException();
                else
                    block.locations[i].root = block.root;
            }
            for (size_t j = 0; j < block.index.size(); ++j) {
                block.locations[i].index.push_back(block.index[j]);
            }
        }
    } else {
        if (block.root.empty()){
            throw ParserNotValidException();
        }
    }
    return false;
}

Parser::Parser(char *confFileName, Server *server) {
    std::fstream file(confFileName);
    std::stringstream lines;
    std::string str;
    ServerBlock tmp;
    Location loc;
    if (file.fail()){
        std::cout << "WARRIES" << std::endl;
        throw ParserNotValidException();
    }
    while (std::getline(file, str))
    {
        if (str.substr(0, 1) == "#")
            continue;
        std::stringstream ss;
        ss << str;
        while (ss >> str){
            switch (tmp.status) {
                case clean : {
                    if (str == "server") {
                        tmp.status = waitForServer;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForServer : {
                    if (str == "{"){
                        tmp.status = waitForServerParams;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForLocation : {
                    if (str == "{"){
                        tmp.status = waitForLocationParams;
                    } else {
                        loc.location.push_back(str);
                    }
                    break;
                }
                case waitForServerParams : {
                    if (str == "}") {
                        tmp.status = clean;
                        check_block(tmp);
                        if (tmp.createDirs())
                        {
                            blocks.push_back(tmp);
                            blocks[blocks.size() - 1].fillPorts(server);
                        }
                        else
                            std::cout << "Block " << tmp << "has been excluded from set because of error\n";
                        tmp = ServerBlock();
                    } else if (str == "listen") {
                        tmp.status = waitForListen;
                    } else if (str == "server_name") {
                        tmp.status = waitForServerName;
                    } else if (str == "root"){
                        tmp.status = waitForServerRoot;
                    } else if (str == "index") {
                        tmp.status = waitForServerIndex;
                    } else if (str == "location"){
                        tmp.status = waitForLocation;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForLocationParams : {
                    if (str == "}") {
                        tmp.status = waitForServerParams;
                        tmp.locations.push_back(loc);
                        loc = Location();
                    } else if (str == "root"){
                        tmp.status = waitForLocationRoot;
                    } else if (str == "index"){
                        tmp.status = waitForLocationIndex;
                    } else if (str == "method"){
                        tmp.status = waitForMethod;
                    } else if (str == "autoindex"){
                        tmp.status = waitForAutoIndex;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForListen : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.pop_back();
                    }
                    char *check;
                    size_t ret = strtol(str.c_str(), &check, 10);
                    if (check == str.c_str() + str.size())
                        tmp.listen.insert(ret);
                    else{
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForServerName : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        tmp.server_name.insert(str.substr(0,str.size() - 1));
                    } else {
                        tmp.server_name.insert(str);
                    }
                    break;
                }
                case waitForServerRoot : {
                    if (!tmp.root.empty())
                        throw ParserNotValidException();
                    else {
                        if (str[str.size() - 1] == ';')
                        {
                            tmp.status = waitForServerParams;
                            tmp.root = str.substr(0,str.size() - 1);
                        } else {
                            tmp.root = str;
                        }
                    }
                    break;
                }
                case waitForLocationRoot : {
                    if (!loc.root.empty())
                        throw ParserNotValidException();
                    else {
                        if (str[str.size() - 1] == ';'){
                            tmp.status = waitForLocationParams;
                            loc.root = str.substr(0,str.size() - 1).c_str();
                        } else {
                            loc.root = str;
                        }
                    }
                    break;
                }
                case waitForServerIndex : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.pop_back();
                    } 
                    tmp.index.push_back(str);
                    break;
                }
                case waitForLocationIndex : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.pop_back();
                    }
                    loc.index.push_back(str);
                    break;
                }
                case waitForMethod : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.pop_back();
                    }
                    if (str == "GET")
                        loc.methods.insert(GET);
                    else if (str == "POST") {
                        loc.methods.insert(POST);
                    } else if (str == "DELETE") {
                        loc.methods.insert(DELETE);
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForAutoIndex: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.pop_back();
                    }
                    if ( str == "on" ){
                        loc.autoindex = true;
                    } else if ( str == "off"){
                        loc.autoindex = false;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
            }
            if (str == ";")
            {
                if (tmp.status >= 3 && tmp.status <= 5)
                    tmp.status = waitForServerParams;
                else if (tmp.status >= 8){
                    tmp.status = waitForLocationParams;
                } else {
                    throw ParserNotValidException();
                }
            }
        }
    }
}
bool unorderIsPrefix( std::string const& lhs, std::string const& rhs )
{
    return std::equal(
            lhs.begin(),
            lhs.begin() + std::min( lhs.size(), rhs.size() ),
            rhs.begin() );
}

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


std::string Parser::getfilename(std::string server_name, int port, std::string request) {
    std::string out;
    struct stat statbuf;

    for (size_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].server_name.count(server_name) || !blocks[i].listen.count(port))
            continue;
        ServerBlock block = blocks[i];
        if (!block.locations.empty()){
            for (size_t j = 0; j < block.locations.size(); ++j) {
                Location loc = block.locations[j];
                if (unorderIsPrefix(request, loc.location[0]) || (loc.location.size() > 1 &&
                hasEnding(request, loc.location[1]))){
                    out = loc.root + request;
                    if (stat(out.c_str(), &statbuf))
                        return "404";
                    if (S_ISDIR(statbuf.st_mode))
                        for (size_t k = 0; k < loc.index.size(); ++k) {
                            if((out = getfilename(server_name, port, loc.location[0] + loc.index[k])) != "404"){
                                return (out);
                            }
                        }
                    return (out);
                }
            }
            Location loc = block.locations[0];
            if (unorderIsPrefix(request, loc.location[0]) || (loc.location.size() > 1 &&
            hasEnding(request, loc.location[1]))){
                out = loc.root + request;
                if (stat(out.c_str(), &statbuf))
                    return "404";
                if (S_ISDIR(statbuf.st_mode))
                    for (size_t k = 0; k < loc.index.size(); ++k) {
                        if((out = getfilename(server_name, port, loc.index[k])) != "404"){
                            return (out);
                        }
                    }
                return (out);
            }
        } else {
            out = block.root + request;
            if (stat(out.c_str(), &statbuf))
                return "404";
            if (S_ISDIR(statbuf.st_mode))
            {
                for (size_t k = 0; k < block.index.size(); ++k) {
                    if((out = getfilename(server_name, port, block.index[k])) != "404"){
                        return (out);
                    }
                }
                return ("403");
            }
            return (out);
        }
    }

    return ("404");
}
