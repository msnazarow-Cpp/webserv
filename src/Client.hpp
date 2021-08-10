#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <vector>

#include "Port.hpp"
#include "File.hpp"

class Client{
private:
    int descriptor;
    std::stringstream buffer;
    std::ostringstream response;
    int code;
    std::string content;
    int responseSize;
    int responsePos;
    int requestType;
    size_t requestLen;
    Port *port;
    int status;
    FileUpload *file;
    
public:
    Client(Port *port, timeval &timeout): file(0)
    {
        std::cout << "CLIENT TO PORT " << port->getDescriptor() << "\n";
        descriptor = accept(port->getDescriptor(), 0, 0);
        if (descriptor < 0)
            throw Exception("Client connection exception");
        if (fcntl(descriptor, F_SETFL, O_NONBLOCK) < 0)
            throw Exception("Client fcntl exception");
        setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        this->port = port;
        requestType = 0;
        requestLen = 0;
        status = 0;
        responsePos = 0;
    }
    
    ~Client()
    {
        close(descriptor);
        buffer.str("");
    }
    
    int &getDescriptor()
    {
        return (descriptor);
    }
    
    std::stringstream *getBuffer()
    {
        return (&buffer);
    }
    
    std::string getBufferStr()
    {
        return (buffer.str());
    }
    
    void fillBuffer(char c)
    {
        buffer << c;
    }
    
    int getType()
    {
        return (requestType);
    }
    
    void setType(int type)
    {
        requestType = type;
    }
    
    size_t getLen()
    {
        return (requestLen);
    }
    
    void setLen(size_t len)
    {
        requestLen = len;
    }
    
    Port *getPort()
    {
        return (port);
    }
    
    int getStatus()
    {
        return (status);
    }
    
    void setStatus(int val)
    {
        status = val;
    }
    
    void handleGet(std::string &request, std::map<std::string, ServerBlock *>::iterator &result)
    {
        std::cout << "Handle GET\n";
        std::stringstream host;
        size_t pos;
        size_t pos2;
        
        std::cout << "REQUEST:\n" << request << "\nEND\n";
        
        pos = 4;
        pos2 = request.find(" ", pos);
        host << request.substr(pos, pos2 - pos);
        std::string hostr = host.str();
        std::cout << "\nSEARCH IN: " << hostr << " | pos = " << pos << " | pos2 = " << pos2 << "\n";
        host.str("");
        if (hostr == "/")
            hostr = "/index.html";
        host << result->second->getRoot() << hostr;
        std::ifstream f(host.str());
        if (f.good())
        {
            std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            content = str;
            code = 200;
        }
        f.close();
        std::cout << "\nADDRESS IS: " << host.str() << "\n";
        formAnswer();
    }

    void handlePost(std::string &request, std::map<std::string, ServerBlock *>::iterator &result)
    {
        std::cout << "Handle POST\n";
        
        std::stringstream sizestream;
        int size;
        size_t pos = 5;
        size_t pos2;
        
        pos2 = request.find("\r\nContent-Length: ", pos);
        if (pos2 != std::string::npos)
        {
            pos2 += 18;
            pos = request.find("\r\n", pos2);
            if (pos != std::string::npos)
            {
                sizestream << request.substr(pos2, pos - pos2);
                sizestream >> size;
                std::cout << "SIZE = " << size << "\n";
            }
            sizestream.str("");
        }
        int toreceive = size;
        
        pos = request.find("\r\n------Web");
        if (pos != std::string::npos)
            pos += 2;
        int ret;
             
        std::string filename;
            
        std::cout << "WRITE STAGE 1\n";
        pos = request.find("\nContent-Disposition:", 0);
        if (pos == std::string::npos)
        {
            std::cout << "NOT FOUND Content-Disposition\n";
            return ; // ERROR HANDLE, но потом
        }
        std::cout << "WRITE STAGE 2\n";
        pos2 = request.find("filename=\"", pos);
        if (pos2 == std::string::npos)
        {
            std::cout << "NOT FOUND filename=\"\n";
            return ; // ERROR HANDLE, но потом
        }
        std::cout << "WRITE STAGE 3\n";
        pos2 += 10;
        pos = request.find("\"\r\n", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "NOT FOUND \"rn\n";
            return ; // ERROR HANDLE, но потом
        }
        std::stringstream streamer;
        std::cout << "WRITE STAGE 4\n";
        streamer << "./www/server1/uploads/";
        streamer << request.substr(pos2, pos - pos2);
        filename = streamer.str();
        streamer.str("");
        std::cout << "FILENAME IS: " << filename << " | pos = " << pos << " | pos2 = " << pos2 << "\n";
        
        
        
        
        pos2 = request.find("Content-Type: ", pos);
        if (pos2 == std::string::npos)
        {
            std::cout << "NOT FOUND Content-Type: \n";
            return ; // ERROR HANDLE, но потом
        }
        std::cout << "Content type - pos= " << pos2 << "\n";
        pos = request.find("\r\n\r\n", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "WRONG\n";
            return ; // ERROR HANDLE, но потом
        }
        pos += 4;
        std::cout << "SIZE = " << size << " | POS = " << pos;
        
        pos2 = request.find("\r\n------Web", pos);
        if (pos2 != std::string::npos)
            size = pos2 - pos;
        else
            size -= pos;
        std::cout << " | POS2 = " << pos2 << "\n";
        
        //std::cout << "Request\n" << request << "\nend\n";
        
        std::cout << "LENGTH = " << size << "\n";
        try{
            file = new FileUpload(filename.c_str(), size, request.substr(pos, size));
            setStatus(3);
            code = 200;
            content = "<h1>Uploaded Successfully</h1>";
            formAnswer();
        } catch (Exception &e){
            std::cout << e.what() << "\n"; //ERROR HANDLE HERE
            setStatus(-1);
            code = 404;
        }
    }
    
    void formAnswer()
    {
        std::cout << "FORM ANSWER\n";
        if (code == 404)
            content = "<h1>404 Not Found</h1>";
        response << "HTTP/1.1 " << code << " OK\r\n";
        response << "Cache-Control: no-cache, private\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << content.size() << "\r\n";
        response << "\r\n";
        response << content;
        responseSize = response.str().size() + 1;
        if (requestType == 1)
            status = 2;
    }
    
    void handleRequest()
    {
        std::cout << "HANDLE REQUEST\n";

        
        std::string request = getBufferStr();
        //std::cout << "Received:\n" << request << "\nEND\n";
        std::stringstream host;
        size_t pos = request.find("\r\nHost: ", 0);
        if (pos != std::string::npos)
        {
            pos += 8;
            size_t pos2 = request.find(":", pos);
            host << request.substr(pos, pos2 - pos);
            //content = host.str();
            std::map<std::string, ServerBlock *>::iterator result = port->getMap().find(host.str());
            status = 1;
            if (result != port->getMap().end())
            {
                host.str("");
                if (requestType == 1)
                    handleGet(request, result);
                else if (requestType == 2)
                    handlePost(request, result);
            }
            //else cтраница недоступна по данному порту
        }
        host.str("");
        std::cout << "END HANDLE REQUEST\n";
    }
    
    void sendResponse()
    {
        std::cout << "SEND RESPONSE: size = " << responseSize << " | pos = " << responsePos << " | result = " << responseSize - responsePos << "\n";
        std::cout << "To send:\n" << response.str().substr(responsePos, responseSize - responsePos).c_str() << "\nEND\n";
        int send_size = send(descriptor, response.str().substr(responsePos, responseSize - responsePos).c_str(), responseSize - responsePos, 0);
        std::cout << "Send returned : " << send_size << "\n";
        //if (send_size <= 0)
            
        responsePos += send_size - 1;
        if (responsePos == responseSize - 1)
        {
            status = 0;
            response.str("");
            responsePos = 0;
            responseSize = 0;
        }
    }
    
    bool ends_with(std::string const &value, std::string const &ending)
    {
        if (ending.size() > value.size())
            return (false);
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    bool is_full()
    {
        std::string value = getBufferStr();
        if (!getType())
        {
            if (!value.compare(0, 4, "GET "))
                setType(1);
            else if (!value.compare(0, 5, "POST "))
                setType(2);
        }
        if (getType() == 1 && ends_with(value, "\r\n\r\n"))
            return (true);

        size_t pos = 5;
        size_t pos2;
        if (!getLen())
        {
            std::stringstream sizestream;
            size_t size;
            pos2 = value.find("\r\nContent-Length: ", pos);
            if (pos2 != std::string::npos)
            {
                pos2 += 18;
                pos = value.find("\r\n", pos2);
                if (pos != std::string::npos)
                {
                    sizestream << value.substr(pos2, pos - pos2);
                    sizestream >> size;
                    std::cout << "SIZE = " << size << "\n";
                    setLen(size);
                }
                sizestream.str("");
            }
        }
        
        pos2 = value.find("\r\n\r\n", pos);
        if (pos2 == std::string::npos)
            return (false);
        pos2 += 4;
        std::cout << "Length = " << value.length() << " | pos = " << pos2 << "\n";
        if (value.length() - getLen() == pos2)
        {
            setStatus(1);
            return (true);
        }
        return (false);
    }
    
    FileUpload *getFile()
    {
        return (file);
    }
    
    void resetFile()
    {
        file = 0;
    }
    
};

#include "Server.hpp"
#endif
