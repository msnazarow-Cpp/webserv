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
    "\tLocation AutoIndex: " << d.autoindex << std::endl <<
    "\tLocation ClientMaxBodySize: " << d.client_max_body_size << std::endl <<
    "\tLocation CgiPass" << d.cgi_pass << std::endl);
}
