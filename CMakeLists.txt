cmake_minimum_required(VERSION 3.21)
project(single_pass_tsc C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -pedantic -Werror)

add_executable(single_pass_tsc main.c vendor/stretchy_buffer.h)