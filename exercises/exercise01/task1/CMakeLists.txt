cmake_minimum_required(VERSION 3.27)
project(task1 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c11")


add_executable(main main.c addressbook.c addressbook.h)
add_subdirectory(includes)
