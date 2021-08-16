#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>

#include "Port.hpp"
#include "File.hpp"
#include "Parser.hpp"

class Client{
private:
    int descriptor;
    std::stringstream buffer;
    
    std::ostringstream response;
    int responseSize;
    int responsePos;
    int code;
    
    size_t requestLen;
    std::string content;
    
    int requestType;
    std::stringstream path;
    std::string target;
    
    std::map<std::string, std::string> envs;
    //std::string requestMethod;
    std::string requestProtocol;
    //std::string requestQuery;
    //std::string requestHost;
    std::string requestBody;
    //std::string requestPort;
    //std::string requestContentType;
    std::string requestBoundary;
    
    std::string requestBuffer;
    
    int requestBodySize;
    int pipefd[2];
    
    
    Port *port;
    FileUpload *file;
    int status;
    
public:
    Client(Port *port, timeval &timeout): file(0)
    {
        //std::cout << "CLIENT TO PORT " << port->getDescriptor() << "\n";
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
        reset();
    }
    
    void reset()
    {
        buffer.str("");
        response.str("");
        path.str("");
        content.clear();
        requestBodySize = 0;
        
        target.clear();
        requestProtocol.clear();
        //requestHost.clear();
        //requestPort.clear();
        //requestQuery.clear();
        requestBody.clear();
        requestBuffer.clear();
        //requestMethod.clear();
        //requestContentType.clear();
        requestBoundary.clear();
        requestType = 0;
        
        envs.clear();
        
        
        responseSize = 0;
        responsePos = 0;
        code = 0;
        
        
        file = 0;
        status = 0;
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
    
    bool parseHeader(std::string const &request, std::map<std::string, ServerBlock>::iterator &result, Parser *parser)
    {
        size_t pos;
        size_t pos2;
        //std::cout << "PARSE HEADER\n";
        //std::cout << "PARSE HEADER\nREQUEST:\n" << request << "\nEND\n";
        if (requestType == 1)
            pos = 4;
        else if (requestType == 2)
            pos = 5;
        else 
            pos = -1; // TODO: pos не инициализируется
        //std::cout << "METHOD: #" << envs["REQUEST_METHOD"] << "#\n";
        
        
        //PATH & TARGET
        pos2 = request.find(" ", pos);
        if (pos2 == std::string::npos)
        {
            std::cout << "Wrong request header (target)\n";
            status = -1;
            return (true);
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
            std::cout << "Wrong request header (protocol)\n";
            status = -1;
            return (true);
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
            std::cout << "Wrong request header (host)\n";
            status = -1;
            return (true);
        }
        pos2 += 6;
        pos = request.find(":", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "Wrong request header (host:port)\n";
            status = -1;
            return (true);
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
            std::cout << "Wrong request header (port)\n";
            status = -1;
            return (true);
        }
        std::stringstream streamPort;
        streamPort << request.substr(pos, pos2 - pos);
        int requestPort;
        streamPort >> requestPort;
        envs.insert(std::pair<std::string, std::string>("SERVER_PORT", streamPort.str()));
        streamPort.str("");
        //std::cout << "Request port: " << envs["SERVER_PORT"] << "\n";
        
        path << parser->getfilename(requestHost, requestPort, target);
        //std::cout << "Path: " << path.str() << "\n";
        /*
        //CONTENT-TYPE
        pos = request.find("Content-Type: ", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "Wrong request header (content-type)\n";
            status = -1;
            return (true);
        }
        pos += 14;
        pos2 = request.find(";", pos);
        if (pos2 == std::string::npos)
        {
            std::cout << "Wrong request header (content-type)\n";
            status = -1;
            return (true);
        }
        //requestContentType = request.substr(pos, pos2 - pos);
        envs.insert(std::pair<std::string, std::string>("CONTENT_TYPE", request.substr(pos, pos2 - pos)));
        std::cout << "CONTENT-TYPE: #" << envs["CONTENT_TYPE"] << "#\n";
        
        //BOUNDARY
        pos = request.find("boundary=", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "Wrong request header (boundary)\n";
            status = -1;
            return (true);
        }
        pos += 9;
        pos2 = request.find("\r\n", pos);
        if (pos2 == std::string::npos)
        {
            std::cout << "Wrong request header (boundary)\n";
            status = -1;
            return (true);
        }
        requestBoundary = request.substr(pos, pos2 - pos);
        std::cout << "BOUNDARY: #" << requestBoundary << "#\n";
        */
        
        pos2 += 2;;
        pos = request.find("\r\n\r\n", pos2);
        if (pos == std::string::npos)
        {
            std::cout << "Wrong request header (header limits)\n";
            status = -1;
            return (true);
        }
        std::string headerTmp = request.substr(pos2, pos - pos2 + 2);
        
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
        
        envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", requestProtocol));
        envs.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "whatever/0.0"));
        envs.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
        envs.insert(std::pair<std::string, std::string>("SCRIPT_NAME", path.str()));
        envs["SCRIPT_FILENAME"] = envs["SCRIPT_NAME"];
        envs["PATH_INFO"] = target;
        envs["PATH_TRANSLATED"] = target;
        envs["REDIRECT_STATUS"] = true;
        
        //BODY
        if (getLen())
        {
            pos2 = request.find("\r\n\r\n");
            pos2 += 4;
            requestBody = request.substr(pos2, getLen());
            //std::cout << "Request body:\n#" << requestBody << "#\nEnd\n";
            try{
               // std::cout << "HERE1\n";
                std::stringstream filename;
                //std::cout << "Map: " << result->second.getRoot() << "\n";
                filename << result->second.getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                //std::cout << "Buffer: " << result->second.getBuffer() << "\n";
                requestBuffer = filename.str();
                //std::cout << "RequestBuffer: " << requestBuffer << "\n";
                std::stringstream tmp2;
                tmp2 << ".buffer/." << port->getDescriptor() << "_" << descriptor;
                file = new FileUpload(requestBuffer.c_str(), getLen(), requestBody);
                envs["HTTP_TMP"] = tmp2.str();
                requestBody.clear();
                setStatus(3);
                filename.str("");
                //setStatus(-1);
                //std::cout << "end parse\n";
                
            } catch (Exception &e){
                std::cout << e.what() << "\n";
                setStatus(-1);
                code = 404;
            }
        }
        else
        {
            std::ifstream f(path.str().c_str());
            if (f.good())
            {
                std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content = str;
                code = 200;
            }
            f.close();
            //std::cout << "\nADDRESS IS: " << path.str() << "\n";
            status = 2;
            formAnswer();
        }
        
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
        int enter = open(".enter", O_RDONLY);
        if (enter < 0)
        {
            std::cout << "ENTER FILE ERROR\n";
            status = -1;
            return ;
        }
        std::vector<std::string> args_cpp;
        args_cpp.push_back("./cgi/php-cgi");
        args_cpp.push_back("./www/server1/fileupload.php");
        const char * args[3] = {args_cpp[0].c_str(), args_cpp[1].c_str(), NULL};
        int result = 0;
        pipefd[0] = descriptor;
        pipe(pipefd);
        //std::cout << "Pipe = " << pipefd[0] << "\n";
        //exit(0);
        pid_t pid = fork();
        if (pid < 0)
        {
            std::cout << "Fork error\n";
            status = -1;
            return ;
        }
        
        
        if (!pid)
        {
            dup2(pipefd[1], 1);
            dup2(pipefd[1], 2);
            close(pipefd[0]);
            dup2(enter, 0);
            exit(execve("./cgi/php-cgi", (char* const *)args, cgiEnvCreate()));
        }
        else
        {
            waitpid(pid, &result, 0);
            close(pipefd[1]);
            close(enter);
            status = 6;
            /*char c;
            
            while(read(pipefd[0], &c, 1) > 0)
                content += c;
            finishPipe();*/
            /*close(pipefd[0]);
            
            resetFile();
            //std::cout << "CGI: #" << content << "#END CGI\n";
            setStatus(2);*/
        }
        /*if (!result)
            code = 200;
        else
            code = 404; //???*/
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
        if (code == 404)
        {
            //content.str("");
            content = "<h1>404 Not Found</h1>";
        }
        //std::string contentStr = content.str();
        //std::cout << "CONTENT: " << content << "\n";
        response << requestProtocol << " " << code << " OK\r\n";
        response << "Cache-Control: no-cache, private\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << content.size() << "\r\n";
        response << "\r\n";
        response << content;
        responseSize = response.str().size() + 1;
        /*if (requestType == 1)
            status = 2;*/
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
                /*if (requestType == 1)
                    handleGet(request, result);
                else if (requestType == 2)
                    handlePost(request, result);*/
                parseHeader(request, result, parser);
            }
            //else cтраница недоступна по данному порту
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
        //if (send_size <= 0)
            
        responsePos += send_size - 1;
        if (responsePos == responseSize - 1)
            reset();
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
        }
        if (getType() == 1 && ends_with(value, "\r\n\r\n"))
            return (true);

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
    
    FileUpload *getFile()
    {
        return (file);
    }
    
    void resetFile()
    {
        if (file)
        {
            file->setStatus(-1);
            remove(file->getPath());
            file = 0;
        }
    }
    
    int getPipe()
    {
        return (pipefd[0]);
    }
    
    void fillContent(char c)
    {
        content += c;
    }
    
    void finishPipe()
    {
        //std::cout << "Finish PIPE\n";
        close (pipefd[0]);
        resetFile();
        //std::cout << "CGI: #" << content << "#END CGI\n";
        setStatus(2);
        formAnswer();
    }
};

#include "Server.hpp"
#endif
