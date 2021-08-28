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
    bool requestIsChunked;
    
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


    //bool fromCgi;
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
        requestIsChunked = false;
        //fromCgi = false;
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
        //std::cout << "COMPLETE RESET of " << descriptor << "\n";
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
        requestIsChunked = false;
        envs.clear();
        
        
        responseSize = 0;
        responsePos = 0;
        code = 0;


        //if (fromCgi)
        //    keepAlive = true;
        //fromCgi = false;


        s_block = 0;
        resetFile(&fileWrite);
        resetFile(&fileRead);
        if (keepAlive)
        {
            status = 0;
            std::cout << "Connection kept\n";
        }
        else
        {
            status = -1;
            std::cout << "Connection broken\n";
        }
        //status = 0;
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

    void fillErrorContent()
    {
        switch (code){
            case 100:{
                content = "<h1>100: Continue</h1>";
                break;
            }
            case 202:{
                content = "<h1>202: Accepted</h1>";
                break;
            }
            case 204:{
                content = "<h1>204: No content</h1>";
                break;
            }
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
            case 413:{
                content = "<h1>413: Payload Too Large</h1>";
                break;
            }
            case 422:{
                content = "<h1>422: Unprocessable entity</h1>";
                break;
            }
            case 500:{
                content = "<h1>500: Internal server error</h1>";
                break;
            }
            case 501:{
                content = "<h1>501: Not implemented</h1>";
                break;
            }
        }
    }

    bool handleErrorPage()//, ServerBlock *block)
    {
        //std::cout << "Handle error\n";
        if (!s_block)
        {
            status = -1;
            return (true);
        }
        if (code == 100 || code == 204 || code == 405)
            keepAlive = true;
        std::string error = s_block->getErrorPage(code);
        //std::cout << "Error: " << error << "\n";
        if (error.empty())
        {
            fillErrorContent();
            status = 2;
            formAnswer();
        }
        else
        {
            try{
                fileRead = new FileUpload(error, 0, "", this, true);//, false);
                fileRead->setStatus(2);
                status = 6;
            }catch(Exception &e){
                fillErrorContent();
                status = 2;
                formAnswer();
            }
        }
        return (false);
    }

    bool parseHeader(std::string &request, Parser *parser)//std::map<std::string, ServerBlock>::iterator &result, Parser *parser)
    {
        size_t pos;
        size_t pos2;
        //std::cout << "PARSE HEADER\n";
        //std::cout << "PARSE HEADER\nREQUEST:\n" << request.substr(0, 500) << "\nEND\n";
        if (requestType == 1 || requestType == 4)
            pos = 4;
        else if (requestType == 2)
            pos = 5;
        else if (requestType == 3)
            pos = 7;
        else
        {
            //content = "Wrong request type";
            //std::cout << "Wrong request type\n";
            code = 405;
            return (handleErrorPage());//, &(result->second)));
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
            return (handleErrorPage());//, &(result->second)));
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
            return (handleErrorPage());//, &(result->second)));
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
            return (handleErrorPage());//, &(result->second)));
        }
        pos2 += 6;
        pos = request.find(":", pos2);
        pos3 = request.find("\r\n", pos2);
        std::string requestHost;
        std::stringstream streamPort;
        int requestPort;
        if (pos != std::string::npos && pos < pos3)
        {
            requestHost = request.substr(pos2, pos - pos2);

            //PORT
            pos++;
            pos2 = request.find("\r\n", pos);
            if (pos2 == std::string::npos)
            {
                /*std::cout << "Wrong request header (port)\n";
                status = -1;
                return (true);*/
                code = 400;
                return (handleErrorPage());//, &(result->second)));
            }

            streamPort << request.substr(pos, pos2 - pos);

            streamPort >> requestPort;
        }
        else if (pos3 != std::string::npos)
        {
            requestHost = request.substr(pos2, pos3 - pos2);
            streamPort << "80";
            streamPort >> requestPort;
            pos2 = pos3 + 2;
        }
        else
        {
            code = 400;
            return (handleErrorPage());
        }
        //requestHost = request.substr(pos2, pos - pos2);

        envs.insert(std::pair<std::string, std::string>("SERVER_NAME", requestHost));
        //std::cout << "Request host: " << envs["SERVER_NAME"] << "\n";


        envs.insert(std::pair<std::string, std::string>("SERVER_PORT", streamPort.str()));
        streamPort.str("");
        //std::cout << "Request port: " << envs["SERVER_PORT"] << "\n";

        pos = request.find("Connection: ", pos2);
        if (pos == std::string::npos)
            keepAlive = true;
        else
        {
            pos += 12;
            pos2 = request.find("\r\n", pos);
            if (pos2 == std::string::npos)
            {
                keepAlive = true;
                envs["HTTP_CONNECTION"] = "keep-alive";
            }
            else
            {
                if ((!request.compare(pos, pos2 - pos, "close")))
                {
                    keepAlive = false;
                    envs["HTTP_CONNECTION"] = "close";
                }
                else
                {
                    keepAlive = true;
                    envs["HTTP_CONNECTION"] = "keep-alive";
                }
            }
        }

        /*if (!keepAlive)
        {
            struct linger so_linger;
            so_linger.l_onoff = true;
            so_linger.l_linger = 0;
            setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &so_linger, sizeof (so_linger));
        }*/

        /*if (requestType == 4 && status == 1)
        {
            status = 10;
            std::cout << "STATUS SET TO 10\n";
            chunkSize = 0;
            return (false);
        }*/
    //if (requestType == 4)
    //    return (true);
        bool isErrorPage = false, isLegit = true;
        int maxSize = -1;



        size_t i = 0;
        for (i = 0; i < target.size(); i++)
        {
            if (target[i] != '/')
                break ;
        }
        if (i > 1)
        {
            std::string tmp = target.substr(i - 1, target.size() + 1 - i);
            target = tmp;
        }

        path << parser->getfilename(requestHost, requestPort, target, isErrorPage, cgi, isLegit, requestType, code, maxSize, "", requestIsChunked, 0);

        //std::cout << "CGI = " << cgi << "\nPATH = " << path.str() << "\n";
        //std::cout << "Legit = " << isLegit << " | Error = " << isErrorPage << "\n";
        if (!isLegit || isErrorPage)
        {
            //std::cout << "CODE = " << code << "\n";
            return (handleErrorPage());
        }

        
        
        pos = request.find("\r\n\r\n", pos2);
        if (pos == std::string::npos)
        {
            /*std::cout << "Wrong request header (header limits)\n";
            status = -1;
            return (true);*/
            code = 400;
            return (handleErrorPage());//, &(result->second)));
        }


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
                return (handleErrorPage());//, &(result->second)));
            }
        }
        else if (requestType == 2 && !requestBodySize && !requestIsChunked)
        {
            code = 204;
            return (handleErrorPage());
        }
        std::string headerTmp = request.substr(pos2, pos - pos2 + 2);
        //std::cout << "HEADER REST:\n" << headerTmp << "\nEND\n";
        pos2 = 2;
        while ((pos3 = headerTmp.find("\r\n", pos2)) != std::string::npos)
        {
            pos = headerTmp.find(": ", pos2);
            if (pos == std::string::npos)
            {
                code = 400;
                return (handleErrorPage());
            }
            std::string t1 = "HTTP_" + headerTmp.substr(pos2, pos - pos2);
            std::string t2 = headerTmp.substr(pos + 2, pos3 - pos - 2);

            std::transform(t1.begin(), t1.end(), t1.begin(), ::toupper);
            size_t pos4 = t1.find("-");
            if (pos4 != std::string::npos)
                t1[pos4] = '_';
            //std::cout << "KEY: " << t1 << " | VALUE: " << t2 << "\n";
            envs[t1] = headerTmp.substr(pos + 2, pos3 - pos - 2);
            pos2 = pos3 + 2;
        }
        
        //envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", requestProtocol));


        /*std::string directory("");
        size_t posCur = 0, posLast = 0, posMiddle = 0;
        while ((posCur = target.find("/", posLast)) != std::string::npos)
            posLast = posCur + 1;
        if (posLast)
        {
            posCur = 0;
            //std::cout << "HERE3\n";
            while ((posCur = target.find("/", posMiddle)) != posLast - 1)
                posMiddle = posCur + 1;
            //std::cout << "HERE4\n";
            if (posMiddle)
                posMiddle--;
            posLast--;
            directory = target.substr(posMiddle, posLast - posMiddle);
            std::string newRequest = target.substr(posLast, request.size() - posLast);
            std::cout << "DIRECTORY IS: " << directory << " | NEW REQUEST: " << newRequest << "\n";
            target = newRequest;
        }*/

        size_t posCur = 0, posLast = 0;
        while ((posCur = target.find("/", posLast)) != std::string::npos)
            posLast = posCur + 1;
        if (posLast)
            envs["PATH_INFO"] = target.substr(posLast - 1, target.size() - posLast + 1);
        /*if (posLast)
        {
            size_t len1 = s_block->getRoot().size();
            std::string pathTmp = path.str();
            size_t len2 = pathTmp.size();
            envs["PATH_INFO"] = pathTmp.substr(len1, len2 - len1);
        }*/
        else
            envs["PATH_INFO"] = target;

        envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", "HTTP/1.1"));
        envs.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "whatever/0.0"));
        envs.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
        envs.insert(std::pair<std::string, std::string>("SCRIPT_NAME", path.str()));
        envs["SCRIPT_FILENAME"] = envs["SCRIPT_NAME"];
        //envs["PATH_INFO"] = path.str();

        envs["PATH_TRANSLATED"] = envs["PATH_INFO"];
        envs["REDIRECT_STATUS"] = "true";
        
        //std::cout << "PATH INFO: " << envs["PATH_INFO"] << "\n";

        //BODY
        //std::cout << "BODY SIZE IS " << getLen() << "\n";
        if (getLen() && !cgi.empty() && !requestIsChunked)
        {
            //std::cout << "We have len!\n";
            if (maxSize > -1 && maxSize < requestBodySize)
            {
                code = 413;
                return (handleErrorPage());//, &(result->second)));
            }
            pos2 = request.find("\r\n\r\n");
            if (pos2 == std::string::npos)
            {
                code = 400;
                return (handleErrorPage());//, &(result->second)));
            }
            pos2 += 4;

            //std::cout << "SGI run\n";
            requestBody = request.substr(pos2, getLen());

            try{
                //std::cout << "CGI HERE1\n";
                std::stringstream filename;
                //std::cout << "Map: " << result->second.getRoot() << "\n";
                //filename << result->second.getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                //std::cout << "Buffer: " << result->second.getBuffer() << "\n";
                requestBuffer = filename.str();
                envs["HTTP_BODY"] = requestBuffer;
                envs["HTTP_BUFFER_PATH"] = s_block->getBuffer();
                envs["HTTP_UPLOADS_PATH"] = s_block->getUploadsDir();
                //std::cout << "ENVPATH: " << envs["HTTP_UPLOADS_PATH"] << "\n";
                //std::cout << "RequestBuffer: " << requestBuffer << "\n";
                //std::stringstream tmp2;
                //tmp2 << ".buffer/." << port->getDescriptor() << "_" << descriptor;

                fileWrite = new FileUpload(requestBuffer, getLen(), requestBody, this, false);//, false);
                //std::cout << "Client " << getDescriptor() << "created fileWrite " << getFileWrite()->getDescriptor() << "\n";
                filename << "_read";
                fileRead = new FileUpload(filename.str(), 0, "", this, false);//, false);
                //std::cout << "Client " << getDescriptor() << "created fileRead " << getFileRead()->getDescriptor() << "\n";
                //envs["HTTP_TMP"] = tmp2.str();
                requestBody.clear();
                setStatus(3);
                filename.str("");
                //setStatus(-1);
                //std::cout << "end parse\n";
                return (false);
            } catch (Exception &e){
                /*std::cout << e.what() << "\n";
                setStatus(-1);
                code = 404;*/
                code = 500;
                //test500(1);
                return (handleErrorPage());//, &(result->second)));
            }
        }
        else if (requestIsChunked)
        {
            pos2 = request.find("\r\n\r\n");
            if (pos2 == std::string::npos)
            {
                code = 400;
                return (handleErrorPage());//, &(result->second)));
            }
            pos2 += 4;

            int chunkSize = 0;
            std::stringstream chunks;
            while ((pos = request.find("\r\n", pos2)) != std::string::npos)
            {
                std::stringstream lenConverter;

                //std::cout << "HEX SIZE IS " << request.substr(pos2, pos - pos2) << "\n";
                lenConverter << request.substr(pos2, pos - pos2);
                lenConverter >> std::hex >> requestBodySize;
                //std::cout << "Converted body size = " << requestBodySize << "\n";
                lenConverter.str("");
                chunkSize += requestBodySize;
                pos2 = pos + 2;
                chunks << request.substr(pos2, requestBodySize);
                pos2 = pos2 + requestBodySize + 2;
            }

            if (chunkSize < 0)
            {
                code = 500;
                //test500(2);
                return (handleErrorPage());
            }
            else if (maxSize > -1 && chunkSize > maxSize)
            {
                code = 413;
                return (handleErrorPage());
            }

            std::stringstream filename;
            //std::string filename = path.str();
            //std::cout << "filename is " << filename;
            try{
                if (requestType == 4)
                {
                    filename << path.str();
                    fileWrite = new FileUpload(filename.str(), chunkSize, chunks.str(), this, false);//, true);
                    fileWrite->setConstant();
                    fileRead = 0;
                    chunks.str("");
                    filename.str("");
                    setStatus(3);
                    return (false);
                }
                else if (!cgi.empty())
                {
                    filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
                    requestBuffer = filename.str();
                    envs["HTTP_BODY"] = requestBuffer;
                    envs["HTTP_BUFFER_PATH"] = s_block->getBuffer();
                    envs["HTTP_UPLOADS_PATH"] = s_block->getUploadsDir();
                    //std::cout << "RequestBuffer: " << requestBuffer << "\n";
                    //std::stringstream tmp2;
                    //tmp2 << ".buffer/." << port->getDescriptor() << "_" << descriptor;
                    fileWrite = new FileUpload(requestBuffer, chunkSize, chunks.str(), this, false);//, false);
                    //std::cout << "Client " << getDescriptor() << "created fileWrite " << getFileWrite()->getDescriptor() << "\n";
                    filename << "_read";
                    fileRead = new FileUpload(filename.str(), 0, "", this, false);//, false);
                    //std::cout << "Client " << getDescriptor() << "created fileRead " << getFileRead()->getDescriptor() << "\n";
                    //envs["HTTP_TMP"] = tmp2.str();
                    //fileRead = new FileUpload(filename + "_read", 0, "", this, false, false);
                    chunks.str("");
                    filename.str("");
                    setStatus(3);
                    return (false);
                }

            } catch (Exception &e){
                code = 500;
                //test500(3);
                return (handleErrorPage());
            }
            //}
        }
        else if (!envs["QUERY_STRING"].empty() && !cgi.empty())
        {
            try{
                std::stringstream filename;
                fileWrite = 0;
                //filename << result->second.getBuffer() << "/." << port->getDescriptor() << "_" << descriptor << "_read";
                filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor << "_read";
                fileRead = new FileUpload(filename.str(), 0, "", this, false);//, false);
                //std::cout << "Client " << getDescriptor() << "created fileRead " << getFileRead()->getDescriptor() << "\n";
                envs["HTTP_BODY"] = "";
                envs["HTTP_BUFFER_PATH"] = s_block->getBuffer();
                envs["HTTP_UPLOADS_PATH"] = s_block->getUploadsDir();
                std::cout << "ENVPATH: " << envs["HTTP_UPLOADS_PATH"] << "\n";
                setStatus(3);
                filename.str("");
            } catch (Exception &e){
                /*std::cout << e.what() << "\n";
                setStatus(-1);
                code = 404;*/
                code = 500;
                //test500(4);
                return (handleErrorPage());//, &(result->second)));
            }
            return (false);
        }

        /*if (requestType == 2)
        {
            status = -1;
            return (true);
        }*/

        try{
            fileRead = new FileUpload(path.str(), 0, "", this, true);//, false);
            fileRead->setStatus(2);
            status = 6;
        }catch(Exception &e){
            /*std::cout << e.what() << "\n";
            setStatus(-1);
            code = 404;*/
            code = 500;
            //test500(5);
            return (handleErrorPage());//, &(result->second)));
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
        int enter;

        if (!requestIsChunked)
        {
            //std::cout << "CLassic enter\n";
            enter = open(".enter", O_RDWR | O_CREAT, S_IRWXG | S_IRWXU | S_IRWXO);
            if (enter < 0)
            {
                //std::cout << "ENTER FILE ERROR\n";
                //status = -1;
                code = 500;
                //test500(6);
                handleErrorPage();
                return ;
            }
        }
        else
        {
            //std::cout << "Tester enter\n";
            if (!fileWrite || !fileWrite->resetDescriptor())
            {
                code = 500;
                //test500(7);
                handleErrorPage();
                return ;
            }
            //std::cout << "Tester enter done\n";
        }
        /*fileRead->descriptor = open(FileRead->filepath);
        if (fileRead->getDescriptor() < 0)
        {
            std::cout << "PIPE FILE ERROR\n";
            status = -1;
            return ;
        }*/
        //std::cout << "CGI is " << cgi << "\n";
        std::vector<std::string> args_cpp;
        args_cpp.push_back(cgi);
        args_cpp.push_back(path.str());
        const char * args[3] = {args_cpp[0].c_str(), args_cpp[1].c_str(), NULL};
        int result = 0;


        pid_t pid = fork();
        if (pid < 0)
        {
            //std::cout << "Fork error\n";
            //status = -1;
            code = 500;
            //test500(8);
            handleErrorPage();//, &(result->second));
            return ;
        }
        
        
        if (!pid)
        {
            dup2(fileRead->getDescriptor(), 1);
            dup2(fileRead->getDescriptor(), 2);
            if (!requestIsChunked)
                dup2(enter, 0);
            else
                dup2(fileWrite->getDescriptor(), 0);
            exit(execve(cgi.c_str(), (char* const *)args, cgiEnvCreate()));
        }
        else
        {
            waitpid(pid, &result, 0);//&result
            if (!requestIsChunked)
                close(enter);
            //else
            //    fromCgi = true;
            //std::cout << "Waitpid\n";
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
            bool ret = fileRead->resetDescriptor();
            if (result || !ret)
            {
                //std::cout << "828: PIPE FILE ERROR: | result = " << result << " | reset = " << ret << "\n";
                //status = -1;
                fileRead->setStatus(-2);
                //handleErrorPage(code);//, &(result->second));
                //status = -3;
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
            if (requestType != 4 && !cgi.empty())
            {
                status = 5;
                cgiResponseSimple();
                return ;
            }
            else
                status = 2;
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


        //if (!keepAlive)
        //    response << "Connection: close\r\n";
        /*if (!fromCgi)
        {
            response << "Content-Length: " << content.size() << "\r\n\r\n";
            //response << "\r\n";
        }
        else
        {*/
            size_t pos = content.find("\r\n\r\n", 0);
            if (pos != std::string::npos)
            {
                pos += 4;
                size_t newsize = content.size() - pos;
                response << "Content-Length: " << newsize << "\r\n";
            }
            else
                response << "Content-Length: " << content.size() << "\r\n\r\n";
        //}
        //if (code != 204)
        response << content;
        /*if (fromCgi)
            response << "\r\n\r\n";*/
        responseSize = response.str().size() + 1;
        /*if (requestType == 1)
            status = 2;*/
        //std::cout << "End form answer\n";
    }
    
    void handleRequest(Parser *parser)
    {
        //std::cout << "HANDLE REQUEST\n";

        /*if (!requestType)
        {
            std::cout << "Wrong request type\n";
            code = 405;
            handleErrorPage(code);
            return ;
        }*/
        std::string request = getBufferStr();
        //std::cout << "Received:\n" << request << "\nEND\n";
        std::stringstream host;
        size_t pos = request.find("\r\nHost: ", 0);
        if (pos != std::string::npos)
        {
            pos += 8;
            size_t pos2 = request.find(":", pos);
            size_t pos3 = request.find("\r\n", pos);
            if (pos2 != std::string::npos && pos3 != std::string::npos)
            {
                //std::cout << "Port found\n";
                if (pos2 < pos3)
                    host << request.substr(pos, pos2 - pos);
                else
                    host << request.substr(pos, pos3 - pos);
            }
            /*else if (pos3 == std::string::npos)
            {
                std::cout << "ERROR 400-1\n";
                code = 400;
                handleErrorPage(code);
                return ;
            }*/
            std::map<std::string, ServerBlock>::iterator result = port->getMap().find(host.str());
            //std::cout << "RESULT: " << result->first << "\n";// << *(result->second) << "\n";
            //if (status != 9)
                status = 1;
            if (result != port->getMap().end())
            {
                host.str("");
                s_block = &(result->second);
                parseHeader(request, parser);//result, parser);
            }
            else
                std::cout << "NO SUCH HOST FOR PORT\n";
            //TODO else cтраница недоступна по данному порту
        }
        else
        {
            //std::cout << "ERROR 400-2\n";
            code = 400;
            handleErrorPage();
            return ;
        }
        host.str("");
        //std::cout << "END HANDLE REQUEST\n";
    }
    
    void sendResponse()
    {
        //std::cout << "SEND RESPONSE: size = " << responseSize << " | pos = " << responsePos << " | result = " << responseSize - responsePos << "\n";
        //std::cout << "short:\n###\n" << response.str().substr(responsePos, 500) << "\n###\n";
        //std::cout << "To send:\n" << response.str().substr(responsePos, responseSize - responsePos).c_str() << "\nEND\n";
        std::cout << "Send CODE " << code << "\n";
        int send_size = send(descriptor, response.str().substr(responsePos, responseSize - responsePos - 1).c_str(), responseSize - responsePos - 1, 0);
        //std::cout << "Send returned : " << send_size << "\n";
        if (send_size <= 0)
        {
            //std::cout << "Error on response sending: " << send_size << "\n";
            status = -1;
            return ;
            //std::cout << "MAY BE ERROR HERE\n";
        }
        //std::string tester =  response.str().substr(responsePos, responseSize - responsePos);
        //std::cout << "TEster:\n" << tester << "\n";
        //sleep(10);

        responsePos += send_size;// - 1;
        if (responsePos == responseSize - 1)
        {
            //std::cout << "Response sent. responseSize = " << responseSize << " | responsePos = " << responsePos << "\n";
            reset();
        }
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
        //std::cout << "IS FULL: " << value << "\n";
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
            else if (!value.compare(0, 4, "PUT "))
            {
                //requestMethod = "POST";
                envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "PUT"));
                setType(4);
            }
            else
                setType(0);
        }
        if (((!getType() || getType() == 1 || getType() == 3) && ends_with(value, "\r\n\r\n")))// || (!requestType && value.length()))// || (getType() == 4 && status != 10)
        {
            //std::cout << "OK IS FULL\n";
            //setStatus(1);
            return (true);
        }
        /*else if (requestIsChunked && ends_with(value, "\r\n0\r\n\r\n"))
        {
            setStatus(1);
            std::cout << "PUT is full\n";
            return (true);
        }*/

        size_t pos = 5;
        size_t pos2;
        if (!getLen() && !requestIsChunked)
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
            else
            {
                pos2 = value.find("\r\nTransfer-Encoding: ", pos);
                if (pos2 != std::string::npos)
                {
                    pos2 += 21;
                    pos =  value.find("\r\n", pos2);
                    if (pos != std::string::npos)
                    {
                        sizestream << value.substr(pos2, pos - pos2);
                        std::string checkChunk = sizestream.str();
                        if (!checkChunk.compare("chunked"))
                            requestIsChunked = true;
                        sizestream.str("");
                    }

                }
            }
        }

        if (requestIsChunked)
        {
            if (ends_with(value, "\r\n0\r\n\r\n"))
            {
                setStatus(1);
                //std::cout << "PUT is full\n";
                return (true);
            }
            return (false);
        }
        pos2 = value.find("\r\n\r\n", pos);
        if (pos2 == std::string::npos)
            return (false);
        pos2 += 4;
        //std::cout << "Length = " << value.length() << " | pos = " << pos2 << "\n";
        if (!requestIsChunked && value.length() - getLen() == pos2)
        {
            setStatus(1);
            //std::cout << "STATUS SET TO 1\n";
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
        //std::cout << "Handle error start\n";
        handleErrorPage();
        //std::cout << "Handle error end\n";
    }

    void fillContent(char c)
    {
        content += c;
    }
    
    void finishPipe()
    {
        //std::cout << "FINISH PIPE\n";
        //fileRead->setStatus(2);
        //resetFile(&fileWrite);
        resetFile(&fileRead);
        code = 500;
        handleErrorPage();
        //test500(9);
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

    void test500(int val)
    {
        std::cout << "ERROR 500: " << val << "\n";
        std::cout << "request was:\n" << getBufferStr().substr(0, 500) << "\nEND\n";
        exit(0);
    }
};

#include "Server.hpp"
#endif
