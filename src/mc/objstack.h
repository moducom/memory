#pragma once

#include "memory-chunk.h"

// Adds extra metadata in objstack allocations for runtime
// sanity checks
#define FEATURE_MC_MEM_OBJSTACK_CHECK

// Performs free operations by basically moving m_data pointer directly
#define FEATURE_MC_MEM_OBJSTACK_GNUSTYLE

namespace moducom { namespace dynamic {

// TODO: split this out into layer varieties
// TODO: adapt to utilize IMemory, if we can
// TODO: Consider renaming to ObStack as that's the semi-proper gnu term
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


#ifdef FEATURE_MC_MEM_OBJSTACK_GNUSTYLE
    void free(void* objstack_ptr)
    {
        size_t differential = m_data - (uint8_t *)objstack_ptr;

        m_length += differential;
        m_data = (uint8_t *)objstack_ptr;

        // FEATURE_MC_MEM_OBJSTACK_CHECK might not work here, since
        // we can bypass freeing many objects at once here.  At a minimum
        // though we need to decrement since Descriptor occupies memory
        // just before allocated object.
#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
        m_data -= sizeof(Descriptor);
        m_length += sizeof(Descriptor);
        // the sanity_check is what might not match differential, though
        // if we are super (perhaps overly) explicit about our free operations
        // it will match
        size_t sanity_check_size = ((Descriptor*)m_data)->size;
#endif
    }

    template <class T>
    inline void del(T* t)
    {
        t->~T();
        free(t);
    }
#else
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

    template <class T>
    inline void del(T* t)
    {
        t->~T();
        free(sizeof(T));
    }
#endif



    size_t available() const { return m_length; }

#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
    size_t size() const { return max_length - m_length; }
#endif
};


}}

inline void* operator new(size_t sz, moducom::dynamic::ObjStack& os)
{
    return os.alloc(sz);
}


#if defined(FEATURE_MC_MEM_OBJSTACK_CHECK) && !defined(FEATURE_MC_MEM_OBJSTACK_GNUSTYLE)
// placement delete a bit weak, only called during exception unwinding
// otherwise we have to call the objstack delete helper
inline void operator delete(void* ptr, moducom::dynamic::ObjStack& os)
{
    typedef moducom::dynamic::ObjStack::Descriptor descriptor_t;

    size_t size = ((descriptor_t*)((uint8_t*)ptr) - sizeof(descriptor_t))->size;

    os.free(size);
}
#endif

