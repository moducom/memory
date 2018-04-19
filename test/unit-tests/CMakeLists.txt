cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-Werror=reorder)

set(MC_MEM_DIR ../../src)
set(ESTDLIB_DIR ../../ext/estdlib/src)

include_directories(${ESTDLIB_DIR})

INCLUDE_DIRECTORIES(../../ext/Catch/single_include)
include_directories(${MC_MEM_DIR})

add_subdirectory(${MC_MEM_DIR} CMAKE_RUNTIME_OUTPUT_DIRECTORY)

project(unit-tests)

add_executable(${PROJECT_NAME} "main.cpp" "memory_pool.cpp" "memory_test.cpp"
    "string_test.cpp"
    "integrity.cpp"
    "netbuf.cpp"
    experimental.cpp memory-chunk.cpp)

target_link_libraries(${PROJECT_NAME} moducom_memory_lib)
