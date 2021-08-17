//
// Created by msnazarow on 14.08.2021.
//
#include "ServerBlock.hpp"
#include <ostream>
#include "Extention.hpp"

bool operator==(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name == rhs.server_name;
}

bool operator!=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name != rhs.server_name;
}

bool operator<(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name < rhs.server_name;
}

bool operator<=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name <= rhs.server_name;
}

bool operator>(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name > rhs.server_name;
}

bool operator>=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name >= rhs.server_name;
}

std::ostream &operator<<(std::ostream &os, const ServerBlock &d) {
    return os << d.server_name << " " << d.root << " " << d.error_page << " " <<
              d.listen << " " << d.client_max_body_size << " " << d.index << std::endl << d.locations << std::endl;}

bool ServerBlock::createDirs()
{
    bufferDir = root + "/.buffer";
    const char *tmp = bufferDir.c_str();
    mkdir(tmp, 0777);
    struct stat info;

    if(!(!stat(tmp, &info) && (info.st_mode & S_IFDIR))) //TODO что именно тут происходит?
        return (false);
    
    uploadDir = root + "/uploads";
    const char *tmp2 = uploadDir.c_str();
    mkdir(tmp2, 0777);

    if(!(!stat(tmp2, &info) && (info.st_mode & S_IFDIR)))
        return (false);
    return (true);
}

void ServerBlock::fillPorts(Server *server)
{
    Port *newport;
    for (std::set<int>::iterator it = listen.begin(); it != listen.end(); it++)
    {
        newport = server->hasPort(*it);
        if (!newport)
        {
            newport = new Port(*it);
            server->addPort(newport);
        }
        //std::cout << "ADDED PORT " << *it << "\n";
        newport->addServerBlock(*this);
        
    }
}

std::string ServerBlock::getRoot()
{
    return (root);
}
std::string ServerBlock::getBuffer()
{
    return (bufferDir);
}

ServerBlock::ServerBlock()
        :status(clean),server_name(),listen(),error_page(),root(),locations(),client_max_body_size(-1),index(){
}
