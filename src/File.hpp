#ifndef FILE_HPP
#define FILE_HPP
#include "Server.hpp"

class Client;

class FileUpload{
private:
    friend class Client;
    std::string filepath;
    int size;
    std::string content;
    Client *client;
    int pos;
    int status;
    int descriptor;   
    bool constant;

public:
    FileUpload(std::string filepath, int size, std::string const &content, Client *client, bool constant): filepath(filepath), size(size), content(content), client(client), pos(0), status(0), constant(constant)
    {
        if (!constant)
        {
            descriptor = open(filepath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0777);
            //std::cout << "Descriptor = " << descriptor << "\n";
            //std::cout << "CONTENT:\n" << content << "\nEND CONTENT\n";
            //std::cout << "FILEPATH: " << this->filepath << "\n";
        }
        else
        {
            descriptor = open(filepath.c_str(), O_RDONLY);
            //std::cout << "Descriptor = " << descriptor << "\n";
            //std::cout << "CONTENT:\n" << content << "\nEND CONTENT\n";
        }
        if (descriptor < 0)
            throw Exception("File writing exception");
    }
    ~FileUpload()
    {
        close(descriptor);
    }
    
    void fileWrite()
    {
        //descriptor.write(content.c_str(), size);
        
        //std::cout << "FILE WRITE : size = " << size << " | pos = " << pos << " | result = " << size - pos << "\n";
        //std::cout << "To write:\n" << content.substr(pos, size - pos).c_str() << "\nEND\n";
        int send_size = write(descriptor, content.substr(pos, size - pos).c_str(), size - pos);
        //std::cout << "File writing returned : " << send_size << "\n";
        if (send_size <= 0)
        {
            status = -2;
            return ;
        }
        pos += send_size - 1;
        if (pos == size - 1)
        {
            status = 1;
            pos = 0;
            size = 0;
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
        return (filepath);
    }
    
    bool resetDescriptor()
    {
        close(descriptor);
        //std::cout << "TRY TO OPEN: " << filepath << "\n";
        descriptor = open(filepath.c_str(), O_RDONLY);
        if (descriptor < 0)
            return (false);
        return (true);
    }
    
    Client *getClient()
    {
        return (client);
    }

    bool isConstant()
    {
        return (constant);
    }
};

#include "Client.hpp"
#endif
