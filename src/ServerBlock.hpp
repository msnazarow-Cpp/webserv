#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP
#include <iostream>

class ServerBlock
{
private:
    std::string hostname;
    std::string root;
    
    friend bool operator ==(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname == rhs.hostname;
    }
    friend bool operator !=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname != rhs.hostname;
    }
    friend bool operator <(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname < rhs.hostname;
    }
    friend bool operator <=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname <= rhs.hostname;
    }
    friend bool operator >(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname > rhs.hostname;
    }
    friend bool operator >=(const ServerBlock& lhs, const ServerBlock& rhs)
    {
        return lhs.hostname >= rhs.hostname;
    }
    
public:
    ServerBlock(){}
    ServerBlock(std::string host, std::string root):hostname(host), root(root){}
    
    std::string getHost()
    {
        return hostname;
    }
    std::string getRoot()
    {
        return root;
    }
    
};


#endif
