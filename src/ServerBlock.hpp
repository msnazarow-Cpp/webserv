#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP
#include <iostream>
#include <map>

struct location{
    std::string location;
    std::string root;
    std::string index;
    bool autoindex;
    size_t client_max_body_size;
    std::string fastcgi_pass;
    std::string fastcgi_params;
};

class ServerBlock
{

private:
    int status;
    friend class Parser;
    std::vector<std::string> server_name;
    std::vector<int> listen;
    std::map<int,std::string> error_page;
    std::string root;
    std::vector<location> locations;
    size_t client_max_body_size;

    friend bool operator ==(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name == rhs.server_name;
    }
    friend bool operator !=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name != rhs.server_name;
    }
    friend bool operator <(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name < rhs.server_name;
    }
    friend bool operator <=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name <= rhs.server_name;
    }
    friend bool operator >(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name > rhs.server_name;
    }
    friend bool operator >=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.server_name >= rhs.server_name;
    }
    
public:
//    ServerBlock(){}
//    ServerBlock(std::string host, std::string root):server_name(host), root(root){}
////
////    std::string getHost()
////    {
////        return server_name;
////    }
////    std::string getRoot()
////    {
////        return root;
////    }
    
};


#endif
