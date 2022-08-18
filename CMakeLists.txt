cmake_minimum_required(VERSION 3.21)
project(project2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS -lpthread)

add_executable(project2
        project_2.c)
