#include "Port.hpp"

Port::Port(int port): port(port)
{
    descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0)
        throw Exception("Socket creation exception");
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    int reuse = 1;
    if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        throw Exception("Setsockopt(SO_REUSEADDR) exception");
#ifdef __APPLE__
    if (setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &reuse, sizeof(reuse)) < 0)
        throw Exception("Setsockopt(SO_NOSIGPIPE) exception");
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    if (bind(descriptor, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw Exception("Bind to port/ip exception");
    if (listen(descriptor, SOMAXCONN) < 0)
        throw Exception("Listening exception");

}

void Port::addClient(int &client)
{
    clients.push_back(&client);
}

int &Port::getDescriptor()
{
    return (descriptor);
}

void Port::addServerBlock(ServerBlock block)
{
    for (std::set<std::string>::iterator it = block.server_name.begin(); it != block.server_name.end(); it++)
        servers.insert(std::pair<std::string, ServerBlock>(*it, block));

}

std::map<std::string, ServerBlock> &Port::getMap()
{
    return (servers);
}

int Port::getPort()
{
    return (port);
}
