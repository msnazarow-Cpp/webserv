//
// Created by Shelley Gertrudis on 8/12/21.
//

#ifndef C_SOCKET_SERVER_GROUP__PARSER_H
#define C_SOCKET_SERVER_GROUP__PARSER_H

#include <vector>
#include <fstream>
#include <sstream>
#include "ServerBlock.hpp"


class Parser {
public:
    static std::vector<ServerBlock> parse(char *confFileName){
        std::fstream file(confFileName);
        std::stringstream lines;
        std::string str;
        ServerBlock tmp;
        tmp.status = 0;
        std::vector<ServerBlock> out;
        if (file.fail()){
            std::cout << "WARRIES" << std::endl;

        }
        while (file >> str)
        {
            if (str.substr(0, 1) == "#")
                continue;
            else if  (str == "server") {
                if (tmp.status == 0) {
                    tmp.status = 1;
                } else {
                    exit(1);
                }
            }
            else if (str == "{") {
                    if (tmp.status == 1) {
                        tmp.status = 2;
                    } else {
                        exit(1);
                    }
                }
            else if (str == "}"){
                if (tmp.status == 2){
                    tmp.status = 0;
                } else {
                    exit(1);
                }
            }
            else if (str == "listen") {
                    if (tmp.status == 2){
                        tmp.status = 3;
                    } else {
                        exit(1);
                    }
                }
            else if (str == "server_name") {
                    if (tmp.status == 2){
                        tmp.status = 4;
                    } else {
                        exit(1);
                    }
                }
            else if (str == "root"){
                if (tmp.status == 2){
                    tmp.status = 5;
                } else {
                    exit (1);
                }
            }
            else if (str == "location"){

            }
            else if (str == ";")
            {
                if (tmp.status >= 3)
                    tmp.status = 2;
                else {
                    exit (1);
                }
            }
            else if (tmp.status == 3) {
                if (str.back() == ';')
                {
                    tmp.status = 2;
                    char *check;
                    const char *buf = str.substr(0,str.size() - 1).c_str();
                    int ret = strtol(buf, &check, 10);
                    if (check == buf + str.size() - 1)
                        tmp.listen.push_back(ret);
                    else{
                        exit (1);
                    }
                } else {
                    char *check;
                    const char *buf = str.c_str();
                    int ret = strtol(buf, &check, 10);
                    if (check == buf + str.size())
                        tmp.listen.push_back(ret);
                    else{
                        exit (1);
                    }
                }

            } else if (tmp.status == 4){
                        if (str.back() == ';')
                        {
                            tmp.status = 2;
                            const char *buf = str.substr(0,str.size() - 1).c_str();
                            tmp.server_name.push_back(buf);
                        } else {
                            tmp.server_name.push_back(str);
                        }
            } else if (tmp.status == 5){
                if (str.back() == ';')
                {
                    tmp.status = 2;
                    tmp.root = str.substr(0,str.size() - 1).c_str();
                } else {
                    tmp.root = str;
                }
            }

        }
    }

};


#endif //C_SOCKET_SERVER_GROUP__PARSER_H
