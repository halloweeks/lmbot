cmake_minimum_required(VERSION 3.10)

project(ClientApp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(client
    src/main.c
    src/connection.c
    src/account.c
    src/log.c
    src/protocol.c
    src/des.c
    src/map_point.c
)

target_include_directories(client PRIVATE include)