cmake_minimum_required(VERSION 3.14)
project(os4 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(os4 main.c osqueue.c osqueue.h threadPool.h threadPool.c)