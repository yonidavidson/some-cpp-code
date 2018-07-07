cmake_minimum_required(VERSION 3.10)
project(uri)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)

add_executable(uri
        src/catch.hpp
        src/lib.cpp
        src/lib.h
        src/test.cpp)
