cmake_minimum_required(VERSION 3.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(bitfilled
    LANGUAGES CXX
    DESCRIPTION "Bit fields for modern C++"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(${PROJECT_NAME} INTERFACE)
add_subdirectory(${PROJECT_NAME})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(IS_TOPLEVEL_PROJECT TRUE)
else()
    set(IS_TOPLEVEL_PROJECT FALSE)
endif()

option(BITFILLED_TESTS "Enable unit testing" ${IS_TOPLEVEL_PROJECT})
if(BITFILLED_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
