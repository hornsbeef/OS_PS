cmake_minimum_required(VERSION 3.27)
project(exercise05 C)

set(CMAKE_C_STANDARD 11)

add_executable(exercise05_task2
        task2/task2.c
)

add_executable(exercise05_task3
        task3/task3.c
)

add_executable(exercise05_task1_scheduler
        task1/scheduler.c
)
add_executable(exercise05_task1_service
        task1/service.c
)