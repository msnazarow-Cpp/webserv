#ifndef FILE_HPP
#define FILE_HPP
#include "Server.hpp"

class FileUpload{
private:
    int descriptor;
    int size;
    int pos;
    int status;
    std::string content;

public:
    FileUpload(char const *filename, int size, std::string const &content): size(size), content(content), pos(0), status(0)
    {
        descriptor = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777);
        std::cout << "Descriptor = " << descriptor << "\n";
        //std::cout << "CONTENT:\n" << content << "\nEND CONTENT\n";
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
        
        std::cout << "FILE WRITE : size = " << size << " | pos = " << pos << " | result = " << size - pos << "\n";
        //std::cout << "To write:\n" << content.substr(pos, size - pos).c_str() << "\nEND\n";
        int send_size = write(descriptor, content.substr(pos, size - pos).c_str(), size - pos);
        std::cout << "File writing returned : " << send_size << "\n";
        //if (send_size <= 0)
        pos += send_size - 1;
        if (pos == size - 1)
        {
            status = -1;
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
};

#endif
