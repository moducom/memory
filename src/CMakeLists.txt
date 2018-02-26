cmake_minimum_required(VERSION 3.2)
project(moducom_memory)

set(CMAKE_CXX_STANDARD 98)

add_library(moducom_memory_lib STATIC
        mc/array-helper.h
        mc/linkedlist.cpp
        mc/linkedlist.h
        mc/memory-chunk.h
        mc/memory-pool.h
        mc/memory.h
        mc/memory_index.h
        mc/memory_index2.h
        mc/memory_pool.h
        mc/string.h
        memory.cpp
        MemoryPool.cpp
        MemoryPool.h
        string.cpp)

# create export group, doesn't work yet
# install ( TARGETS src EXPORT moducom_memory )

