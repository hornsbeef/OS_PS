cmake_minimum_required(VERSION 3.27)
project(exercise08 C)

set(CMAKE_C_STANDARD 11)

add_executable(task1_server
        task1/server.c
)

add_executable(task2_client
        task2/client.c
)

add_executable(task2_server
        task2/server.c
)

add_executable(task3_server
        task3/server.c
        task3/myqueue.h
)
