//
// Created by Shelley Gertrudis on 8/12/21.
//

#include <algorithm>
#include "Parser.hpp"
#include "Extention.hpp"
#include <sys/stat.h>

Parser::Parser(char *confFileName) {
    std::fstream file(confFileName);
    std::stringstream lines;
    std::string str;
    ServerBlock tmp;
    Location loc;
    if (file.fail()){
        std::cout << "WARRIES" << std::endl;
        throw ParserNotValidException();
    }
    while (file >> str)
    {
        if (str.substr(0, 1) == "#")
            continue;
        else if  (str == "server") {
            if (tmp.status == clean) {
                tmp.status = waitForServer;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "{") {
            if (tmp.status == waitForServer) {
                tmp.status = waitForServerParams;
            } else if (tmp.status == waitForLocation){
                tmp.status = waitForLocationParams;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "}"){
            if (tmp.status == waitForServerParams){
                tmp.status = clean;
                blocks.push_back(tmp);
                tmp = ServerBlock();
            } else if (tmp.status == waitForLocationParams){
                tmp.status = waitForServerParams;
                tmp.locations.push_back(loc);
                loc = Location();
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "listen") {
            if (tmp.status == waitForServerParams){
                tmp.status = waitForListen;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "server_name") {
            if (tmp.status == waitForServerParams){
                tmp.status = waitForServerName;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "root"){
            if (tmp.status == waitForServerParams){
                tmp.status = waitForServerRoot;
            } else if (tmp.status == waitForLocationParams){
                tmp.status = waitForLocationRoot;
            } else {
                throw ParserNotValidException();
            }
        } else if (str == "index") {
            if (tmp.status == waitForServerParams){
                tmp.status = waitForServerIndex;
            } else if (tmp.status == waitForLocationParams){
                tmp.status = waitForLocationIndex;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == "location"){
            if (tmp.status == waitForServerParams){
                tmp.status = waitForLocation;
            } else {
                throw ParserNotValidException();
            }
        }
        else if (str == ";")
        {
            if (tmp.status >= 3 && tmp.status <= 5)
                tmp.status = waitForServerParams;
            else if (tmp.status >= 8){
                tmp.status = waitForLocationParams;
            }{
                throw ParserNotValidException();
            }
        }
        else if (tmp.status == waitForListen) {
            if (str[str.size() - 1] == ';')
            {
                tmp.status = waitForServerParams;
                char *check;
                std::string buf = str.substr(0,str.size() - 1);
                int ret = strtol(buf.c_str(), &check, 10);
                if (check == buf.c_str() + str.size() - 1)
                    tmp.listen.insert(ret);
                else{
                    throw ParserNotValidException();
                }
            } else {
                char *check;
                const char *buf = str.c_str();
                int ret = strtol(buf, &check, 10);
                if (check == buf + str.size())
                    tmp.listen.insert(ret);
                else{
                    throw ParserNotValidException();
                }
            }
        } else if (tmp.status == waitForServerName){
            if (str[str.size() - 1] == ';')
            {
                tmp.status = waitForServerParams;
                tmp.server_name.insert(str.substr(0,str.size() - 1));
            } else {
                tmp.server_name.insert(str);
            }
        } else if (tmp.status == waitForServerRoot){
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
        } else if (tmp.status == waitForLocationRoot){
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
        } else if (tmp.status == waitForLocation) {
            loc.location.push_back(str);
        } else if (tmp.status == waitForServerIndex) {
            if (str[str.size() - 1] == ';')
            {
                tmp.status = waitForServerParams;
                tmp.index.push_back(str.substr(0,str.size() - 1));
            } else {
                tmp.index.push_back(str);
            }
        } else if (tmp.status == waitForLocationIndex) {
            if (str[str.size() - 1] == ';')
            {
                tmp.status = waitForLocationParams;
                loc.index.push_back(str.substr(0,str.size() - 1));
            } else {
                loc.index.push_back(str);
            }
        }

    }
}
bool
unorderIsPrefix( std::string const& lhs, std::string const& rhs )
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

    for (int i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].server_name.count(server_name) || !blocks[i].listen.count(port))
            continue;
        ServerBlock block = blocks[i];
        if (!block.locations.empty()){
            for (int j = 0; j < block.locations.size(); ++j) {
                Location loc = block.locations[j];
                if (unorderIsPrefix(request, loc.location[0]) || (loc.location.size() > 1 &&
                hasEnding(request, loc.location[1]))){
                    out = loc.root + request;
                    if (stat(out.c_str(), &statbuf))
                        return "404";
                    if (S_ISDIR(statbuf.st_mode))
                        for (int k = 0; k < loc.index.size(); ++k) {
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
                    for (int k = 0; k < loc.index.size(); ++k) {
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
                for (int k = 0; k < block.index.size(); ++k) {
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
