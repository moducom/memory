cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

set(MC_MEM_DIR ../../src)

INCLUDE_DIRECTORIES(../../ext/Catch/single_include)
include_directories(${MC_MEM_DIR})

add_subdirectory(${MC_MEM_DIR} CMAKE_RUNTIME_OUTPUT_DIRECTORY)

project(unit-tests)

add_executable(${PROJECT_NAME} "main.cpp" "memory_pool.cpp" "memory_test.cpp" "string_test.cpp" experimental.cpp)

target_link_libraries(${PROJECT_NAME} moducom_memory_lib)
