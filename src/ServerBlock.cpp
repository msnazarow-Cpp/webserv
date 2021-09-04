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
    if (bufferDir.empty() || uploads_directory.empty())
        return (false);
    struct stat info;
    std::string tmp;
    if (bufferDir[0] == '.') {
        std::string tmp2 = bufferDir.substr(1, bufferDir.size() - 1);
        bufferDir.clear();
        bufferDir = tmp2;
        tmp2.clear();
        tmp = (root + bufferDir);
        bufferRoot = "1";
    }
    else {
        tmp = bufferDir;
        bufferRoot = "0";
    }
    mkdir(tmp.c_str(), 0777);

    if(!(!stat(tmp.c_str(), &info) && (info.st_mode & S_IFDIR)))
        return (false);

    if (uploads_directory[0] == '.') {
        std::string tmp2 = uploads_directory.substr(1, uploads_directory.size() - 1);
        uploads_directory.clear();
        uploads_directory = tmp2;
        tmp2.clear();
        tmp = (root + uploads_directory);
        uploadsRoot = "1";
    }
    else {
        tmp = uploads_directory;
        uploadsRoot = "0";
    }
    mkdir(tmp.c_str(), 0777);

    if(!(!stat(tmp.c_str(), &info) && (info.st_mode & S_IFDIR)))
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
        :status(clean),server_name(),listen(),error_page(),root(),uploads_directory(""),locations(),client_max_body_size(-1),index(),bufferDir(""),autoindex(),getTry(false),domainRedirect(""){
}

ServerBlock::~ServerBlock()
{
    server_name.clear();
    listen.clear();
    methods.clear();
    error_page.clear();
    root.clear();
    uploads_directory.clear();
    locations.clear();
    try_files.clear();
    index.clear();
    bufferDir.clear();
    bufferRoot.clear();
    uploadsRoot.clear();
    domainRedirect.clear();
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

std::string &ServerBlock::getIsBufferRoot()
{
    return (bufferRoot);
}

std::string &ServerBlock::getIsUploadsRoot()
{
    return (uploadsRoot);
}

bool ServerBlock::redirIsTemp()
{
    return redirectIsTemp;
}

std::string &ServerBlock::getRedirect()
{
    return domainRedirect;
}