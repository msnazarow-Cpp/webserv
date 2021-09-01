#include "File.hpp"

FileUpload::FileUpload(std::string filepath, int size, std::string const &content, Client *client, bool constant): _filepath(filepath), _size(size), _content(content), _client(client), pos(0), status(0), _constant(constant)
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
FileUpload::~FileUpload()
{
    _content.clear();
    _filepath.clear();
    close(descriptor);
}

void FileUpload::fileWrite()
{
    int send_size = write(descriptor, _content.substr(pos, _size - pos).c_str(), _size - pos);
    if (send_size <= 0)
    {
        status = -2;
        _content.clear();
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

int FileUpload::getDescriptor()
{
    return (descriptor);
}

int FileUpload::getStatus()
{
    return (status);
}

void FileUpload::setStatus(int val)
{
    status = val;
}

std::string FileUpload::getPath()
{
    return (_filepath);
}

bool FileUpload::resetDescriptor()
{
    close(descriptor);
    descriptor = open(_filepath.c_str(), O_RDONLY);
    if (descriptor < 0)
        return (false);
    return (true);
}

Client *FileUpload::getClient()
{
    return (_client);
}

bool FileUpload::isConstant()
{
    return (_constant);
}

void FileUpload::setConstant()
{
    _constant = true;
}
