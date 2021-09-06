#include "Server.hpp"
#include "Client.hpp"

Server::Server()
{
    timeout.tv_sec = 25;
    timeout.tv_usec = 0;
    fillContentTypes();
}

Server::~Server()
{
    contentType.clear();
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
    timeout.tv_sec = 5;
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
            Client *client_sock = new Client(curport, &contentType);
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

void Server::fillContentTypes()
{
    contentType[".png"] = "image/png";
    contentType[".php"] = "text/html";
    contentType[".bmp"] = "image/bmp";
    contentType[".cgm"] = "image/cgm";
    contentType[".g3"] = "image/g3fax";
    contentType[".gif"] = "image/gif";
    contentType[".ief"] = "image/ief";
    contentType[".jpeg"] = "image/jpeg";
    contentType[".jpg"] = "image/jpeg";
    contentType[".ktx"] = "image/ktx";
    contentType[".pjpeg"] = "image/pjpeg";
    contentType[".btif"] = "image/prs.btif";
    contentType[".svg"] = "image/svg+xml";
    contentType[".tiff"] = "image/tiff";
    contentType[".psd"] = "image/vnd.adobe.photoshop";
    contentType[".uvi"] = "image/vnd.dece.graphic";
    contentType[".djvu"] = "image/vnd.djvu";
    contentType[".sub"] = "image/vnd.dvb.subtitle";
    contentType[".dwg"] = "image/vnd.dwg";
    contentType[".dxf"] = "image/vnd.dxf";
    contentType[".fbs"] = "image/vnd.fastbidsheet";
    contentType[".fpx"] = "image/vnd.fpx";
    contentType[".fst"] = "image/vnd.fst";
    contentType[".mmr"] = "image/vnd.fujixerox.edmics-mmr";
    contentType[".rlc"] = "image/vnd.fujixerox.edmics-rlc";
    contentType[".mdi"] = "image/vnd.ms-modi";
    contentType[".npx"] = "image/vnd.net-fpx";
    contentType[".wbmp"] = "image/vnd.wap.wbmp";
    contentType[".xif"] = "image/vnd.xiff";
    contentType[".webp"] = "image/webp";
    contentType[".ras"] = "image/x-cmu-raster";
    contentType[".cmx"] = "image/x-cmx";
    contentType[".fh"] = "image/x-freehand";
    contentType[".ico"] = "image/x-icon";
    contentType[".pcx"] = "image/x-pcx";
    contentType[".pic"] = "image/x-pict";
    contentType[".pnm"] = "image/x-portable-anymap";
    contentType[".pbm"] = "image/x-portable-bitmap";
    contentType[".pgm"] = "image/x-portable-graymap";
    contentType[".ppm"] = "image/x-portable-pixmap";
    contentType[".rgb"] = "image/x-rgb";
    contentType[".xbm"] = "image/x-xbitmap";
    contentType[".xpm"] = "image/x-xpixmap";
    contentType[".xwd"] = "image/x-xwindowdump";
    contentType[".adp"] = "audio/adpcm";
    contentType[".au"] = "audio/basic";
    contentType[".mid"] = "audio/midi";
    contentType[".mp4a"] = "audio/mp4";
    contentType[".mpga"] = "audio/mpeg";
    contentType[".oga"] = "audio/ogg";
    contentType[".uva"] = "audio/vnd.dece.audio";
    contentType[".eol"] = "audio/vnd.digital-winds";
    contentType[".dra"] = "audio/vnd.dra";
    contentType[".dts"] = "audio/vnd.dts";
    contentType[".dtshd"] = "audio/vnd.dts.hd";
    contentType[".lvp"] = "audio/vnd.lucent.voice";
    contentType[".pya"] = "audio/vnd.ms-playready.media.pya";
    contentType[".ecelp4800"] = "audio/vnd.nuera.ecelp4800";
    contentType[".ecelp7470"] = "audio/vnd.nuera.ecelp7470";
    contentType[".ecelp9600"] = "audio/vnd.nuera.ecelp9600";
    contentType[".rip"] = "audio/vnd.rip";
    contentType[".weba"] = "audio/webm";
    contentType[".aac"] = "audio/x-aac";
    contentType[".aif"] = "audio/x-aiff";
    contentType[".m3u"] = "audio/x-mpegurl";
    contentType[".wax"] = "audio/x-ms-wax";
    contentType[".wma"] = "audio/x-ms-wma";
    contentType[".ram"] = "audio/x-pn-realaudio";
    contentType[".rmp"] = "audio/x-pn-realaudio-plugin";
    contentType[".wav"] = "audio/x-wav";
    contentType[".3gp"] = "video/3gpp";
    contentType[".3g2"] = "video/3gpp2";
    contentType[".h261"] = "video/h261";
    contentType[".h263"] = "video/h263";
    contentType[".h264"] = "video/h264";
    contentType[".jpgv"] = "video/jpeg";
    contentType[".jpm"] = "video/jpm";
    contentType[".mj2"] = "video/mj2";
    contentType[".mp4"] = "video/mp4";
    contentType[".mpeg"] = "video/mpeg";
    contentType[".ogv"] = "video/ogg";
    contentType[".qt"] = "video/quicktime";
    contentType[".uvh"] = "video/vnd.dece.hd";
    contentType[".uvm"] = "video/vnd.dece.mobile";
    contentType[".uvp"] = "video/vnd.dece.pd";
    contentType[".uvs"] = "video/vnd.dece.sd";
    contentType[".uvv"] = "video/vnd.dece.video";
    contentType[".fvt"] = "video/vnd.fvt";
    contentType[".mxu"] = "video/vnd.mpegurl";
    contentType[".pyv"] = "video/vnd.ms-playready.media.pyv";
    contentType[".uvu"] = "video/vnd.uvvu.mp4";
    contentType[".viv"] = "video/vnd.vivo";
    contentType[".webm"] = "video/webm";
    contentType[".f4v"] = "video/x-f4v";
    contentType[".fli"] = "video/x-fli";
    contentType[".flv"] = "video/x-flv";
    contentType[".m4v"] = "video/x-m4v";
    contentType[".asf"] = "video/x-ms-asf";
    contentType[".wm"] = "video/x-ms-wm";
    contentType[".wmv"] = "video/x-ms-wmv";
    contentType[".wmx"] = "video/x-ms-wmx";
    contentType[".wvx"] = "video/x-ms-wvx";
    contentType[".avi"] = "video/x-msvideo";
    contentType[".movie"] = "video/x-sgi-movie";
    contentType[".ics"] = "text/calendar";
    contentType[".css"] = "text/css";
    contentType[".csv"] = "text/csv";
    contentType[".html"] = "text/html";
    contentType[".n3"] = "text/n3";
    contentType[".txt"] = "text/plain";
    contentType[".par"] = "text/plain-bas";
    contentType[".dsc"] = "text/prs.lines.tag";
    contentType[".rtx"] = "text/richtext";
    contentType[".sgml"] = "text/sgml";
    contentType[".tsv"] = "text/tab-separated-values";
    contentType[".t"] = "text/troff";
    contentType[".ttl"] = "text/turtle";
    contentType[".uri"] = "text/uri-list";
    contentType[".curl"] = "text/vnd.curl";
    contentType[".dcurl"] = "text/vnd.curl.dcurl";
    contentType[".mcurl"] = "text/vnd.curl.mcurl";
    contentType[".scurl"] = "text/vnd.curl.scurl";
    contentType[".fly"] = "text/vnd.fly";
    contentType[".flx"] = "text/vnd.fmi.flexstor";
    contentType[".gv"] = "text/vnd.graphviz";
    contentType[".spot"] = "text/vnd.in3d.spot";
    contentType[".jad"] = "text/vnd.sun.j2me.app-descriptor";
    contentType[".wml"] = "text/vnd.wap.wml";
    contentType[".wmls"] = "text/vnd.wap.wmlscript";
    contentType[".s"] = "text/x-asm";
    contentType[".c"] = "text/x-c";
    contentType[".f"] = "text/x-fortran";
    contentType[".java"] = "text/x-java-source";
    contentType[".p"] = "text/x-pascal";
    contentType[".etx"] = "text/x-setext";
    contentType[".uu"] = "text/x-uuencode";
    contentType[".vcs"] = "text/x-vcalendar";
    contentType[".vcf"] = "text/x-vcard";
    contentType[".yaml"] = "text/yaml";
}
