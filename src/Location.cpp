//
// Created by msnazarow on 14.08.2021.
//
#include "Location.hpp"
#include "Extention.hpp"
#include <ostream>
std::ostream &operator<<(std::ostream &os, const Location &d) {
    return (os << d.location << " " << d.root << " " << d.index << " " << d.autoindex << " "
    << d.client_max_body_size << " " << d.fastcgi_pass << " " << d.fastcgi_params << std::endl);
}
