#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP
#include <sys/stat.h>
#include <iostream>
#include <map>
#include "Location.hpp"
#include <vector>
#include <set>
#include "Exception.hpp"


class Server;
enum Status{
    clean,
    waitForServer,
    waitForServerParams,
    waitForListen,
    waitForServerName,
    waitForServerIndex,
    waitForServerRoot,
    waitForLocationIndex,
    waitForLocationRoot,
    waitForLocation,
    waitForLocationParams,
    waitForMethod,
    waitForAutoIndex,
    waitForCgi
};
class ServerBlock
{
public:
    ServerBlock();
    friend class Parser;
    friend class Port;
    bool createDirs();
    void fillPorts(Server *server);
    std::string getRoot();
    std::string getBuffer();

private:

    Status status;
    std::set<std::string> server_name;
    std::set<int> listen;
    std::map<int,std::string> error_page;
    std::string root;
    std::vector<Location> locations;
    size_t client_max_body_size;
    std::vector<std::string>index;
    std::string bufferDir;
    std::string uploadDir;
    friend bool operator ==(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator !=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator <(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator <=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator >(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator >=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend std::ostream& operator<<(std::ostream &os, const ServerBlock& d);
};

std::ostream& operator<<(std::ostream &os, const ServerBlock& d);

#include "Server.hpp"
#endif
