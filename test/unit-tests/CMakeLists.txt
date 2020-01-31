cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

project(memlib-unit-tests)

add_definitions(-Werror=reorder)

# Legacy option -- remember this whole module is legacy, so re-enabling this just
# so that tests run as we phase out module overall
add_definitions(-DFEATURE_ESTD_ALLOCATOR_LOCKCOUNTER)

set(MC_MEM_DIR ../../src)
set(ESTDLIB_DIR ../../ext/estdlib/src)

include_directories(${ESTDLIB_DIR})

INCLUDE_DIRECTORIES(../../ext/Catch/single_include)
include_directories(${MC_MEM_DIR})

add_subdirectory(${MC_MEM_DIR} mcmem)

add_executable(${PROJECT_NAME}
    "main.cpp"
    "memory_pool.cpp"
    "memory_test.cpp"
    "integrity.cpp"
    "netbuf.cpp"
    experimental.cpp memory-chunk.cpp)

target_link_libraries(${PROJECT_NAME} moducom_memory_lib)
