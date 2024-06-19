cmake_minimum_required(VERSION 3.28)
project(test C)

set(CMAKE_C_STANDARD 11)

add_executable(test task1/main.c
        task1/myqueue.h)
