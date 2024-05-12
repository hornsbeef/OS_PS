cmake_minimum_required(VERSION 3.27)
project(exercise07 C)

set(CMAKE_C_STANDARD 11)

add_executable(exercise07_task2.c
        task2/task2.c
)

add_executable(exercise07_task3.c
        task3/task3.c
        task3/thread_pool.h
        task3/thread_pool.c
)