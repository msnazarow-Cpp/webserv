//
// Created by msnazarow on 14.08.2021.
//
#include "ServerBlock.hpp"
#include <ostream>
#include "Extention.hpp"
bool operator==(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name == rhs.server_name;
}

bool operator!=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name != rhs.server_name;
}

bool operator<(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name < rhs.server_name;
}

bool operator<=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name <= rhs.server_name;
}

bool operator>(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name > rhs.server_name;
}

bool operator>=(const ServerBlock &lhs, const ServerBlock &rhs) {
    return lhs.server_name >= rhs.server_name;
}

std::ostream &operator<<(std::ostream &os, const ServerBlock &d) {
    return os << d.server_name << " " << d.root << " " << d.error_page << " " <<
              d.listen << " " << d.client_max_body_size << " " << d.index << std::endl << d.locations << std::endl;}
