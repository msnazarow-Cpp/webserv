#ifndef CLIENT_HPP
#define CLIENT_HPP

#ifndef __APPLE__
#include <wait.h>
#endif
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>


#include <cstdio>
//#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>

#include "Port.hpp"
#include "File.hpp"
#include "Parser.hpp"
#include "ServerBlock.hpp"


class Client{

public:
    static int count;
    static int active;
private:
    int descriptor;
    std::stringstream buffer;
    
    std::ostringstream response;
    int responseSize;
    int responsePos;
    int code;
    
    size_t requestLen;
    std::string content;
    std::string cgi;
    
    int requestType;
    std::stringstream path;
    std::string target;
    
    std::map<std::string, std::string> envs;
    std::string requestProtocol;
    std::string requestBody;
    std::string requestBoundary;
    
    std::string requestBuffer;
    
    int requestBodySize;
    
    
    Port *port;
    FileUpload *fileWrite;
    FileUpload *fileRead;
    int status;
    bool keepAlive;

    struct timeval timer;
    ServerBlock *s_block;
    
public:
    Client(Port *port):/* cgi(),*/ fileWrite(0), fileRead(0)
    {
        //std::cout << "CLIENT TO PORT " << port->getDescriptor() << "\n";
        sockaddr_in addr;
        int addrLen = sizeof(addr);
        descriptor = accept(port->getDescriptor(), (sockaddr *) &addr, (socklen_t *)&addrLen);
        if (descriptor < 0)
            throw Exception("Client connection exception");
        struct linger so_linger;
        so_linger.l_onoff = true;
        so_linger.l_linger = 0;
        setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
        /*int reuse = 1;
        if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
            throw Exception("Setsockopt(SO_REUSEADDR) exception");
        if (setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &reuse, sizeof(reuse)) < 0)
            throw Exception("Setsockopt(SO_NOSIGPIPE) exception");*/
        if (fcntl(descriptor, F_SETFL, O_NONBLOCK) < 0)
            throw Exception("Client fcntl exception");
        //setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        this->port = port;
        requestType = 0;
        requestLen = 0;
        status = 0;
        responsePos = 0;
        code = 0;
        keepAlive = false;
        gettimeofday(&timer, 0);
        ++count;
        ++active;
        s_block = 0;
    }
    
    ~Client()
    {
        if (!keepAlive)
            shutdown(descriptor, SHUT_RDWR);
        close(descriptor);
        reset();
        --active;
    }
    
    void reset()
    {
        //std::cout << "COMPLETE RESET\n";
        buffer.str("");
        response.str("");
        path.str("");
        content.clear();
        requestBodySize = 0;
        cgi.clear();
        target.clear();
        requestProtocol.clear();
        requestBody.clear();
        requestBuffer.clear();
        requestBoundary.clear();
        requestType = 0;
        
        envs.clear();
        
        
        responseSize = 0;
        responsePos = 0;
        code = 0;
        
        s_block = 0;
        resetFile(&fileWrite);
        resetFile(&fileRead);
        if (keepAlive)
            status = 0;
        else
            status = -1;
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

    void setCode(int val)
    {
        code = val;
    }

    void fillErrorContent(int code)
    {
        switch (code){
            case 400:{
                content = "<h1>400: Bad request</h1>";
                break;
            }
            case 401:{
                content = "<h1>401: Authorization required</h1>";
                break;
            }
            case 403:{
                content = "<h1>403: Access forbidden</h1>";
                break;
            }
            case 404:{
                content = "<h1>404: Not Found</h1>";
                break;
            }
            case 405:{
                content = "<h1>405: Request type not allowed</h1>";
                break;
            }
            case 408:{
                content = "<h1>408: Timeout</h1>";
                break;
            }
            case 411:{
                content = "<h1>411: Length required</h1>";
                break;
            }
            case 500:{
                content = "<h1>500: Internal server error</h1>";
                break;
            }
        }
    }

    bool handleErrorPage(int code)//, ServerBlock *block)
    {
        std::cout << "Handle error\n";
        if (!s_block)
        {
            status = -1;
            return (true);
        }
        std::string error = s_block->getErrorPage(code);
        std::cout << "Error: " << error << "\n";
        if (error.empty())
        {

            fillErrorContent(code);
            status = 2;
            formAnswer();
        }
        else
        {
            try{
                fileRead = new FileUpload(error, 0, "", this, true);
                fileRead->setStatus(2);
                status = 6;
            }catch(Exception &e){
                fillErrorContent(code);
                status = 2;
                formAnswer();
            }
        }
        return (false);
    }

    bool parseHeader(std::string const &request, Parser *parser)//std::map<std::string, ServerBlock>::iterator &result, Parser *parser)
    {
        size_t pos;
        size_t pos2;
        //std::cout << "PARSE HEADER\n";
        //std::cout << "PARSE HEADER\nREQUEST:\n" << request << "\nEND\n";
        if (requestType == 1)
            pos = 4;
        else if (requestType == 2)
            pos = 5;
        else if (requestType == 3)
            pos = 7;
        else
        {
            //content = "Wrong request type";
            code = 405;
            return (handleErrorPage(code));//, &(result->second)));
            //return (false);
        }
        //std::cout << "METHOD: #" << envs["REQUEST_METHOD"] << "#\n";

        //PATH & TARGET
        pos2 = request.find(" ", pos);
        if (pos2 == std::string::npos)
        {
            /*std::cout << "Wrong request header (target)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage(code));//, &(result->second)));
        }
        path << request.substr(pos, pos2 - pos);
        target = path.str();
        envs["REQUEST_URI"] = target;
        size_t pos3 = path.str().find("?", 0);
        if (pos3 != std::string::npos)
        {
            pos3++;
            //requestQuery = target.substr(pos3, target.length() - pos3);
            envs.insert(std::pair<std::string, std::string>("QUERY_STRING", target.substr(pos3, target.length() - pos3)));
            target = path.str().substr(0, pos3 - 1);
        }
        else
            envs.insert(std::pair<std::string, std::string>("QUERY_STRING", ""));
            //requestQuery = "";
        
        //std::cout << "QUERY_STRING: #" << envs["QUERY_STRING"] << "#\n";
        
        
        //std::cout << "Target: " << target << " | Path: ";
        path.str("");
        /*if (target == "/")
            target = "/index.html";
        path << result->second->getRoot() << target;
        std::cout << path.str() << "\n";*/
        
        //PROTOCOL
        pos2++;
        pos = request.find("\r\n", pos2);
        if (pos == std::string::npos)
        {
            /*std::cout << "Wrong request header (protocol)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage(code));//, &(result->second)));
        }
        requestProtocol = request.substr(pos2, pos - pos2);
        //std::cout << "Request protocol: " << requestProtocol << "\n";
        
        //END FOR GET (without session)
        /*if (requestType == 1)
            return (false);*/
        
        //HOSTNAME
        pos += 2;
        pos2 = request.find("Host: ", pos);
        if (pos2 == std::string::npos)
        {
            /*std::cout << "Wrong request header (host)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage(code));//, &(result->second)));
        }
        pos2 += 6;
        pos = request.find(":", pos2);
        if (pos == std::string::npos)
        {
            /*std::cout << "Wrong request header (host:port)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage(code));//, &(result->second)));
        }
        //requestHost = request.substr(pos2, pos - pos2);
        std::string requestHost = request.substr(pos2, pos - pos2);
        envs.insert(std::pair<std::string, std::string>("SERVER_NAME", requestHost));
        //std::cout << "Request host: " << envs["SERVER_NAME"] << "\n";
        
        //PORT
        pos++;
        pos2 = request.find("\r\n", pos);
        if (pos2 == std::string::npos)
        {
            /*std::cout << "Wrong request header (port)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage(code));//, &(result->second)));
        }
        std::stringstream streamPort;
        streamPort << request.substr(pos, pos2 - pos);
        int requestPort;
        streamPort >> requestPort;
        envs.insert(std::pair<std::string, std::string>("SERVER_PORT", streamPort.str()));
        streamPort.str("");
        //std::cout << "Request port: " << envs["SERVER_PORT"] << "\n";

        pos = request.find("Connection: ", pos2);
        if (pos == std::string::npos)
            keepAlive = false;
        else
        {
            pos += 12;
            pos2 = request.find("\r\n", pos);
            if (pos2 == std::string::npos)
                keepAlive = false;
            else
            {
                if ((!request.compare(pos, pos2 - pos, "keep-alive")))
                    keepAlive = true;
                else
                    keepAlive = false;
            }
        }

        /*if (!keepAlive)
        {
            struct linger so_linger;
            so_linger.l_onoff = true;
            so_linger.l_linger = 0;
            setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &so_linger, sizeof (so_linger));
        }*/

        bool isErrorPage, isLegit;
        isLegit = true; //TODO У тебя isLegit остовалось неинициализированным - оно пока и не должно быть инициализированным, ему значение задается при вызове getfilename
        int maxSize = -1;
        path << parser->getfilename(requestHost, requestPort, target, isErrorPage, cgi, isLegit, requestType, code, maxSize);
        //std::cout << "CGI = " << cgi << "\nPATH = " << path.str() << "\n";
        if (!isLegit || isErrorPage)
        {
            /*std::ifstream f(path.str().c_str());
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
            }
            else*/
            /*try{
                fileRead = new FileUpload(path.str(), 0, "", this, true);
                fileRead->setStatus(2);
                status = 6;
            }catch(Exception &e){
                switch (code){
                    case 400:{
                        content = "<h1>400: Bad request</h1>";
                        break;
                    }
                    case 403:{
                        content = "<h1>403: Access forbidden</h1>";
                        break;
                    }
                    case 404:{
                        content = "<h1>404: Not Found</h1>";
                        break;
                    }
                    case 405:{
                        content = "<h1>405: Request type not allowed</h1>";
                        break;
                    }
                }
                status = 2;
                formAnswer();
            }*/
            /*{
                switch (code){
                    case 400:{
                        content = "<h1>400: Bad request</h1>";
                        break;
                    }
                    case 403:{
                        content = "<h1>403: Access forbidden</h1>";
                        break;
                    }
                    case 404:{
                        content = "<h1>404: Not Found</h1>";
                        break;
                    }
                    case 405:{
                        content = "<h1>405: Request type not allowed</h1>";
                        break;
                    }
                }
            }*/
            //f.close();
            //std::cout << "\nADDRESS IS: " << path.str() << "\n";
            //status = 2;
            //formAnswer();
            //return (false);
            return (handleErrorPage(code));//, &(result->second)));
        }

        
        
        pos = request.find("\r\n\r\n", pos2);
        if (pos == std::string::npos)
        {
            /*std::cout << "Wrong request header (header limits)\n";
            status = -1;
            return (true);*/
            return (handleErrorPage(code));//, &(result->second)));
        }
        std::string headerTmp = request.substr(pos2, pos - pos2 + 2);

        //std::cout << "REQUEST TYPE = " << requestType << "\n";
        //usleep(1000000);

        if (requestType == 3)
        {
            if (!remove(path.str().c_str()))
            {
                content = "<h1>File " + target + " deleted successfully</h1>";
                code = 200;
                status = 2;
                formAnswer();
                return (false);
            }
            else
            {
                code = 404;
                return (handleErrorPage(code));//, &(result->second)));
            }
            /*status = 2;
            formAnswer();
            return (false);*/
        }
        
        /*std::cout << "HEADER REST:\n" << headerTmp << "\nEND\n";
        pos2 = 0;
        while ((pos3 = headerTmp.find("\r\n", pos2)) != std::string::npos)
        {
            pos = headerTmp.find(": ", pos2);
            if (pos == std::string::npos)
            {
                std::cout << "Wrong request header: " << pos2 << " - " << pos3 << "\n";
                status = -1;
                return (true);
            }
            std::string t1 = headerTmp.substr(pos2, pos - pos2);
            //std::string t2 = headerTmp.substr(pos + 2, pos3 - pos - 2);
            //std::cout << "KEY: " << t1 << " | VALUE: " << t2 << "\n";
            std::transform(t1.begin(), t1.end(), t1.begin(), ::toupper);
            size_t pos4 = t1.find("-");
            if (pos4 != std::string::npos)
                t1[pos4] = '_';
            envs[t1] = headerTmp.substr(pos + 2, pos3 - pos - 2);
            pos2 = pos3 + 2;
        }*/
        
        //envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", requestProtocol));
        envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", "HTTP/1.1"));
        envs.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "whatever/0.0"));
        envs.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
        envs.insert(std::pair<std::string, std::string>("SCRIPT_NAME", path.str()));
        envs["SCRIPT_FILENAME"] = envs["SCRIPT_NAME"];
        envs["PATH_INFO"] = target;
        envs["PATH_TRANSLATED"] = target;
        envs["REDIRECT_STATUS"] = true;
        
        
        
        //BODY
        if (getLen() && !cgi.empty())
        {
            if (maxSize > -1 && maxSize < requestBodySize)
            {
                code = 411;
                return (handleErrorPage(code));//, &(result->second)));
            }
            pos2 = request.find("\r\n\r\n");
            if (pos2 == std::string::npos)
            {
                code = 400;
                return (handleErrorPage(code));//, &(result->second)));
            }
            pos2 += 4;
            requestBody = request.substr(pos2, getLen());
            //std::cout << "Request body:\n#" << requestBody << "#\nEnd\n";
            try{
                //std::cout << "HERE1\n";
                std::stringstream filename;
                //std::cout << "Map: " << result->second.getRoot() << "\n";
                //filename << result->second.getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                //std::cout << "Buffer: " << result->second.getBuffer() << "\n";
                requestBuffer = filename.str();
                //std::cout << "RequestBuffer: " << requestBuffer << "\n";
                std::stringstream tmp2;
                tmp2 << ".buffer/." << port->getDescriptor() << "_" << descriptor;
                fileWrite = new FileUpload(requestBuffer, getLen(), requestBody, this, false);
                //std::cout << "Client " << getDescriptor() << "created fileWrite " << getFileWrite()->getDescriptor() << "\n";
                filename << "_read";
                fileRead = new FileUpload(filename.str(), 0, "", this, false);
                //std::cout << "Client " << getDescriptor() << "created fileRead " << getFileRead()->getDescriptor() << "\n";
                envs["HTTP_TMP"] = tmp2.str();
                requestBody.clear();
                setStatus(3);
                filename.str("");
                //setStatus(-1);
                //std::cout << "end parse\n";
                
            } catch (Exception &e){
                /*std::cout << e.what() << "\n";
                setStatus(-1);
                code = 404;*/
                code = 500;
                return (handleErrorPage(code));//, &(result->second)));
            }
            return (false);
        }
        else if (!envs["QUERY_STRING"].empty() && !cgi.empty())
        {
            try{
                std::stringstream filename;
                fileWrite = 0;
                //filename << result->second.getBuffer() << "/." << port->getDescriptor() << "_" << descriptor << "_read";
                filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor << "_read";
                fileRead = new FileUpload(filename.str(), 0, "", this, false);
                //std::cout << "Client " << getDescriptor() << "created fileRead " << getFileRead()->getDescriptor() << "\n";
                envs["HTTP_TMP"] = "";
                setStatus(3);
                filename.str("");
            } catch (Exception &e){
                /*std::cout << e.what() << "\n";
                setStatus(-1);
                code = 404;*/
                code = 500;
                return (handleErrorPage(code));//, &(result->second)));
            }
            return (false);
        }
        
        try{
            fileRead = new FileUpload(path.str(), 0, "", this, true);
            fileRead->setStatus(2);
            status = 6;
        }catch(Exception &e){
            /*std::cout << e.what() << "\n";
            setStatus(-1);
            code = 404;*/
            code = 500;
            return (handleErrorPage(code));//, &(result->second)));
        }
       // else
        //{
            //std::cout << "HERE1: " << path.str() << "\n";
            /*std::ifstream f(path.str().c_str());
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
                code = 200;
            }
            else
                code = 404;
            f.close();
            //std::cout << "\nADDRESS IS: " << path.str() << "\n";
            status = 2;
            formAnswer();*/
        //}
        
        return (false);
    }
        
    char **cgiEnvCreate()
    {
        char **result = new char *[envs.size() + 1];
        result[envs.size()] = 0;
        size_t i = 0;
        for (std::map<std::string, std::string>::iterator it = envs.begin(); it != envs.end(); it++)
            result[i++] = strdup((it->first + "=" + it->second).c_str());
        return (result);
    }
    
    void cgiResponseSimple()
    {
        //file->resetDescriptor();
        int enter = open(".enter", O_RDWR | O_CREAT, S_IRWXG | S_IRWXU | S_IRWXO);
        if (enter < 0)
        {
            std::cout << "ENTER FILE ERROR\n";
            status = -1;
            return ;
        }
        /*fileRead->descriptor = open(FileRead->filepath);
        if (fileRead->getDescriptor() < 0)
        {
            std::cout << "PIPE FILE ERROR\n";
            status = -1;
            return ;
        }*/
        std::vector<std::string> args_cpp;
        args_cpp.push_back(cgi);
        args_cpp.push_back(path.str());
        const char * args[3] = {args_cpp[0].c_str(), args_cpp[1].c_str(), NULL};
        int result = 0;

        pid_t pid = fork();
        if (pid < 0)
        {
            std::cout << "Fork error\n";
            //status = -1;
            code = 500;
            handleErrorPage(code);//, &(result->second));
            return ;
        }
        
        
        if (!pid)
        {
            dup2(fileRead->getDescriptor(), 1);
            dup2(fileRead->getDescriptor(), 2);
            dup2(enter, 0);
            exit(execve(cgi.c_str(), (char* const *)args, cgiEnvCreate()));
        }
        else
        {
            waitpid(pid, &result, 0);//&result
            close(enter);
            //std::cout << "Check PATH: " << fileRead->getPath() << "\n";
            /*std::ifstream f(fileRead->getPath().c_str());
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
                //std::cout << "CONTENT:\n" << content << "\nEnd\n";
                code = 200;
            }
            else
            {
                code = 500;
                
            }
            f.close();
            //std::cout << "\nADDRESS IS: " << path.str() << "\n";
            status = 2;
            formAnswer();*/
            
            
            
            //std::cout << "PATH: " << fileRead->filepath << "\n";
            //fileRead->descriptor = open(fileRead->filepath, O_RDONLY);
            
            if (result || !fileRead->resetDescriptor())
            {
                std::cout << "PIPE FILE ERROR\n";
                //status = -1;
                code = 500;
                handleErrorPage(code);//, &(result->second));
                fileRead->setStatus(-2);
                return ;
            }
            fileRead->setStatus(2);
            status = 6;
        }
    }
    
    void formAnswer()
    {
        //std::cout << "FORM ANSWER\n";
        if (status == 4)
        {
            status = 5;
            cgiResponseSimple();
            return ;
        }
        if (!code)
            code = 200;
        /*if (code == 404)
            content = "<h1>404 Not Found</h1>";*/
        //std::string contentStr = content.str();
        //std::cout << "CONTENT: " << content << "\n";
        //response << requestProtocol << " " << code << " OK\r\n";
        response << "HTTP/1.1 " << code << " OK\r\n";
        response << "Cache-Control: no-cache, private\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << content.size() << "\r\n";

        //if (!keepAlive)
        //    response << "Connection: close\r\n";
        response << "\r\n";
        response << content;
        responseSize = response.str().size() + 1;
        /*if (requestType == 1)
            status = 2;*/
        //std::cout << "End form answer\n";
    }
    
    void handleRequest(Parser *parser)
    {
        //std::cout << "HANDLE REQUEST\n";

        
        std::string request = getBufferStr();
        //std::cout << "Received:\n" << request << "\nEND\n";
        std::stringstream host;
        size_t pos = request.find("\r\nHost: ", 0);
        if (pos != std::string::npos)
        {
            pos += 8;
            size_t pos2 = request.find(":", pos);
            host << request.substr(pos, pos2 - pos);
            std::map<std::string, ServerBlock>::iterator result = port->getMap().find(host.str());
            //std::cout << "RESULT: " << result->first << "\n";// << *(result->second) << "\n";
            status = 1;
            if (result != port->getMap().end())
            {
                host.str("");
                s_block = &(result->second);
                parseHeader(request, parser);//result, parser);
            }
            //TODO else cтраница недоступна по данному порту
        }
        host.str("");
        //std::cout << "END HANDLE REQUEST\n";
    }
    
    void sendResponse()
    {
        //std::cout << "SEND RESPONSE: size = " << responseSize << " | pos = " << responsePos << " | result = " << responseSize - responsePos << "\n";
        //std::cout << "To send:\n" << response.str().substr(responsePos, responseSize - responsePos).c_str() << "\nEND\n";
        int send_size = send(descriptor, response.str().substr(responsePos, responseSize - responsePos).c_str(), responseSize - responsePos, 0);
        //std::cout << "Send returned : " << send_size << "\n";
        if (send_size <= 0)
        {
            status = -1;
            return ;
            //std::cout << "MAY BE ERROR HERE\n";
        }
            
        responsePos += send_size - 1;
        if (responsePos == responseSize - 1)
            reset();
        //std::cout << "End send response\n";
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
        //std::cout << value << "\n";
        if (!getType())
        {
            if (!value.compare(0, 4, "GET "))
            {
                //requestMethod = "GET";
                envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "GET"));
                setType(1);
            }
            else if (!value.compare(0, 5, "POST "))
            {
                //requestMethod = "POST";
                envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "POST"));
                setType(2);
            }
            else if (!value.compare(0, 7, "DELETE "))
                setType(3);
            else
                setType(0);
        }
        if (((getType() == 1 || getType() == 3) && ends_with(value, "\r\n\r\n")) || (!requestType && value.length()))
        {
            //std::cout << "IS FULL\n";
            setStatus(1);
            return (true);
        }

        size_t pos = 5;
        size_t pos2;
        if (!getLen())
        {
            std::stringstream sizestream;
            pos2 = value.find("\r\nContent-Length: ", pos);
            if (pos2 != std::string::npos)
            {
                pos2 += 18;
                pos = value.find("\r\n", pos2);
                if (pos != std::string::npos)
                {
                    sizestream << value.substr(pos2, pos - pos2);
                    //envs.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", sizestream.str()));
                    sizestream >> requestBodySize;
                    //std::cout << "SIZE = " << requestBodySize << "\n";
                    setLen(requestBodySize);
                }
                sizestream.str("");
            }
        }
        
        pos2 = value.find("\r\n\r\n", pos);
        if (pos2 == std::string::npos)
            return (false);
        pos2 += 4;
        //std::cout << "Length = " << value.length() << " | pos = " << pos2 << "\n";
        if (value.length() - getLen() == pos2)
        {
            setStatus(1);
            return (true);
        }
        return (false);
    }
    
    FileUpload *getFileWrite()
    {
        return (fileWrite);
    }
    
    FileUpload *getFileRead()
    {
        return (fileRead);
    }
    
    void resetFile(FileUpload **file)
    {
        if (*file)
        {
            //std::cout << "CLient " << getDescriptor() << " | has reseted file " << (*file)->getDescriptor() << "\n";
            (*file)->setStatus(-1);
            if (!(*file)->isConstant())
                remove(((*file)->getPath()).c_str());
            delete (*file);
            *file = 0;
        }
    }

    void handleTimeout()
    {
        if (fileWrite)
        {
            close(fileWrite->getDescriptor());
            resetFile(&fileWrite);
        }
        if (fileRead)
        {
            close(fileRead->getDescriptor());
            resetFile(&fileRead);
        }
        code = 408;
        std::cout << "Handle error start\n";
        handleErrorPage(code);
        std::cout << "Handle error end\n";
    }

    void fillContent(char c)
    {
        content += c;
    }
    
    void finishPipe()
    {
        //std::cout << "FINISH PIPE\n";
        fileRead->setStatus(2);
        //resetFile(&fileWrite);
    }

    bool shouldKeep()
    {
        return (keepAlive);
    }

    struct timeval &getTimer()
    {
        return (timer);
    }

    void setTimer()
    {
        gettimeofday(&timer, 0);
    }
};

#include "Server.hpp"
#endif
