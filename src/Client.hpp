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
#include "TextHolder.hpp"

class Client{

/*public:
    static int count;
    static int active;*/

private:
    int descriptor;
    std::string response;
    int responseSize;
    int responsePos;
    int code;
    
    size_t requestLen;
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

    TextHolder *buffer;

public:
    Client(Port *_port);
    ~Client();
    void reset(bool val);
    int &getDescriptor();
    int getType();
    void setType(int type);
    size_t getLen();
    void setLen(size_t len);
    Port *getPort();
    int getStatus();
    void setStatus(int val);
    void setCode(int val);
    void fillErrorContent();
    bool handleErrorPage();
    bool parseHeader(Parser *parser);
    char **cgiEnvCreate();
    void cgiResponseSimple();
    void formAnswer();
    void handleRequest(Parser *parser);
    void sendResponse();
    bool ends_with(std::string const &value, std::string const &ending);
    bool is_full();
    FileUpload *getFileWrite();
    FileUpload *getFileRead();
    void resetFile(FileUpload **file);
    void finishPipe();
    bool shouldKeep();
    struct timeval &getTimer();
    void setTimer();
   //void test500(int val);
    void setKeep(bool val);
    TextHolder *getBuffer();
    void resetBuffer();
    void formRedirect(std::string redirLocation);
};

#include "Server.hpp"
#endif
