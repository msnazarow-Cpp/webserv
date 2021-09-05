#include "Server.hpp"

Server::Server()
{
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
}

Server::~Server()
{
    cleaner();
    clearPorts();
}

void Server::refillSets()
{
    //std::cout << "SET REFILL\n";
    FD_ZERO(&read_current);
    FD_ZERO(&write_current);
    struct timeval curTime;
    gettimeofday(&curTime, 0);
    std::vector<Client *>::iterator itC = allclients.begin();
    while (itC != allclients.end())
    {
        if ((*itC)->getStatus() == -1 || curTime.tv_sec - (*itC)->getTimer().tv_sec >= TIMEOUT)
        {
            if ((*itC)->getFileWrite())
            {
                std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite());
                if (itF != allfiles.end())
                    allfiles.erase(itF);
            }
            if ((*itC)->getFileRead())
            {
                std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileRead());
                if (itF != allfiles.end())
                    allfiles.erase(itF);
            }
            delete (*itC);
            itC = allclients.erase(itC);
            continue ;
        }
        if (!(*itC)->getStatus())
            FD_SET((*itC)->getDescriptor(), &read_current);
        if ((*itC)->getStatus() == 6)
        {
            (*itC)->setStatus(7);
            addFile((*itC)->getFileRead());
        }
        if ((*itC)->getStatus() == 3)
        {
            if (!(*itC)->getFileWrite() || (*itC)->getFileWrite()->getStatus() == 1)
                (*itC)->setStatus(4);
            else if ((*itC)->getFileWrite()->getStatus() == -2)
                (*itC)->setStatus(-1);
        }
        if ((*itC)->getStatus() == 2 || (*itC)->getStatus() == 4)
            FD_SET((*itC)->getDescriptor(), &write_current);
        itC++;
    }
    std::vector<FileUpload *>::iterator itF = allfiles.begin();
    while (itF != allfiles.end())
    {
        if ((*itF)->getStatus() < 0)
        {
            if ((*itF)->getStatus() == -2)
            {
                Client *curclient = (*itF)->getClient();
                itF = allfiles.erase(itF);
                curclient->finishPipe();
            }
            else
                itF = allfiles.erase(itF);
            continue ;
        }
        if (!(*itF)->getStatus())
            FD_SET((*itF)->getDescriptor(), &write_current);
        if ((*itF)->getStatus() == 2)
            FD_SET((*itF)->getDescriptor(), &read_current);
        itF++;
    }
    for (std::vector<Port *>::iterator it = allports.begin(); it != allports.end(); ++it)
        FD_SET((*it)->getDescriptor(), &read_current);
    //std::cout << "SET REFILLED\n";
    //std::cout << "Ports = " << portsCount() << " | Clients = " << clientsCount() << " | Files = " << filesCount() << " | max = " << getLastSock() <<"\n";
}

int Server::getLastSock()
{
    int last_sock = -1;
    int descr;
    for (size_t i = 0; i < allfiles.size(); i++)
    {
        descr = allfiles[i]->getDescriptor();
        if (descr > last_sock)
            last_sock = descr;
    }
    
    for (size_t i = 0; i < allclients.size(); i++)
    {
        descr = allclients[i]->getDescriptor();
        if (descr > last_sock)
            last_sock = descr;
    }
    for (size_t i = 0; i < allports.size(); i++)
    {
        descr = allports[i]->getDescriptor();
        if (descr > last_sock)
            last_sock = descr;
    }
    return (last_sock);
}

int Server::selector()
{
    return (select(getLastSock() + 1, &read_current, &write_current, NULL, &timeout));
}

void Server::addPort(Port *port)
{
    allports.push_back(port);
}

size_t Server::portsCount()
{
    return (allports.size());
}

Port *Server::getPort(size_t ind)
{
    return (allports[ind]);
}

void Server::addClient(Client *client)
{
    allclients.push_back(client);
}

size_t Server::clientsCount()
{
    return (allclients.size());
}

Client *Server::getClient(size_t ind)
{
    return (allclients[ind]);
}

void Server::addFile(FileUpload *file)
{
    allfiles.push_back(file);
}

size_t Server::filesCount()
{
    return (allfiles.size());
}

void Server::handleConnections()
{
    //std::cout << "Handle connection\n";
    Port *curport;
    int descr;
    for (size_t i = 0; i < portsCount(); ++i)
    {
        curport = allports[i];
        descr = curport->getDescriptor();
        if (isSetRead(descr))
        {
            try{
            Client *client_sock = new Client(curport);
            addClient(client_sock);
            }catch (Exception &e){
                std::cout << "Connection refused\n";
            }
        }
    }
}

void Server::readRequests()
{
    //std::cout << "READ REQUESTS BLOCK\n";
    ssize_t ret;
    int descr;
    std::vector<Client *>::iterator itC = allclients.begin();
    while (itC != allclients.end())
    {
        descr = (*itC)->getDescriptor();
        if (isSetRead(descr))
        {
            ret = read(descr, buf, BUFFERSIZE);
            if (ret > 0)
            {
                (*itC)->setTimer();
                (*itC)->getBuffer()->fillBuffer(buf, ret);
                bzero(&buf, ret);
                if ((*itC)->is_full())
                {
                    (*itC)->handleRequest(_parser);
//                    (*itC)->handleRequest(_parser);
                    if ((*itC)->getStatus() == 3 && (*itC)->getFileWrite())
                        addFile((*itC)->getFileWrite());
                }
                itC++;
            }
            else if (ret <= 0)
            {
                if ((*itC)->getFileWrite())
                {
                    std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite());
                    if (itF != allfiles.end())
                        allfiles.erase(itF);
                }
                if ((*itC)->getFileRead())
                {
                    std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileRead());
                    if (itF != allfiles.end())
                        allfiles.erase(itF);
                }
                (*itC)->setKeep(false);
                delete (*itC);
                itC = allclients.erase(itC);
            }
        }
        else
            itC++;
    }

    std::vector<FileUpload *>::iterator itF = allfiles.begin();
    while (itF != allfiles.end())
    {
        descr = (*itF)->getDescriptor();
        if (isSetRead(descr))
        {
            ret = read(descr, buf, BUFFERSIZE);
            if (ret > 0)
            {
                (*itF)->getClient()->setTimer();
                (*itF)->getClient()->getBuffer()->fillContent(buf, ret);
                bzero(&buf, ret);
                itF++;
            }
            else if (!ret)
            {
                (*itF)->getClient()->setTimer();
                size_t buflen = strlen(buf);
                (*itF)->getClient()->getBuffer()->fillContent(buf, buflen);
                bzero(&buf, ret);
                (*itF)->getClient()->setStatus(2);
                (*itF)->getClient()->formAnswer();
                itF = allfiles.erase(itF);
            }
            else if (ret < 0)
                itF = allfiles.erase(itF);
        }
        else
            itF++;
    }
}

void Server::sendAnswer()
{
    //std::cout << "Send answer block\n";
    int descr;
    std::vector<FileUpload *>::iterator itF = allfiles.begin();
    while (itF != allfiles.end())
    {
        descr = (*itF)->getDescriptor();
        if (isSetWrite(descr))
        {
            (*itF)->getClient()->setTimer();
            (*itF)->fileWrite();
        }
        itF++;
    }

    std::vector<Client *>::iterator itC = allclients.begin();
    while (itC != allclients.end())
    {
        descr = (*itC)->getDescriptor();
        if (isSetWrite(descr))
        {
            (*itC)->setTimer();
            if ((*itC)->getStatus() == 4)
                (*itC)->formAnswer();
            else
            {
                if ((*itC)->getFileWrite())
                {
                    std::vector<FileUpload *>::iterator itDelete = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite());
                    if (itDelete != allfiles.end())
                        allfiles.erase(itDelete);
                }
                if ((*itC)->getFileRead())
                {
                    std::vector<FileUpload *>::iterator itDelete = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileRead());
                    if (itDelete != allfiles.end())
                        allfiles.erase(itDelete);
                }
                (*itC)->sendResponse();
                if ((*itC)->getStatus() < 0)
                {
                    delete (*itC);
                    itC = allclients.erase(itC);
                    continue ;
                }
            }
        }
        itC++;
    }
}

void Server::cleaner()
{
    allfiles.clear();
    std::vector<Client *>::iterator it = allclients.begin();
    while (it != allclients.end())
    {
        delete (*it);
        it = allclients.erase(it);
    }
}

Port *Server::hasPort(int val)
{
    for (size_t i=0; i < allports.size(); i++)
    {
        if (allports[i]->getPort() == val)
            return (allports[i]);
    }
    return (0);
}

void Server::setParser(Parser *parser)
{
    this->_parser = parser;
}

bool Server::isSetRead(int fd)
{
    return (FD_ISSET(fd, &read_current));
}

bool Server::isSetWrite(int fd)
{
    return (FD_ISSET(fd, &write_current));
}

void Server::clearPorts()
{
    std::vector<Port *>::iterator it = allports.begin();
    while (it != allports.end())
    {
        delete (*it);
        it = allports.erase(it);
    }
}
