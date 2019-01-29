cmake_minimum_required(VERSION 3.13)
project(simpleSoftwareRenderer)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -g3 -pg -O0")

add_executable(simpleSoftwareRenderer main.cpp TGAImage.cpp TGAImage.h Model.cpp Model.h geometry.cpp geometry.h)