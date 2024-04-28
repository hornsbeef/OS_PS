cmake_minimum_required(VERSION 3.0)
project(exercise06 C)

set(CMAKE_C_STANDARD 11)

add_executable(exercise06_task1_atomic
        task1/task1_atomic.c
)

add_executable(exercise06_task1_mutex
        task1/task1_mutex.c
)

add_executable(exercise06_task2
        task2/task2.c
)

add_executable(exercise06_task3
        task3/task3.c
)