#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP
#include <iostream>
#include <map>
#include "Location.hpp"
#include <vector>
#include <set>

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
    waitForLocationParams
};
class ServerBlock
{
public:
    ServerBlock():status(clean),server_name(),listen(),error_page(),root(),locations(),client_max_body_size(-1),index(){}
    friend class Parser;

private:

    Status status;
    std::set<std::string> server_name;
    std::set<int> listen;
    std::map<int,std::string> error_page;
    std::string root;
    std::vector<Location> locations;
    size_t client_max_body_size;
    std::vector<std::string>index;
    friend bool operator ==(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator !=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator <(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator <=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator >(const ServerBlock& lhs, const ServerBlock& rhs);
    friend bool operator >=(const ServerBlock& lhs, const ServerBlock& rhs);
    friend std::ostream& operator<<(std::ostream &os, const ServerBlock& d);
};

std::ostream& operator<<(std::ostream &os, const ServerBlock& d);

#endif
