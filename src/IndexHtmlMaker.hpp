//
// Created by msnazarow on 19.08.2021.
//

#ifndef WEBSERV_CLEAN_INDEXHTMLMAKER_HPP
#define WEBSERV_CLEAN_INDEXHTMLMAKER_HPP


#include <exception>
#include <string>

class IndexHtmlMaker {
public:
    class IndexHtmlMakerException : std::exception{};
    static std::string makeIndexFile(std::string rootDirectory, std::string requestDirectory);
};


#endif //WEBSERV_CLEAN_INDEXHTMLMAKER_HPP
