cmake_minimum_required(VERSION 3.28)
project(exercise09 C)

set(CMAKE_C_STANDARD 11)

include_directories(task1)

add_executable(task1_server
        task1/server.c)

add_executable(task1_client
        task1/client.c
)

add_executable(task2_server
        task2/server.c
)
add_executable(task2_client
        task2/client.c
)

add_executable(task3_server
        task3/server.c
)

add_executable(task3_client
        task3/client.c
)