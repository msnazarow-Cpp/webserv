//
// Created by Shelley Gertrudis on 8/12/21.
//

#include <algorithm>
#include "Parser.hpp"
#include "IndexHtmlMaker.hpp"
#include <sys/stat.h>
#ifndef IP
    #define IP "localhost"
#endif
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
            if (block.locations[i].autoindex == nil) {
                block.locations[i].autoindex = block.autoindex;
            }
            if (block.locations[i].client_max_body_size == (size_t)-1) {
                block.locations[i].client_max_body_size = block.client_max_body_size;
            }
            if (block.locations[i].methods.empty()) {
                block.locations[i].methods = block.methods;
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
    size_t ret;
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
                            if (tmp.server_name.empty())
                                tmp.server_name.insert(IP);
                            blocks.push_back(tmp);
                            blocks.back().fillPorts(server);
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
                    } else if (str == "error_page"){
                        tmp.status = waitForErrorPageNumber;
                    } else if (str == "client_max_body_size"){
                        tmp.status = waitForRootClientMaxBodySize;
                    } else if (str == "autoindex"){
                        tmp.status = waitForRootAutoIndex;
                    } else if (str == "method"){
                        tmp.status = waitForRootMethod;
                    } else if (str == "try_files"){
                        tmp.status = waitForServerTryFiles;
                    } else if (str == "uploads_directory"){
                        tmp.status = waitForUploadsDirectory;
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
                        tmp.status = waitForLocationMethod;
                    } else if (str == "autoindex"){
                        tmp.status = waitForLocationAutoIndex;
                    } else if (str == "cgi_pass") {
                        tmp.status = waitForCgi;
                    } else if (str == "client_max_body_size"){
                        tmp.status = waitForLocationClientMaxBodySize;
                    } else if (str == "try_files"){
                        tmp.status = waitForLocationTryFiles;
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForListen : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    char *check;
                    ret = strtol(str.c_str(), &check, 10);
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
                            loc.root = str.substr(0,str.size() - 1);
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
                        str.erase(str.end() - 1);
                    } 
                    tmp.index.push_back(str);
                    break;
                }
                case waitForLocationIndex : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    loc.index.push_back(str);
                    break;
                }
                case waitForLocationMethod : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    if (str == "GET"){
                        loc.methods.insert(GET);
                        //std::cout << "GET INSERTED\n";
                    }else if (str == "POST") {
                        loc.methods.insert(POST);
                        //std::cout << "POST INSERTED\n";
                    } else if (str == "DELETE") {
                        loc.methods.insert(DELETE);
                        //std::cout << "DELETE INSERTED\n";
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForRootMethod : {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    if (str == "GET"){
                        tmp.methods.insert(GET);
                        //std::cout << "GET INSERTED\n";
                    }else if (str == "POST") {
                        tmp.methods.insert(POST);
                        //std::cout << "POST INSERTED\n";
                    } else if (str == "DELETE") {
                        tmp.methods.insert(DELETE);
                        //std::cout << "DELETE INSERTED\n";
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForLocationAutoIndex: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    if ( str == "on" ){
                        loc.autoindex = static_cast<boolPlusNil>(true);
                    } else if ( str == "off"){
                        loc.autoindex = static_cast<boolPlusNil>(false);
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForRootAutoIndex: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    if ( str == "on" ){
                        tmp.autoindex = static_cast<boolPlusNil>(true);
                    } else if ( str == "off"){
                        tmp.autoindex = static_cast<boolPlusNil>(false);
                    } else {
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForCgi: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    loc.cgi_pass = str;
                }
                    break;
                case waitForLocationClientMaxBodySize: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    char *check;
                    ret = strtol(str.c_str(), &check, 10);
                    if (check == str.c_str() + str.size())
                        loc.client_max_body_size = ret;
                    else{
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForRootClientMaxBodySize: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    char *check;
                    ret = strtol(str.c_str(), &check, 10);
                    if (check == str.c_str() + str.size())
                        tmp.client_max_body_size = ret;
                    else{
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForErrorPageNumber: {
                    char *check;
                    ret = strtol(str.c_str(), &check, 10);
                    if (check == str.c_str() + str.size()) {
                        tmp.error_page[ret];
                        tmp.status = waitForErrorPage;
                    } else{
                        throw ParserNotValidException();
                    }
                    break;
                }
                case waitForErrorPage: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    tmp.error_page[ret] = str;
                    break;
                }
                case waitForServerTryFiles: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    tmp.try_files.push_back(str);
                    break;
                }
                case waitForLocationTryFiles: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForLocationParams;
                        str.erase(str.end() - 1);
                    }
                    loc.try_files.push_back(str);
                    break;
                }
                case waitForUploadsDirectory: {
                    if (str[str.size() - 1] == ';')
                    {
                        tmp.status = waitForServerParams;
                        str.erase(str.end() - 1);
                    }
                    tmp.uploads_directory = str;
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
    if (blocks.empty())
        throw  ParserNotValidException();
}
bool unorderIsPrefix( std::string const& lhs, std::string const& rhs )
{
    return std::equal(
            lhs.begin(),
            lhs.begin() + rhs.size(),
            rhs.begin());
}

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


std::string Parser::getfilename(std::string server_name, int port, std::string request, bool &isErrorPage, std::string &cgi, bool &isLegit, int requestType, int &code, int &maxSize) {
    std::string out;
    struct stat statbuf;
    
    //std::cout << "SEARCH: " << server_name << " | " << port << " | " << request << "\n";
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].server_name.count(server_name) || !blocks[i].listen.count(port))
            continue;
        ServerBlock block = blocks[i];
        for (size_t j = 0; j < block.locations.size(); ++j) {
            Location loc = block.locations[j];
            if (unorderIsPrefix(request, loc.location[0]) || (loc.location.size() > 1 &&
            hasEnding(request, loc.location[1]))){
                out = loc.root + request;
                if (stat(out.c_str(), &statbuf)) {//TODO Теперь если try_files поместить в конфиге в самую первую локацию, cgi не будут работать - срабатывает строка 439, и проверка не переходит к следующей локации
                    for (size_t k = 0; k < loc.try_files.size(); ++k) {
                        if (hasEnding(request, loc.try_files[k]))
                        {
                            break;
                        }
                        out = getfilename(server_name, port, request + loc.try_files[k], isErrorPage, cgi, isLegit, requestType, code, maxSize);
                        if (code != 404) {
                            isErrorPage = false;
                            return (out);
                        }
                    }
                    isErrorPage = true;
                    code = 404;
                    /*if (block.error_page.count(404)){
                        return (block.root + '/' + block.error_page[404]);
                    }
                    return "404";*/
                    return (block.getErrorPage(code));
                }
                if (S_ISDIR(statbuf.st_mode)){
                    if (request[request.size() - 1] != '/')
                        request.push_back('/');
                    for (size_t k = 0; k < loc.index.size(); ++k) {
                        out = getfilename(server_name, port, request + loc.index[k], isErrorPage, cgi, isLegit, requestType, code, maxSize);
                        if(code != 404) {
                            isErrorPage = false;
                            return (out);
                        }
                        if (loc.autoindex){
                            try{
                                return (IndexHtmlMaker::makeIndexFile(loc.root, request));
                            } catch (...){
                                //return "505";
                                isErrorPage = true;
                                code = 505;
                                return (block.getErrorPage(code));
                            }
                        } else {
                            code = 403;
                            isErrorPage = true;
                            /*if (block.error_page.count(403)){
                                return block.root + '/' + block.error_page[403];
                            }
                            else
                                return "403";*/
                            return (block.getErrorPage(code));
                        }
                    }
                }
                isErrorPage = false;
                cgi = loc.cgi_pass; // зачем тут геттер?
                maxSize = loc.client_max_body_size;
                //std::cout << "loc methods count = " << loc.methods.size() << "\n";
                switch (requestType) {
                    case 0:{
                        isLegit = false;
                        code = 400;
                        //return
                        break ;
                    }
                    case 1:{
                        if (loc.methods.count(GET))
                            isLegit = true;
                        else
                        {
                            code = 405;
                            isLegit = false;
                            //return
                        }
                        break ;
                    }
                    case 2:{
                        if (loc.methods.count(POST))
                            isLegit = true;
                        else
                        {
                            code = 405;
                            isLegit = false;
                            //return
                        }
                        break ;
                    }
                    case 3:{
                        if (loc.methods.count(DELETE))
                            isLegit = true;
                        else
                        {
                            code = 405;
                            isLegit = false;
                            //return
                        }
                        break ;
                    }
                }
                if (!isLegit)
                    out = block.getErrorPage(code);
                return (out);
            }
        }
        out = block.root + request;
        if (stat(out.c_str(), &statbuf))
        {
            for (size_t k = 0; k < block.try_files.size(); ++k) {
                out = getfilename(server_name, port, request + block.try_files[k], isErrorPage, cgi, isLegit, requestType, code, maxSize);
                if (code != 404) {
                    return (out);
                }
            }
            isErrorPage = true;
            code = 404;
            /*if (block.error_page.count(404)){
                return (block.root + '/' + block.error_page[404]);
            }
            return "404";*/
            return (block.getErrorPage(code));
        }
        if (S_ISDIR(statbuf.st_mode)){
            if (request[request.size() - 1] != '/')
                request.push_back('/');
            for (size_t k = 0; k < block.index.size(); ++k) {
                if (hasEnding(request,block.try_files[i]))
                    break;
                out = getfilename(server_name, port, request + block.index[k], isErrorPage, cgi, isLegit, requestType, code, maxSize);
                if(code != 404){
                    isErrorPage = false;
                    return (out);
                }
            }
            if (block.autoindex){
                try {
                    return (IndexHtmlMaker::makeIndexFile(block.root, request));
                } catch (...) {
                    //return ("505");
                    isErrorPage = true;
                    code = 505;
                    return (block.getErrorPage(code));
                }
            }
            code = 403;
            isErrorPage = true;
            /*if (block.error_page.count(403)){
                return block.root + '/' + block.error_page[403];
            }
            else
                return "403";*/
            return (block.getErrorPage(code));
        }
        isErrorPage = false;
        return (out);
    }
    isErrorPage = true;
    code = 404;
    //return ("404");
    return (blocks[0].getErrorPage(code));
}

size_t Parser::getBlocksCount()
{
    return (blocks.size());
}

