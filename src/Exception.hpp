#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP
#include <iostream>
class Exception: public std::exception{
private:
    std::string message;
public:
    Exception(std::string message) throw(): message(message){}
    ~Exception() throw(){}
    const char* what() const throw()
    {
        return (message.c_str());
    }
};
#endif
