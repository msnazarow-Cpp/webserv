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
        src/ServerBlock.hpp src/Parser.cpp src/Parser.h)
add_executable(test
        src/Client.hpp
        src/Exception.hpp
        src/File.hpp
        src/Port.hpp
        src/Server.hpp
        src/ServerBlock.hpp
        src/Parser.cpp
        src/Parser.h
        src/test.cpp)
