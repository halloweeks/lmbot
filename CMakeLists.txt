cmake_minimum_required(VERSION 3.10)

project(LordsMobileBot VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(client
    src/main.c
    src/connection.c
    src/log.c
    src/protocol.c
    src/des.c
    src/map_point.c
    src/command.c
    src/config.c
)

target_compile_definitions(client PRIVATE
    VERSION="${PROJECT_VERSION}"
)

target_include_directories(client PRIVATE include)

if(WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif()
