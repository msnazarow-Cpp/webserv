cmake_minimum_required(VERSION 3.20)
project(C_socket_server_group_)

set(CMAKE_CXX_STANDARD 98)
include_directories(src)
add_executable(C_socket_server_group_
        src/Client.hpp
        src/Exception.hpp
        src/File.hpp
        src/main.cpp
        src/Port.hpp
        src/Server.hpp
        src/ServerBlock.hpp
        src/Parser.cpp
        src/Parser.hpp
        src/Location.hpp src/ServerBlock.cpp src/Location.cpp)
add_executable(test
        src/Parser.cpp
        src/test.cpp src/ServerBlock.cpp src/Location.cpp)
