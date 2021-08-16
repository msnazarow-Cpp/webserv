//
// Created by msnazarow on 14.08.2021.
//

#ifndef C_SOCKET_SERVER_GROUP__LOCATION_HPP
#define C_SOCKET_SERVER_GROUP__LOCATION_HPP
#include <vector>
#include <string>
#include <set>
enum Method{
    GET,
    POST,
    DELETE
};
struct Location{
    std::vector<std::string> location;
    std::string root;
    std::vector<std::string> index;
    bool autoindex;
    size_t client_max_body_size;
    std::string fastcgi_pass;
    std::string fastcgi_params;
    std::set<Method> methods;
    friend std::ostream& operator<<(std::ostream &os, const Location& d);
    Location(): location(), root(), index(), autoindex(false), client_max_body_size(-1), fastcgi_pass(), fastcgi_params(){}
};
std::ostream& operator<<(std::ostream &os, const Location& d);

#endif //C_SOCKET_SERVER_GROUP__LOCATION_HPP
