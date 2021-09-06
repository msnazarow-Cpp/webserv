#include "TextHolder.hpp"
#include "Client.hpp"

TextHolder::TextHolder(): buffer(""), content(""){}

TextHolder::~TextHolder()
{
    buffer.clear();
    content.clear();
}

void TextHolder::fillBuffer(const char *c, int buf_size)
{
    buffer.append(c, static_cast<size_t>(buf_size));
}

void TextHolder::fillBuffer(std::string const str)
{
    buffer.append(str);
}

void TextHolder::fillContent(const char *c, int buf_size)
{
    content.append(c, static_cast<size_t>(buf_size));
}

void TextHolder::fillContent(std::string const str)
{
    content = str;
}

std::string &TextHolder::getBuffer()
{
    return (buffer);
}

std::string &TextHolder::getContent()
{
    return (content);
}

void TextHolder::concatenate()
{
    buffer += content;
    content.clear();
}
