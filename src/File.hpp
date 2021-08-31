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
    FileUpload(std::string filepath, int size, std::string const &content, Client *client, bool constant): _filepath(filepath), _size(size), _content(content), _client(client), pos(0), status(0), _constant(constant)
    {
        if (!_constant)
            descriptor = open(_filepath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0777);
        else
            descriptor = open(_filepath.c_str(), O_RDONLY);
        if (descriptor < 0) {
            _content.clear();
            throw Exception("File writing exception");
        }
    }
    ~FileUpload()
    {
        _content.clear();
        _filepath.clear();
        close(descriptor);
    }
    
    void fileWrite()
    {
        //descriptor.write(content.c_str(), size);
        
        //std::cout << "FILE WRITE : size = " << size << " | pos = " << pos << " | result = " << size - pos << "\n";
        //std::cout << "To write:\n" << content.substr(pos, size - pos).c_str() << "\nEND\n";
        int send_size = write(descriptor, _content.substr(pos, _size - pos).c_str(), _size - pos);
        //std::cout << "File writing returned : " << send_size << " | Descriptor = " << descriptor << "\n";
        if (send_size <= 0)
        {
            status = -2;
            _content.clear();
            //std::cout << "FILE error of size " << send_size << "\n";
            return ;
        }
        pos += send_size - 1;
        if (pos == _size - 1)
        {
            status = 1;
            pos = 0;
            _size = 0;
            _content.clear();
        }
    }
    
    int getDescriptor()
    {
        return (descriptor);
    }
    
    int getStatus()
    {
        return (status);
    }
    
    void setStatus(int val)
    {
        status = val;
    }
    
    std::string getPath()
    {
        return (_filepath);
    }
    
    bool resetDescriptor()
    {
        close(descriptor);
        //std::cout << "TRY TO OPEN: " << filepath << "\n";
        descriptor = open(_filepath.c_str(), O_RDONLY);
        if (descriptor < 0)
            return (false);
        return (true);
    }
    
    Client *getClient()
    {
        return (_client);
    }

    bool isConstant()
    {
        return (_constant);
    }

    void setConstant()
    {
        _constant = true;
    }
};

#include "Client.hpp"
#endif
