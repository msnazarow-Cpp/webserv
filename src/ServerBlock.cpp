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
    /*size_t pos1 = 0, pos2 = 0;
    while ((pos1 = root.find("/", pos2)) != std::string::npos)
        pos2 = pos1 + 1;
    bufferDir = "/goinfre/jnoma/buffers/" + root.substr(pos2, root.size() - pos2) + "/.buffer";
    std::cout << "Temp buffer dir: " << bufferDir << "\n";*/
    const char *tmp = bufferDir.c_str();
    mkdir(tmp, 0777);
    struct stat info;

    if(!(!stat(tmp, &info) && (info.st_mode & S_IFDIR)))
        return (false);
    
    //uploadDir = root + "/uploads";
    //const char *tmp2 = uploadDir.c_str();

    const char *tmp2 = (root + uploads_directory).c_str();
    //std::cout << "DIR: " << tmp2 << "\n";
    mkdir(tmp2, 0777);

    if(!(!stat(tmp2, &info) && (info.st_mode & S_IFDIR)))
        return (false);
    return (true);
}

void ServerBlock::fillPorts(Server *server)
{
    Port *newport;
    for (std::set<size_t>::iterator it = listen.begin(); it != listen.end(); it++)
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
        :status(clean),server_name(),listen(),error_page(),root(),locations(),client_max_body_size(-1),index(),autoindex(){
}

std::string ServerBlock::getErrorPage(size_t val)
{
    std::map<size_t, std::string>::iterator result = error_page.find(val);
    if (result == error_page.end())
        return ("");
    return (root + "/" + result->second);
}

std::string ServerBlock::getUploadsDir(){
    return (uploads_directory);
}