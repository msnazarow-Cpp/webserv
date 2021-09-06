#ifndef FILE_HPP
#define FILE_HPP
#include "Server.hpp"

class Client;

class FileUpload{
private:
    friend class Client;
    std::string _filepath;
    int _size;
    std::string _content;
    Client *_client;
    int pos;
    int status;
    int descriptor;   
    bool _constant;

public:
    FileUpload(std::string filepath, int size, std::string const &content, Client *client, bool constant);
    ~FileUpload();
    void fileWrite();
    int getDescriptor();
    int getStatus();
    void setStatus(int val);
    std::string getPath();
    bool resetDescriptor();
    Client *getClient();
    bool isConstant();
    void setConstant();
};

#endif
