#ifndef TEXTHOLDER_HPP
#define TEXTHOLDER_HPP

class Client;

class TextHolder{
private:
    std::string buffer;
    std::string content;
public:
    friend class Client;
    TextHolder(): buffer(""), content(""){}
    ~TextHolder(){
        buffer.clear();
        content.clear();
    }
    void fillBuffer(const char *c, int buf_size)
    {
        buffer.append(c, static_cast<size_t>(buf_size));
    }
    void fillBuffer(std::string const str)
    {
        buffer.append(str);
    }
    void fillContent(const char *c, int buf_size)
    {
        content.append(c, static_cast<size_t>(buf_size));
    }
    void fillContent(std::string const str)
    {
        content = str;
    }
    std::string &getBuffer()
    {
        return (buffer);
    }
    std::string &getContent()
    {
        return (content);
    }

    void concatenate()
    {
        buffer += content;
        content.clear();
    }
};

#include "Client.hpp"
#endif