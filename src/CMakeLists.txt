cmake_minimum_required(VERSION 3.2)

project(moducom_memory)

set(SOURCE_FILES
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
        mc/objstack.h
        memory.cpp
        MemoryPool.cpp
        MemoryPool.h
        string.cpp exp/netbuf.h)

add_library(moducom_memory_lib ${SOURCE_FILES})

