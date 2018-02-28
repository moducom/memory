cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(../../ext/Catch/single_include)

project(unit-tests)

add_executable(${PROJECT_NAME} "main.cpp")
