#pragma once

#include "memory-chunk.h"

// Adds extra metadata in objstack allocations for runtime
// sanity checks
#define FEATURE_MC_MEM_OBJSTACK_CHECK

namespace moducom { namespace dynamic {

//template <class TMemoryChunk = pipeline::MemoryChunk>
class ObjStack : public pipeline::MemoryChunk
{
    //typedef TMemoryChunk memory_chunk_t;
    typedef pipeline::MemoryChunk memory_chunk_t;

    //memory_chunk_t memory_chunk;
#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
    size_t max_length;
#endif

public:
    struct Descriptor
    {
        size_t size;
    };

public:
    ObjStack(const memory_chunk_t& memory_chunk) :
        memory_chunk_t(memory_chunk)
    {
#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
        max_length = memory_chunk.length();
#endif
    }

    void* alloc(size_t size)
    {
        // TODO: Asserts

#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
        ((Descriptor*)m_data)->size = size;
        m_data += sizeof(Descriptor);
        m_length -= sizeof(Descriptor);
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
        m_length += sizeof(Descriptor);
        size_t sanity_check_size = ((Descriptor*)m_data)->size;
#endif
    }


#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
    size_t free() const { return max_length - m_length; }
#endif

    template <class T>
    inline void del(T* t)
    {
        t->~T();
        free(sizeof(T));
    }
};


}}

inline void* operator new(size_t sz, moducom::dynamic::ObjStack& os)
{
    return os.alloc(sz);
}


#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
// placement delete a bit weak, only called during exception unwinding
// otherwise we have to call the objstack delete helper
inline void operator delete(void* ptr, moducom::dynamic::ObjStack& os)
{
    typedef moducom::dynamic::ObjStack::Descriptor descriptor_t;

    size_t size = ((descriptor_t*)((uint8_t*)ptr) - sizeof(descriptor_t))->size;

    os.free(size);
}
#endif

