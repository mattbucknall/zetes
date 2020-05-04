cmake_minimum_required(VERSION 2.8)
project(zetes C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wall")

add_executable(zetes-tests
    zetes-tests.c
    ../zetes.c
)

target_include_directories(zetes-tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
