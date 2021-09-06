#include "Client.hpp"

Client::Client(Port *_port, std::map<std::string, std::string> *_contentType): cgi(""), port(_port), fileWrite(0), fileRead(0), s_block(0), contentType(_contentType)
{
    sockaddr_in addr;
    int addrLen = sizeof(addr);
    descriptor = accept(port->getDescriptor(), (sockaddr *) &addr, (socklen_t *)&addrLen);
    if (descriptor < 0)
        throw Exception("Client connection exception");
    struct linger so_linger;
    so_linger.l_onoff = true;
    so_linger.l_linger = 0;
    setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    if (fcntl(descriptor, F_SETFL, O_NONBLOCK) < 0)
        throw Exception("Client fcntl exception");
    this->port = _port;
    requestType = 0;
    requestLen = 0;
    status = 0;
    responsePos = 0;
    code = 0;
    keepAlive = false;
    gettimeofday(&timer, 0);
    buffer = new TextHolder();
    requestIsChunked = false;
    //++count;
    //++active;
}
    
Client::~Client()
{
    close(descriptor);
    reset(true);
    //--active;
}
    
void Client::reset(bool val)
{
    requestBody.clear();
    response.clear();
    path.str("");

    requestBodySize = 0;
    requestLen = 0;
    cgi.clear();
    target.clear();
    requestProtocol.clear();

    requestBuffer.clear();
    requestBoundary.clear();
    requestType = 0;
    requestIsChunked = false;
    envs.clear();

    responseSize = 0;
    responsePos = 0;
    code = 0;

    s_block = 0;
    resetFile(&fileWrite);
    resetFile(&fileRead);
    if (!val)
        resetBuffer();
    else
        delete buffer;
    if (keepAlive)
        status = 0;
    else
        status = -1;
}
    
int &Client::getDescriptor()
{
    return (descriptor);
}

int Client::getType()
{
    return (requestType);
}

void Client::setType(int type)
{
    requestType = type;
}

size_t Client::getLen()
{
    return (requestLen);
}

void Client::setLen(size_t len)
{
    requestLen = len;
}

Port *Client::getPort()
{
    return (port);
}

int Client::getStatus()
{
    return (status);
}

void Client::setStatus(int val)
{
    status = val;
}

void Client::setCode(int val)
{
    code = val;
}

void Client::fillErrorContent()
{
    switch (code) {
        case 100: {
            buffer->fillContent("<h1>100: Continue</h1>");
            break;
        }
        case 202: {
            buffer->fillContent("<h1>202: Accepted</h1>");
            break;
        }
        case 204: {
            buffer->fillContent("<h1>204: No content</h1>");
            break;
        }
        case 400: {
            buffer->fillContent("<h1>400: Bad request</h1>");
            break;
        }
        case 401: {
            buffer->fillContent("<h1>401: Authorization required</h1>");
            break;
        }
        case 403: {
            buffer->fillContent("<h1>403: Access forbidden</h1>");
            break;
        }
        case 404: {
            buffer->fillContent("<h1>404: Not Found</h1>");
            break;
        }
        case 405: {
            buffer->fillContent("<h1>405: Request type not allowed</h1>");
            break;
        }
        case 408: {
            buffer->fillContent("<h1>408: Timeout</h1>");
            break;
        }
        case 411: {
            buffer->fillContent("<h1>411: Length required</h1>");
            break;
        }
        case 413: {
            buffer->fillContent("<h1>413: Payload Too Large</h1>");
            break;
        }
        case 422: {
            buffer->fillContent("<h1>422: Unprocessable entity</h1>");
            break;
        }
        case 500: {
            buffer->fillContent("<h1>500: Internal server error</h1>");
            break;
        }
        case 501: {
            buffer->fillContent("<h1>501: Not implemented</h1>");
            break;
        }
    }
}

bool Client::handleErrorPage()
{
    //std::cout << "Handle error\n";
    resetBuffer();
    if (!s_block) {
        status = -1;
        return (true);
    }
    std::string error = s_block->getErrorPage(code);
    if (error.empty()) {
        fillErrorContent();
        status = 2;
        formAnswer();
    }
    else
    {
        try {
            if (fileRead)
                resetFile(&fileRead);
            fileRead = new FileUpload(error, 0, "", this, true);
            fileRead->setStatus(2);
            status = 6;
        } catch(Exception &e) {
            fillErrorContent();
            status = 2;
            formAnswer();
        }
    }
    return (false);
}

bool Client::parseHeader(Parser *parser)
{
    size_t pos;
    size_t pos2;
    if (requestType == 1 || requestType == 4)
        pos = 4;
    else if (requestType == 2)
        pos = 5;
    else if (requestType == 3)
        pos = 7;
    else
    {
        code = 405;
        return (handleErrorPage());
    }

    //PATH & TARGET
    pos2 = buffer->getBuffer().find(" ", pos);
    if (pos2 == std::string::npos)
    {
        code = 400;
        return (handleErrorPage());
    }
    path << buffer->getBuffer().substr(pos, pos2 - pos);
    target = path.str();
    envs["REQUEST_URI"] = target;
    size_t pos3 = path.str().find("?", 0);
    if (pos3 != std::string::npos) {
        pos3++;
        envs.insert(std::pair<std::string, std::string>("QUERY_STRING", target.substr(pos3, target.length() - pos3)));
        target = path.str().substr(0, pos3 - 1);
    }
    else
        envs.insert(std::pair<std::string, std::string>("QUERY_STRING", ""));
    path.str("");

    //PROTOCOL
    pos2++;
    pos = buffer->getBuffer().find("\r\n", pos2);
    if (pos == std::string::npos) {
        code = 400;
        return (handleErrorPage());
    }
    requestProtocol = buffer->getBuffer().substr(pos2, pos - pos2);

    //HOSTNAME
    pos += 2;
    pos2 = buffer->getBuffer().find("Host: ", pos);
    if (pos2 == std::string::npos) {
        code = 400;
        return (handleErrorPage());
    }
    pos2 += 6;
    pos = buffer->getBuffer().find(":", pos2);
    pos3 = buffer->getBuffer().find("\r\n", pos2);
    std::string requestHost;
    std::stringstream streamPort;
    int requestPort;
    if (pos != std::string::npos && pos < pos3) {
        requestHost = buffer->getBuffer().substr(pos2, pos - pos2);

        //PORT
        pos++;
        pos2 = buffer->getBuffer().find("\r\n", pos);
        if (pos2 == std::string::npos) {
            code = 400;
            return (handleErrorPage());
        }
        streamPort << buffer->getBuffer().substr(pos, pos2 - pos);
        streamPort >> requestPort;
    }
    else if (pos3 != std::string::npos)
    {
        requestHost = buffer->getBuffer().substr(pos2, pos3 - pos2);
        streamPort << "80";
        streamPort >> requestPort;
        pos2 = pos3 + 2;
    }
    else
    {
        code = 400;
        return (handleErrorPage());
    }
    envs.insert(std::pair<std::string, std::string>("SERVER_NAME", requestHost));
    envs.insert(std::pair<std::string, std::string>("SERVER_PORT", streamPort.str()));
    streamPort.str("");

    //CONNECTION
    pos = buffer->getBuffer().find("Connection: ", pos2);
    if (pos == std::string::npos)
        keepAlive = true;
    else {
        pos += 12;
        pos2 = buffer->getBuffer().find("\r\n", pos);
        if (pos2 == std::string::npos) {
            keepAlive = true;
            envs["HTTP_CONNECTION"] = "keep-alive";
        } else {
            if ((!buffer->getBuffer().compare(pos, pos2 - pos, "close"))) {
                keepAlive = false;
                envs["HTTP_CONNECTION"] = "close";
            } else {
                keepAlive = true;
                envs["HTTP_CONNECTION"] = "keep-alive";
            }
        }
    }

    bool isErrorPage = false, isLegit = true;
    int maxSize = -1;
    size_t i = 0;
    for (i = 0; i < target.size(); i++)
    {
        if (target[i] != '/')
            break;
    }
    bool correctPath = false;
    if (i > 1)
    {
        correctPath = true;
        std::string tmp = target.substr(i - 1, target.size() + 1 - i);
        target = tmp;
    }
    path << parser->getfilename(requestHost, requestPort, target, isErrorPage, cgi, isLegit, requestType, code, maxSize, "", requestIsChunked, 0);
    if (code == 301 || code == 302) {
        formRedirect(path.str());
        path.str("");
        return (false);
    }

    if (!isLegit || isErrorPage)
        return (handleErrorPage());

    pos = buffer->getBuffer().find("\r\n\r\n", pos2);
    if (pos == std::string::npos) {
        code = 400;
        return (handleErrorPage());
    }

    if (requestType == 3) {
        if (!rmdir(path.str().c_str()) || !remove(path.str().c_str())) {
            resetBuffer();
            buffer->fillContent("<h1>File " + target + " deleted successfully</h1>");
            code = 200;
            status = 2;
            formAnswer();
            return (false);
        } else {
            code = 404;
            return (handleErrorPage());
        }
    }
    else if (requestType == 2 && !requestBodySize && !requestIsChunked && envs["QUERY_STRING"].empty()) {
        code = 204;
        return (handleErrorPage());
    }

    //HEADER HTTP
    std::string headerTmp = buffer->getBuffer().substr(pos2, pos - pos2 + 2);
    pos2 = 2;
    while ((pos3 = headerTmp.find("\r\n", pos2)) != std::string::npos)
    {
        pos = headerTmp.find(": ", pos2);
        if (pos == std::string::npos) {
            code = 400;
            return (handleErrorPage());
        }
        std::string t1 = "HTTP_" + headerTmp.substr(pos2, pos - pos2);
        std::string t2 = headerTmp.substr(pos + 2, pos3 - pos - 2);
        envs[t1] = headerTmp.substr(pos + 2, pos3 - pos - 2);
        pos2 = pos3 + 2;
    }

    if (correctPath) {
        size_t posCur = 0, posLast = 0;
        while ((posCur = target.find("/", posLast)) != std::string::npos)
            posLast = posCur + 1;
        if (posLast)
            envs["PATH_INFO"] = target.substr(posLast - 1, target.size() - posLast + 1);
        else
            envs["PATH_INFO"] = target;
    }
    else
        envs["PATH_INFO"] = target;

    envs.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", "HTTP/1.1"));
    envs.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "whatever/0.0"));
    envs.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
    envs.insert(std::pair<std::string, std::string>("SCRIPT_NAME", path.str()));
    envs["SCRIPT_FILENAME"] = envs["SCRIPT_NAME"];
    envs["PATH_TRANSLATED"] = envs["PATH_INFO"];
    envs["REDIRECT_STATUS"] = "true";
    envs["HTTP_ROOT"] = s_block->getRoot();
    envs["BUFFER_IS_ROOT"] = s_block->getIsBufferRoot();
    envs["UPLOADS_IS_ROOT"] = s_block->getIsUploadsRoot();

    //BODY
    if (getLen() && !cgi.empty() && !requestIsChunked) {
        if (maxSize > -1 && maxSize < requestBodySize) {
            code = 413;
            return (handleErrorPage());
        }
        pos2 = buffer->getBuffer().find("\r\n\r\n");
        if (pos2 == std::string::npos) {
            code = 400;
            return (handleErrorPage());
        }
        pos2 += 4;
        requestBody = buffer->getBuffer().substr(pos2, getLen());
        resetBuffer();

        try {
            std::stringstream filename;
            filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor;
            requestBuffer = filename.str();
            envs["HTTP_BODY"] = requestBuffer;
            envs["HTTP_BUFFER_PATH"] = s_block->getBuffer();
            envs["HTTP_UPLOADS_PATH"] = s_block->getUploadsDir();
            fileWrite = new FileUpload(requestBuffer, getLen(), requestBody, this, false);
            filename << "_read";
            fileRead = new FileUpload(filename.str(), 0, "", this, false);
            requestBody.clear();
            setStatus(3);
            filename.str("");
            return (false);
        } catch (Exception &e) {
            code = 500;
            return (handleErrorPage());
        }
    }
    else if (requestIsChunked)
    {
        pos2 = buffer->getBuffer().find("\r\n\r\n");
        if (pos2 == std::string::npos)
        {
            code = 400;
            return (handleErrorPage());
        }
        pos2 += 4;
        int chunkSize = 0;
        std::stringstream chunks;
        while ((pos = buffer->getBuffer().find("\r\n", pos2)) != std::string::npos)
        {
            std::stringstream lenConverter;
            lenConverter << buffer->getBuffer().substr(pos2, pos - pos2);
            lenConverter >> std::hex >> requestBodySize;
            lenConverter.str("");
            chunkSize += requestBodySize;
            pos2 = pos + 2;
            chunks << buffer->getBuffer().substr(pos2, requestBodySize);
            pos2 = pos2 + requestBodySize + 2;
        }
        if (chunkSize < 0)
        {
            code = 500;
            chunks.str("");
            return (handleErrorPage());
        }
        else if (maxSize > -1 && chunkSize > maxSize)
        {
            code = 413;
            chunks.str("");
            return (handleErrorPage());
        }
        resetBuffer();
        std::stringstream filename;

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
                fileWrite = new FileUpload(requestBuffer, chunkSize, chunks.str(), this, false);
                filename << "_read";
                fileRead = new FileUpload(filename.str(), 0, "", this, false);//, false);
                chunks.str("");
                filename.str("");
                setStatus(3);
                return (false);
            }

        } catch (Exception &e){
            code = 500;
            return (handleErrorPage());
        }
    }
    else if (!envs["QUERY_STRING"].empty() && !cgi.empty())
    {
        try {
            std::stringstream filename;
            fileWrite = 0;
            filename << s_block->getBuffer() << "/." << port->getDescriptor() << "_" << descriptor << "_read";
            fileRead = new FileUpload(filename.str(), 0, "", this, false);
            envs["HTTP_BODY"] = "";
            envs["HTTP_BUFFER_PATH"] = s_block->getBuffer();
            envs["HTTP_UPLOADS_PATH"] = s_block->getUploadsDir();
            setStatus(3);
            filename.str("");
            resetBuffer();
        } catch (Exception &e) {
            code = 500;
            return (handleErrorPage());
        }
        return (false);
    }

    try {
        fileRead = new FileUpload(path.str(), 0, "", this, true);//, false);
        fileRead->setStatus(2);
        status = 6;
        resetBuffer();
    } catch(Exception &e) {
        code = 500;
        return (handleErrorPage());
    }
    return (false);
}

char **Client::cgiEnvCreate()
{
    char **result = new char *[envs.size() + 1];
    result[envs.size()] = 0;
    size_t i = 0;
    for (std::map<std::string, std::string>::iterator it = envs.begin(); it != envs.end(); it++)
        result[i++] = strdup((it->first + "=" + it->second).c_str());
    return (result);
}

void Client::cgiResponseSimple()
{
    int enter;

    if (!requestIsChunked) {
        enter = open(".enter", O_RDWR | O_CREAT, S_IRWXG | S_IRWXU | S_IRWXO);
        if (enter < 0) {
            code = 500;
            handleErrorPage();
            return ;
        }
    } else {
        if (!fileWrite || !fileWrite->resetDescriptor()) {
            code = 500;
            handleErrorPage();
            return ;
        }
    }
    std::vector<std::string> args_cpp;
    args_cpp.push_back(cgi);
    args_cpp.push_back(path.str());
    const char * args[3] = {args_cpp[0].c_str(), args_cpp[1].c_str(), NULL};
    int result = 0;
    struct stat cgi_checker;
    if (stat(cgi.c_str(), &cgi_checker) || S_ISDIR(cgi_checker.st_mode) || !(cgi_checker.st_mode & S_IEXEC)) {
        code = 500;
        handleErrorPage();
        return ;
    }

    pid_t pid = fork();
    if (pid < 0) {
        code = 500;
        handleErrorPage();
        return ;
    }
    if (pid == 0) {
        dup2(fileRead->getDescriptor(), 1);
        dup2(fileRead->getDescriptor(), 2);
        if (!requestIsChunked)
            dup2(enter, 0);
        else
            dup2(fileWrite->getDescriptor(), 0);
        exit(execve(cgi.c_str(), (char* const *)args, cgiEnvCreate()));
    } else {
        waitpid(pid, &result, 0);
        if (!requestIsChunked)
            close(enter);
        bool ret = fileRead->resetDescriptor();
        if (result || !ret) {
            code = 500;
            handleErrorPage();
            return ;
        }
        fileRead->setStatus(2);
        status = 6;
    }
}

void Client::fillContentType()
{
    size_t pos, posRes = 0;
    while ((pos = target.find(".", posRes)) != std::string::npos)
        posRes = pos + 1;
    if (!posRes) {
        buffer->fillBuffer("Content-Type: text/html\r\n");
        return ;
    }
    posRes--;
    std::string ending(target.substr(posRes, target.size() - posRes));
    std::map<std::string, std::string>::iterator it = contentType->find(ending);
    if (it != contentType->end()) {
        buffer->fillBuffer("Content-Type: " + (*it).second + "\r\n");
        return ;
    }
    buffer->fillBuffer("Content-Type: text/html\r\n");
}

void Client::formAnswer()
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
    std::stringstream codeStr;
    codeStr << code;
    buffer->fillBuffer("HTTP/1.1 " + codeStr.str() + " OK\r\nCache-Control: no-cache, private\r\n");
    codeStr.str("");
    if (code == 200 && !ends_with(target, ".html"))
        fillContentType();
    else
        buffer->fillBuffer("Content-Type: text/html\r\n");

    size_t pos = buffer->getContent().find("\r\n\r\n", 0);
        if (pos != std::string::npos)
        {
            pos += 4;
            size_t newsize = buffer->getContent().size() - pos;
            codeStr << newsize;
            buffer->fillBuffer("Content-Length: " + codeStr.str() + "\r\n");
        }
        else {
            codeStr << buffer->getContent().size();
            buffer->fillBuffer("Content-Length: " + codeStr.str() + "\r\n\r\n");
        }
        codeStr.str("");
    buffer->concatenate();
    responseSize = buffer->getBuffer().size() + 1;
}

void Client::formRedirect(std::string redirLocation)
{
    resetBuffer();
    std::stringstream codeStr;
    codeStr << code;
    buffer->fillBuffer("HTTP/1.1 " + codeStr.str());
    codeStr.str("");

    if (code == 301)
        buffer->fillBuffer(" Moved Permanently\r\nLocation: " + redirLocation);
    else if (code == 302)
        buffer->fillBuffer(" Found\r\nLocation: " + redirLocation);
    /*if (port->getPort() != 80) {
        codeStr << port->getPort();
        buffer->fillBuffer(":" + codeStr.str());
        codeStr.str("");
    }*/
    buffer->fillBuffer("\r\n\r\n");
    responseSize = buffer->getBuffer().size() + 1;
    status = 2;
}

void Client::handleRequest(Parser *parser)
{
    //std::cout << "HANDLE REQUEST\n";
    std::string host;
    size_t pos = buffer->getBuffer().find("\r\nHost: ", 0);
    if (pos != std::string::npos)
    {
        pos += 8;
        size_t pos2 = buffer->getBuffer().find(":", pos);
        size_t pos3 = buffer->getBuffer().find("\r\n", pos);
        if (pos2 != std::string::npos && pos3 != std::string::npos)
        {
            if (pos2 < pos3)
                host = buffer->getBuffer().substr(pos, pos2 - pos);
            else
                host = buffer->getBuffer().substr(pos, pos3 - pos);
        }
        std::map<std::string, ServerBlock>::iterator result = port->getMap().find(host);
        status = 1;
        if (result != port->getMap().end())
        {
            host.clear();
            s_block = &(result->second);
            if (s_block->getRedirect().empty()) {
                parseHeader(parser);
            } else {
                if (s_block->redirIsTemp())
                    code = 302;
                else
                    code = 301;
                formRedirect(s_block->getRedirect());
            }
        }
        else
            status = -1;
    }
    else
    {
        code = 400;
        handleErrorPage();
        return ;
    }
    host.clear();
    //std::cout << "END HANDLE REQUEST\n";
}

void Client::sendResponse()
{
    //std::cout << "SEND RESPONSE: size = " << responseSize << " | pos = " << responsePos << " | result = " << responseSize - responsePos << "\n";
    //std::cout << "short:\n###\n" << buffer->getBuffer().substr(responsePos, 1500) << "\n###\n";
    //std::cout << "To send:\n" << response.str().substr(responsePos, responseSize - responsePos).c_str() << "\nEND\n";
    //std::cout << "Send CODE " << code << "\n";
    int send_size = send(descriptor, buffer->getBuffer().substr(responsePos, responseSize - responsePos - 1).c_str(), responseSize - responsePos - 1, 0);
    //std::cout << "Send returned : " << send_size << "\n";
    if (send_size <= 0)
    {
        status = -1;
        return ;
    }
    responsePos += send_size;
    if (responsePos == responseSize - 1)
        reset(false);
    //std::cout << "End send response\n";
}

bool Client::ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return (false);
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool Client::is_full()
{
    //std::cout << "IS FULL: " << value << "\n";
    if (!getType())
    {
        if (!buffer->getBuffer().compare(0, 4, "GET "))
        {
            envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "GET"));
            setType(1);
        }
        else if (!buffer->getBuffer().compare(0, 5, "POST "))
        {
            envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "POST"));
            setType(2);
        }
        else if (!buffer->getBuffer().compare(0, 7, "DELETE "))
            setType(3);
        else if (!buffer->getBuffer().compare(0, 4, "PUT "))
        {
            envs.insert(std::pair<std::string, std::string>("REQUEST_METHOD", "PUT"));
            setType(4);
        }
        else
            setType(0);
    }

    if (((!getType() || getType() == 1 || getType() == 3) && ends_with(buffer->getBuffer(), "\r\n\r\n")))
        return (true);

    size_t pos = 5;
    size_t pos2;
    if (!getLen() && !requestIsChunked)
    {
        std::stringstream sizestream;
        pos2 = buffer->getBuffer().find("\r\nContent-Length: ", pos);
        if (pos2 != std::string::npos)
        {
            pos2 += 18;
            pos = buffer->getBuffer().find("\r\n", pos2);
            if (pos != std::string::npos)
            {
                sizestream << buffer->getBuffer().substr(pos2, pos - pos2);
                sizestream >> requestBodySize;
                setLen(requestBodySize);
            }
            sizestream.str("");
        }
        else
        {
            pos2 = buffer->getBuffer().find("\r\nTransfer-Encoding: ", pos);
            if (pos2 != std::string::npos)
            {
                pos2 += 21;
                pos =  buffer->getBuffer().find("\r\n", pos2);
                if (pos != std::string::npos)
                {
                    sizestream << buffer->getBuffer().substr(pos2, pos - pos2);
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
        if (ends_with(buffer->getBuffer(), "\r\n0\r\n\r\n"))
        {
            setStatus(1);
            return (true);
        }
        return (false);
    }
    pos2 = buffer->getBuffer().find("\r\n\r\n", pos);
    if (pos2 == std::string::npos)
        return (false);
    pos2 += 4;

    if (!requestIsChunked && buffer->getBuffer().length() - getLen() == pos2)
        return (true);
    return (false);
}

FileUpload *Client::getFileWrite()
{
    return (fileWrite);
}

FileUpload *Client::getFileRead()
{
    return (fileRead);
}

void Client::resetFile(FileUpload **file)
{
    if (*file)
    {
        (*file)->setStatus(-1);
        if (!(*file)->isConstant())
            remove(((*file)->getPath()).c_str());
        delete (*file);
        *file = 0;
    }
}

void Client::finishPipe()
{
    resetFile(&fileRead);
    code = 500;
    handleErrorPage();
    //test500(9);
}

bool Client::shouldKeep()
{
    return (keepAlive);
}

struct timeval &Client::getTimer()
{
    return (timer);
}

void Client::setTimer()
{
    gettimeofday(&timer, 0);
}

/*void Client::test500(int val)
{
    std::cout << "ERROR 500: " << val << "\n";
    std::cout << "request was:\n" << getBufferStr().substr(0, 500) << "\nEND\n";
    exit(0);
}*/

void Client::setKeep(bool val)
{
    keepAlive = val;
}

TextHolder *Client::getBuffer()
{
    return (buffer);
}

void Client::resetBuffer()
{
    delete buffer;
    buffer = new TextHolder();
}
