//
// Created by msnazarow on 14.08.2021.
//
#include "Location.hpp"
#include "Extention.hpp"
#include <ostream>
std::ostream &operator<<(std::ostream &os, const Location &d) {
    return (os <<
    "\t======Location====" << std::endl <<
    "\tLocation Request: " << d.location << std::endl <<
    "\tLocation Root: " << d.root << std::endl <<
    "\tLocation IndexFiles: " << d.index << std::endl <<
    "\tLocation AutoIndex: " << std::boolalpha << d.autoindex << std::endl <<
    "\tLocation ClientMaxBodySize: " << d.client_max_body_size << std::endl <<
    "\tLocation CgiPass: " << d.cgi_pass << std::endl);
}
std::ostream& operator<<(std::ostream& out, const BoolPlusNil value) {
    static std::map<BoolPlusNil, std::string> strings;
    if (strings.size() == 0) {
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(True);
        INSERT_ELEMENT(False);
        INSERT_ELEMENT(nil);
#undef INSERT_ELEMENT
    }

    return out << strings[value];
}
