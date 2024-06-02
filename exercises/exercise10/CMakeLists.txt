cmake_minimum_required(VERSION 3.28)
project(exercise10 C)

set(CMAKE_C_STANDARD 11)

add_executable(task1_
        task1/task1.c
)
add_executable(tas2_myprogram task2/my_program.c)

add_executable(task3_map_string task3/map_string.c
        task3/ceasar_chipher.c
        task3/xor_string.c
        task3/reverse_string.c
        task3/library_one.c
        task3/library_two.c)
