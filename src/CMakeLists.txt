cmake_minimum_required(VERSION 3.2)

project(moducom_memory)

set(ESTDLIB_DIR ../ext/estdlib/src)

include_directories(${ESTDLIB_DIR})

add_subdirectory(${ESTDLIB_DIR} estdlib)

set(SOURCE_FILES
        mc/mem/platform.h
        mc/array-helper.h
        mc/memory-chunk.h
        mc/memory-pool.h
        mc/memory.h
        mc/memory_index.h
        mc/memory_index2.h
        mc/memory_pool.h
        mc/objstack.h
        mc/opts.h
        mc/opts-internal.h

        memory.cpp
        MemoryPool.cpp
        MemoryPool.h

        platform/lwip/lwip-netbuf.h

        exp/llpool.h
        exp/netbuf.h mc/netbuf.h)

add_library(moducom_memory_lib ${SOURCE_FILES})

