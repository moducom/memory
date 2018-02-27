#pragma once

#include "memory-chunk.h"

// Adds extra metadata in objstack allocations for runtime
// sanity checks
//#define FEATURE_MC_MEM_OBJSTACK_CHECK

namespace moducom { namespace dynamic {

//template <class TMemoryChunk = pipeline::MemoryChunk>
class ObjStack : public pipeline::MemoryChunk
{
    //typedef TMemoryChunk memory_chunk_t;
    typedef pipeline::MemoryChunk memory_chunk_t;

    //memory_chunk_t memory_chunk;

    struct Descriptor
    {
        size_t size;
    };

public:
    ObjStack(const memory_chunk_t& memory_chunk) :
        memory_chunk_t(memory_chunk) {}

    void* alloc(size_t size)
    {
        // TODO: Asserts

#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
        ((Descriptor*)m_data)->size = size;
        m_data += sizeof(Descriptor);
#endif
        void* d = m_data;

        m_data += size;
        m_length -= size;

        return d;
    }


    void free(size_t size)
    {
        // TODO: Asserts

        m_data -= size;
        m_length += size;

#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
        m_data -= sizeof(Descriptor);
        size_t sanity_check_size = ((Descriptor*)m_data)->size;
#endif
    }
};

}}
