#ifndef SERVER_HPP
#define SERVER_HPP
#define BUFFERSIZE 1024*1024*10
#define TIMEOUT 120
#include <sys/select.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "Client.hpp"
#include "Port.hpp"

class Parser;

class Server{
private:
    fd_set read_current;
    fd_set write_current;
    std::vector<Client *> allclients;
    std::vector<Client *> toerase;
    std::vector<FileUpload *> allfiles;
    std::vector<FileUpload *> toeraseF;
    std::vector<Port *> allports;
    struct timeval timeout;
    char buf[BUFFERSIZE];
    Parser *_parser;
    
public:
    Server()
    {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
    }
    ~Server(){}
    
    void refillSets()
    {
        //std::cout << "SET REFILL\n";
        FD_ZERO(&read_current);
        FD_ZERO(&write_current);
        //std::cout << "Clients size = " << allclients.size() << "\n";
        struct timeval curTime;
        gettimeofday(&curTime, 0);
        //for (std::vector<Client *>::iterator it = allclients.begin(); it != allclients.end(); ++it)
        std::vector<Client *>::iterator itC = allclients.begin();
        //std::cout << "Here 1\n";
        while (itC != allclients.end())
        {
            //std::cout << "Check client " << (*itC)->getDescriptor() << "\n";
            if ((*itC)->getStatus() < 0 || curTime.tv_sec - (*itC)->getTimer().tv_sec >= TIMEOUT)// && (*itC)->getStatus() >= 0)
            {
                //std::cout << "Removed client: " << (*itC)->getDescriptor() << "\n";
                if ((*itC)->getFileWrite())// && ((*itC)->getFileWrite()->getStatus() >= 0))
                {
                    //(*itC)->getFileWrite()->setStatus(-2);
                    //removeFile((*itC)->getFileWrite());
                    //std::cout << "Removed file " << (*itC)->getFileWrite()->getDescriptor() << "\n";
                    std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite());
                    if (itF != allfiles.end())
                        allfiles.erase(itF);
                }
                if ((*itC)->getFileRead())// && ((*itC)->getFileRead()->getStatus() >= 0))
                {
                    //(*itC)->getFileRead()->setStatus(-2);
                    //removeFile((*itC)->getFileRead());
                    //std::cout << "Removed file " << (*itC)->getFileRead()->getDescriptor() << "\n";
                    std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileRead());
                    if (itF != allfiles.end())
                        allfiles.erase(itF);
                }

                //removeClient(*itC);
                //std::vector<Client *>::iterator toDelete = itC++;
                delete (*itC);
                itC = allclients.erase(itC);
                continue ;
            }

            if (!(*itC)->getStatus())
            {
                FD_SET((*itC)->getDescriptor(), &read_current);
                //std::cout << (*it)->getDescriptor() << " (CLIENT) ADDED TO READ SET\n";
            }
            if ((*itC)->getStatus() == 6)
            {
                (*itC)->setStatus(7);
                //(*it)->finishPipe();
                addFile((*itC)->getFileRead());
                //std::cout << (*it)->getFileRead()->getDescriptor() << " | path: " << (*it)->getFileRead()->getPath() << " | client = " << (*it)->getDescriptor() << " FILE ADDED TO LIST\n";
            }
            if ((*itC)->getStatus() == 3)
            {
                if (!(*itC)->getFileWrite() || (*itC)->getFileWrite()->getStatus() == 1)
                    (*itC)->setStatus(4);
                else if ((*itC)->getFileWrite()->getStatus() == -2)
                    (*itC)->setStatus(-1);
                //(*it)->resetFile();
            }
            if ((*itC)->getStatus() == 2 || (*itC)->getStatus() == 4)
            {
                FD_SET((*itC)->getDescriptor(), &write_current);
                //std::cout << (*it)->getDescriptor() << " (CLIENT) ADDED TO WRITE SET\n";
            }


            /*if ((*it)->getStatus() < 0)
            {
                std::cout << "Marked for remove " << (*it)->getDescriptor() << "\n";
                removeClient((*it));
                if ((*it)->getFileWrite)
                    removeFile((*it)->getFileWrite);
                if ((*it)->getFileWrite)
                removeFile((*it)->getFileWrite);
                
            }*/
            itC++;
        }

        std::vector<FileUpload *>::iterator itF = allfiles.begin();
        //for (std::vector<FileUpload *>::iterator it = allfiles.begin(); it != allfiles.end(); ++it)
        while (itF != allfiles.end())
        {
            //if (!*itF)
             //   std::cout << "FILE END\n";
            //std::cout << "Check file " << (*itF)->getDescriptor() << "\n";
            //std::cout << "Here F0\n";
            //std::cout << "Status = " << (*itF)->getStatus() << "\n";
            if ((*itF)->getStatus() < 0)
            {
                itF = allfiles.erase(itF);
                //allfiles.erase(toDelete);
                //std::cout << "Here F4\n";
                continue ;
            }
            //std::cout << "Here F1\n";
            if (!(*itF)->getStatus())
            {
                FD_SET((*itF)->getDescriptor(), &write_current);
                //std::cout << (*it)->getDescriptor() << " (FILE) ADDED TO WRITE SET\n";
            }
            //std::cout << "Here F2\n";
            if ((*itF)->getStatus() == 2)
            {
                FD_SET((*itF)->getDescriptor(), &read_current);
                //std::cout << (*it)->getDescriptor() << "FILE ADDED TO SET\n";
            }
            //std::cout << "Here F3\n";
            itF++;
        }
        
        for (std::vector<Port *>::iterator it = allports.begin(); it != allports.end(); ++it)
            FD_SET((*it)->getDescriptor(), &read_current);
        //std::cout << "SET REFILLED\n";
       std::cout << "Ports = " << portsCount() << " | Clients = " << clientsCount() << " | Files = " << filesCount() << " | max = " << getLastSock() <<"\n";
    }
    
    int getLastSock()
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
            /*if (allclients[i]->getStatus() == 6)
            {
                descr = allclients[i]->getPipe();
                if (descr > last_sock)
                    last_sock = descr;
            }*/
        }
        
        for (size_t i = 0; i < allports.size(); i++)
        {
            descr = allports[i]->getDescriptor();
            if (descr > last_sock)
                last_sock = descr;
        }
        return (last_sock);
    }
    
    int selector()
    {
        return (select(getLastSock() + 1, &read_current, &write_current, NULL, NULL)); //TODO Плохо работает с time
    }
    
    void addPort(Port *port)
    {
        allports.push_back(port);
    }
    
    size_t portsCount()
    {
        return (allports.size());
    }
    
    Port *getPort(size_t ind)
    {
        return (allports[ind]);
    }
    
    void addClient(Client *client)
    {
        allclients.push_back(client);
    }
    
    size_t clientsCount()
    {
        return (allclients.size());
    }
    
    Client *getClient(size_t ind)
    {
        return (allclients[ind]);
    }
    
    bool isSetRead(int fd)
    {
        return (FD_ISSET(fd, &read_current));
    }
    
    bool isSetWrite(int fd)
    {
        return (FD_ISSET(fd, &write_current));
    }
    
    void removeClient(Client *client)
    {
        FileUpload *file = client->getFileWrite();
        if (file && (file->getStatus() >= 0))
        {
            file->setStatus(-1);
            removeFile(file);
        }
        file = client->getFileRead();
        if (file && (file->getStatus() >= 0))
        {
            file->setStatus(-1);
            removeFile(file);
        }
        toerase.push_back(client);
    }
    
    void addFile(FileUpload *file)
    {
        allfiles.push_back(file);
    }
    
    void removeFile(FileUpload *file)
    {
        toeraseF.push_back(file);
    }
    
    size_t filesCount()
    {
        return (allfiles.size());
    }
    
    void remove()
    {
        for (size_t i = toeraseF.size(); i > 0; i--)
        {
            std::vector<FileUpload *>::iterator it = std::find(allfiles.begin(), allfiles.end(), toeraseF.back()); //TODO Что вообще тут происходит?
            //std::cout << "File " << (*it)->getDescriptor() << " DELETED\n";
            //delete (*it);
            allfiles.erase(it);
            toeraseF.pop_back();
        }
        for (size_t i = toerase.size(); i > 0; i--)
        {
            std::vector<Client *>::iterator it = std::find(allclients.begin(), allclients.end(), toerase.back());
            //std::cout << "Client " << (*it)->getDescriptor() << " DELETED\n";
            /*if ((*it)->getFileRead())
            {
                std::vector<FileUpload *>::iterator it2 = std::find(allfiles.begin(), allfiles.end(), (*it)->getFileRead());
                if (it2 != allfiles.end())
                    allfiles.erase(it2);
            }
            if ((*it)->getFileWrite())
            {
                std::vector<FileUpload *>::iterator it2 = std::find(allfiles.begin(), allfiles.end(), (*it)->getFileWrite());
                if (it2 != allfiles.end())
                    allfiles.erase(it2);
            }*/
            delete (*it);
            allclients.erase(it);
            toerase.pop_back();
            //std::cout << "Client DELETION COMPLETE\n";
        }
    }
    
    fd_set &readSet()
    {
        return (read_current);
    }
    
    fd_set &writeSet()
    {
        return (write_current);
    }
    
    void handleConnections()
    {
        //std::cout << "Handle connection\n";
        Port *curport;
        int descr;
        for (size_t i = 0; i < portsCount(); ++i)
        {
            curport = allports[i];
            descr = curport->getDescriptor();
            //std::cout << "Curport = " << descr << "\n";
            if (isSetRead(descr))
            {
                try{
                Client *client_sock = new Client(curport);
                //std::cout << "New read connection, fd = " << client_sock->getDescriptor() << "\n";
                addClient(client_sock);
                }catch (Exception &e){
                    std::cout << e.what() << "\n";
                }
            }
        }
        //std::cout << "Connection handled\n";
    }
    
    void readRequests()
    {
        //std::cout << "READ REQUESTS BLOCK\n";
        ssize_t ret;
        int descr;
        //Client *curclient;
        //FileUpload *file;
        //for (size_t i = 0; i < clientsCount(); i++)
        //for (std::vector<Client *>::iterator it = allclients.begin(); it != allclients.end(); it++)
        std::vector<Client *>::iterator itC = allclients.begin();
        while (itC != allclients.end())
        {
            //curclient = allclients[i];
            descr = (*itC)->getDescriptor();
            //std::cout << "Check client for read: " << descr << " | status = " << (*itC)->getStatus() << "\n";
            if (isSetRead(descr))// && !curclient->getStatus())
            {
                //std::cout << descr << "-client is readable\n";
                ret = recv(descr, buf, BUFFERSIZE, 0);
                //std::cout << descr << ": Ret = " << ret << "\n";
                if (ret > 0)
                {
                    (*itC)->setTimer();
                    for (ssize_t k = 0; k < ret; k++)
                    {
                        (*itC)->fillBuffer(buf[k]);
                        buf[k] = 0;
                    }
                    //std::cout << "Buffer:\n" << readbufs[*it].str() << "\nEnd buffer\n";
                    if ((*itC)->is_full())
                    {
                        (*itC)->handleRequest(_parser);
                        if ((*itC)->getStatus() == 3 && (*itC)->getFileWrite())
                            addFile((*itC)->getFileWrite());
                    }
                }
                //if (ret < 0)
                 //   continue ;
                    //std::cout << "Ret - 1\n";
                if (ret <= 0)
                {
                    /*file = (*itC)->getFileWrite();
                    if (file && (file->getStatus() >= 0))
                    {
                        file->setStatus(-2);
                        removeFile(file);
                    }
                    file = (*itC)->getFileRead();
                    if (file && (file->getStatus() >= 0))
                    {
                        file->setStatus(-2);
                        removeFile(file);
                    }
                    removeClient(*itC);*/

                    if ((*itC)->getFileWrite())// && ((*itC)->getFileWrite()->getStatus() >= 0))
                        {
                        //(*itC)->getFileWrite()->setStatus(-2);
                        //removeFile((*itC)->getFileWrite());
                        std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite());
                        if (itF != allfiles.end())
                            allfiles.erase(itF);
                        }
                    if ((*itC)->getFileRead())// && ((*itC)->getFileRead()->getStatus() >= 0))
                        {
                        //(*itC)->getFileRead()->setStatus(-2);
                        //removeFile((*itC)->getFileRead());
                        std::vector<FileUpload *>::iterator itF = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileRead());
                        if (itF != allfiles.end())
                            allfiles.erase(itF);
                        }

                    //removeClient(*itC);
                    //std::vector<Client *>::iterator toDelete = itC++;
                    delete (*itC);
                    itC = allclients.erase(itC);

                }
                else
                    itC++;
            }
            else
                itC++;
            /*else if (curclient->getStatus() == 6)
            {
                //std::cout << curclient->getPipe() << "-pipe is readable\n";
                ret = read(curclient->getPipe(), buf, BUFFERSIZE);
                //std::cout << ": Ret = " << ret << "\n";
                if (ret > 0)
                {
                    for (ssize_t k = 0; k < ret; k++)
                    {
                        curclient->fillContent(buf[k]);
                        //std::cout << "#" << buf[i] << "#";
                        buf[k] = 0;
                    }
                }
                if (ret < 0)
                {
                    std::cout << "Pipe read error\n";
                    removeClient(curclient);
                }
                if (!ret)
                {
                    FD_CLR(curclient->getPipe(), &read_current);
                    curclient->finishPipe();
                }
            }*/
            /*else if ((*itC)->getStatus() == -1)
                removeClient(*itC);*/

        }
    

        //for (size_t i = 0; i < filesCount(); i++)
        //for (std::vector<FileUpload *>::iterator it = allfiles.begin(); it != allfiles.end(); it++)
        std::vector<FileUpload *>::iterator itF = allfiles.begin();
        while (itF != allfiles.end())
        {
            //file = allfiles[i];
            descr = (*itF)->getDescriptor();
            //std::cout << "Check file for read: " << descr << " | status = " << file->getStatus(); //" | path = " << file->getPath() << "\n";
            //if (descr <= 2  || descr > 1024)
            //    exit(0);
            if (isSetRead(descr))
            {
                ret = read(descr, buf, BUFFERSIZE);
                //std::cout << descr << " file : Ret = " << ret << "\n";
                if (ret > 0)
                {
                    //std::cout << "RET = " << ret << "\n";
                    (*itF)->getClient()->setTimer();
                    for (int k = 0; k < ret; k++)
                    {
                        (*itF)->getClient()->fillContent(buf[k]);
                        //std::cout << "#" << buf[i] << "#";
                        buf[k] = 0;
                    }
                    itF++;
                }
                else if (!ret)
                {
                    //std::cout << "RET = 0\n";
                    (*itF)->getClient()->setTimer();
                    int k = 0;
                    while (buf[k])
                    {
                        (*itF)->getClient()->fillContent(buf[k]);
                        //std::cout << "#" << buf[i] << "#";
                        buf[k++] = 0;
                    }
                    (*itF)->getClient()->setStatus(2);
                    (*itF)->getClient()->formAnswer();
                    (*itF)->setStatus(-1);
                    FD_CLR(descr, &read_current);
                    //removeFile(*itF);
                    itF = allfiles.erase(itF);
                    //file->getClient()->resetFile(&file);
                }
                else if (ret < 0)
                {
                    std::cout << "Pipe read error\n";
                    //removeFile(*itF);
                    //std::vector<FileUpload *>::iterator toDelete = itF++;
                    itF = allfiles.erase(itF);
                    //continue ;
                }
                //std::cout << "END HERE\n";
            }
            else
                itF++;
            /*else if ((*itF)->getStatus() == -1)
                removeFile(*itF);*/
            //itF++;
        }
        //std::cout << "FInished to read requests\n";
    }
    
    
    void sendAnswer()
    {
        //std::cout << "Send answer block\n";
        // int ret;
        int descr;
        // int len;
        //Client *curclient;
        //FileUpload *file;
        //for (size_t i = 0; i < filesCount(); i++)
        //for (std::vector<FileUpload *>::iterator it = allfiles.begin(); it != allfiles.end(); it++)
        std::vector<FileUpload *>::iterator itF = allfiles.begin();
        while (itF != allfiles.end())
        {
            //file = allfiles[i];
            descr = (*itF)->getDescriptor();
            //std::cout << "Check file for write: " << descr << " | status = " << file->getStatus() <<"\n";//" | client = " << file->getClient()->getDescriptor() << "\n";
            //if (descr <= 2 || descr > 1024)
            //        exit(0);
            if (isSetWrite(descr))
            {
                (*itF)->getClient()->setTimer();
                //std::cout << descr << "-file is ready for writing\n";
                (*itF)->fileWrite();
            }
            /*else if ((*itF)->getStatus() == -1)
                removeFile(*itF);*/
            itF++;
        }
        
        //for (size_t i = 0; i < clientsCount(); i++)
        //for (std::vector<Client *>::iterator it = allclients.begin(); it != allclients.end(); it++)
        std::vector<Client *>::iterator itC = allclients.begin();
        while (itC != allclients.end())
        {
            //curclient = allclients[i];
            descr = (*itC)->getDescriptor();
            //std::cout << "Check client for write: " << descr << " | status = " << curclient->getStatus() << "\n";
            if (isSetWrite(descr))
            {
                (*itC)->setTimer();
                //std::cout << descr << "-client is ready for answer\n";
                if ((*itC)->getStatus() == 4)
                    (*itC)->formAnswer();
                else
                {
                    /*file = (*itC)->getFileWrite();
                    if (file && (file->getStatus() >= 0))
                    {
                        file->setStatus(-1);
                        removeFile(file);
                    }

                    file = (*itC)->getFileRead();
                    if (file && (file->getStatus() >= 0))
                    {
                        file->setStatus(-1);
                        removeFile(file);
                    }*/
                    if ((*itC)->getFileWrite())// && ((*itC)->getFileWrite()->getStatus() >= 0))
                        {
                        //(*itC)->getFileWrite()->setStatus(-2);
                        //removeFile((*itC)->getFileWrite());
                        //std::cout << "Removed file " << (*itC)->getFileWrite()->getDescriptor() << "\n";
                        std::vector<FileUpload *>::iterator itDelete = std::find(allfiles.begin(), allfiles.end(), (*itC)->getFileWrite()); //TODO Пожалуйста(!) Давай нармальные имена переменным
                        if (itDelete != allfiles.end())
                            allfiles.erase(itDelete);
                        }
                    if ((*itC)->getFileRead())// && ((*itC)->getFileRead()->getStatus() >= 0))
                        {
                        //(*itC)->getFileRead()->setStatus(-2);
                        //removeFile((*itC)->getFileRead());
                        //std::cout << "Removed file " << (*itC)->getFileRead()->getDescriptor() << "\n";
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
            /*else if ((*itC)->getStatus() == -1)
                removeClient(*itC);*/
            itC++;
        }
        //std::cout << "End answer block\n";
    }
    
    void cleaner()
    {
        /*int descr;
        Client *curclient;
        for (size_t i = 0; i < clientsCount(); i++)
        {
            curclient = allclients[i];
            descr = curclient->getDescriptor();
            //std::cout << "Check client for clean: " << descr << " | status = " << curclient->getStatus() << "\n";
            curclient->setStatus(-1);
        }*/
        allfiles.clear();
        std::vector<Client *>::iterator it = allclients.begin();
        while (it != allclients.end())
        {
            delete (*it);
            it = allclients.erase(it);
        }
    }
    
    Port *hasPort(int val)
    {
        for (size_t i=0; i < allports.size(); i++)
        {
            if (allports[i]->getPort() == val)
                return (allports[i]);
        }
        return (0);
    }
    
    void setParser(Parser *parser)
    {
        this->_parser = parser;
    }
};

#include "Parser.hpp"
#endif
