//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_MEMORY_CHUNK_H
#define MC_COAP_TEST_MEMORY_CHUNK_H

// for size_t
#include <stddef.h>

// for strlen
#include <string.h>

// for uint8_t and friends
#include <stdint.h>

// for declval/decltype
#include <utility>

#include "mem/platform.h"

namespace moducom { namespace pipeline {

namespace experimental {

// remember this is experimental, not sure we really want to commit to using this down the line
struct memory_chunk_traits
{
    // true = length field represents entire size of buffer, even if only a portion of it is used
    // false = length field represents portion of buffer
    static bool is_length_absolute() { return true; }

    typedef size_t size_type;
    typedef uint8_t* pointer;
};

}

template <class TSize = size_t, class TTraits = experimental::memory_chunk_traits>
class MemoryChunkBase
{
public:
    // Not yet used
    //typedef typename TTraits::size_type size_type;
    typedef TSize size_type;
    typedef TTraits traits_t;

protected:
    size_type m_length;

public:
    size_type length() const { return m_length; }
};

} // pipeline

namespace mem {
// FIX: Not a great name, but good enough to be liberated from 'experimental'
// category.  This class wraps up a memory chunk but adds a 'pos' tracking
// capability.  It does not commit to *what* kind of processing is done
template <class TMemoryChunk
//#if !(defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL))
        , typename _size_t = size_t
//#endif
>
class ProcessedMemoryChunkBase
{
public:
    typedef TMemoryChunk chunk_t;
#if defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL)
    typedef decltype(std::declval<TMemoryChunk>().length()) size_type;
#else
    typedef size_t size_type;
#endif

protected:
    TMemoryChunk _chunk;
    size_type pos;

    ProcessedMemoryChunkBase() : _chunk(), pos(0) {}

    void reset() { pos = 0; }

public:
    ProcessedMemoryChunkBase(const TMemoryChunk& _chunk) :
            _chunk(_chunk),
            pos(0) {}

    // FIX: semi-workaround for const/non const variance of chunk_t
    template <typename TData>
    ProcessedMemoryChunkBase(TData data, size_type length) :
            _chunk(data, length),
            pos(0) {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    ProcessedMemoryChunkBase(ProcessedMemoryChunkBase&& move_from) :
        _chunk(move_from.chunk()),
        pos(move_from.pos)
    {
        new (&move_from._chunk) TMemoryChunk(NULLPTR, 0);
        move_from.pos = 0;
    }

    ProcessedMemoryChunkBase(const ProcessedMemoryChunkBase&) = default;
#endif

    const chunk_t& chunk() const { return _chunk; }

    chunk_t& chunk() { return _chunk; }

    // this retrieves processed data
    // to access processed data, go through chunk()
    uint8_t* unprocessed() { return _chunk.data(pos); }

    const uint8_t* processed() const { return _chunk.data(); }

    size_type length_unprocessed() const { return _chunk.length() - pos; }

    size_type length_processed() const { return pos; }

    // ala std::string and std::vector, this indicates how much data
    // total can be stuffed in to this memory chunk
    size_type capacity() const { return _chunk.length(); }

    // TODO: Put in bounds check here
    // potentially most visible and distinctive feature of ProcessedMemoryChunk is this
    // call and its "side affect"
    bool advance(size_type size)
    {
        pos += size;
        return true;
    }
};

}

namespace pipeline {
namespace experimental {

// basically layer3 variety here
template <class TSize = size_t>
class ReadOnlyMemoryChunk : public MemoryChunkBase<TSize>
{
protected:
    typedef MemoryChunkBase<TSize> base_t;

    uint8_t* m_data;


#ifdef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
    ReadOnlyMemoryChunk() {}
#endif

    ReadOnlyMemoryChunk(uint8_t* data, size_t length) :
        m_data(data)
    {
        base_t::m_length = length;
    }

    // NOTE: Untested.  Protected because it will accept non-const data()
    template <class TMemoryChunk>
    ReadOnlyMemoryChunk(const TMemoryChunk& chunk) :
        // FIX: We need to split out more ReadOnlyMemoryChunks, for now
        // we have this nasty const-dropper
        m_data(chunk.data())
    {
        this->m_length = chunk.length();
    }

public:
    ReadOnlyMemoryChunk(const uint8_t* data, size_t length) :
            // NOTE: semi faking this because regular MemoryChunk derives from
            // this class.  One step short of a kludge if we are careful to really
            // const our way through all of ReadOnlyMemoryChunk
            m_data((uint8_t*)data)
    {
        this->m_length = length;
    }


    ReadOnlyMemoryChunk(const ReadOnlyMemoryChunk& copy_from) :
    // NOTE: semi faking this because regular MemoryChunk derives from
    // this class.  One step short of a kludge if we are careful to really
    // const our way through all of ReadOnlyMemoryChunk
            m_data(copy_from.m_data)
            //m_length(copy_from.m_length)
    {
        this->m_length = copy_from.m_length;
    }



    template <size_t N>
    ReadOnlyMemoryChunk(const uint8_t (&data) [N])
    {
        // FIX: Would do static cast here but compiler (rightfully so) scolds us
        // this indicates strongly to me we need a templatized MemoryChunkBase
        // which splits then into the read only and non read only version - making
        // all MemoryChunks inherit from ReadOnly is neat, but likely better served
        // by move/conversion operators
        m_data = (uint8_t*)data;
        base_t::m_length = N;
    }

    // creates new memorychunk via copy of direct string pointer,
    // ascertains length with standard technique
    static ReadOnlyMemoryChunk str_ptr(const char* str)
    {
        size_t len = strlen(str);
        return ReadOnlyMemoryChunk((uint8_t*)str, len);
    }

    const uint8_t* data(size_t pos = 0) const { return m_data + pos; }

    inline uint8_t operator[](size_t index) const
    {
        return m_data[index];
    }

    // generate a new MemoryChunk with just the remainder of data starting
    // at pos
    inline ReadOnlyMemoryChunk remainder(size_t pos) const
    {
        return ReadOnlyMemoryChunk(m_data + pos, base_t::length() - pos);
    }

    // Somewhat opposite of remainder, create new chunk from the
    // first byte to the specified length
    ReadOnlyMemoryChunk subset(size_t length) const
    {
        // TODO: Assert that length doesn't override boundaries

        return ReadOnlyMemoryChunk(m_data, length);
    }

    // copies out of this chunk to array
    inline void copy_to(void* _copy_to) const
    {
        ::memcpy(_copy_to, m_data, base_t::length());
    }

    // copies up to copy_to_length (or length(), if smaller) to
    // _copy_to
    inline size_t copy_to(void* _copy_to, size_t copy_to_length) const
    {
        if(base_t::length() > copy_to_length)
        {
            ::memcpy(_copy_to, m_data, copy_to_length);
            return copy_to_length;
        }
        else
        {
            copy_to(_copy_to);
            return base_t::length();
        }
    }

};

}

// TODO: Split out naming for MemoryChunk and something like MemoryBuffer
// MemoryBuffer always includes data* but may or may not be responsible for actual buffer itself
// MemoryChunk always includes buffer too - and perhaps (havent decided yet) may not necessarily include data*
// IDEA: Call MemoryChunk 'MemoryDescriptor' to decouple expectation that inline memory is present
//       we can then refer to any MemoryDescriptor-like thing which also has inline memory as a MemoryChunk
//       this will demand a lot of renaming, but seems the clearest.  In other words:
//       MemoryDescriptor: promises data pointer and length
//       MemoryChunk: promises inline data itself, and its implicit length -
//          and adheres the MemoryDescriptor "concept" / signature
class MemoryChunk : public experimental::ReadOnlyMemoryChunk<>
{
    typedef experimental::ReadOnlyMemoryChunk<> base_t;

public:
    // interim type
    typedef experimental::ReadOnlyMemoryChunk<> readonly_t;

// Not recommended, but might be necessary under some conditions.  An
// in-place new probably preferred
#ifdef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
    void length(size_t l) { this->m_length = l; }
    void data(uint8_t* value) { m_data = value; }
#endif

    // NON const version, necessary here to overload with const from ReadOnlyMemoryChunk
    uint8_t* data(size_t pos = 0) const { return m_data + pos; }

    // TODO: If we can verify C++11 compat, make this a &&
    template <class TMemoryChunk>
    MemoryChunk(TMemoryChunk& chunk) :
        // FIX: We need to split out more ReadOnlyMemoryChunks, for now
        // we have this nasty const-dropper
        base_t((uint8_t*)chunk.data(), chunk.length())
    {}

    MemoryChunk(uint8_t* data, size_t length) : base_t(data, length)
    {
    }

    // this constructor only viable with rewritable feature
#ifdef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
    MemoryChunk() {}
#endif

    inline void set(uint8_t c, size_t length) { ::memset(m_data, c, length); }

    inline void set(uint8_t c) { set(c, m_length); }

    // wraps up a strncpy underneath
    inline void strcpy(const char* copy_from)
    {
#ifdef FEATURE_CPP_STRNCPY_S
        // WARN: Not tested at all
        ::strncpy_s((char*)m_data, length(), copy_from, length());
#else
        // strncpy will fill right pad m_data with nulls
        // not specifically desired behavior, so do not
        // count on that continuing to be present
        ::strncpy((char*)m_data, copy_from, length());
#endif
    }

    inline void memcpy(const uint8_t* copy_from, size_t length)
    {
        ::memcpy(m_data, copy_from, length);
    }

    inline void copy_from(const readonly_t& chunk)
    {
        ::memcpy(m_data, chunk.data(), chunk.length());
    }


    inline uint8_t& operator[](size_t index) const
    {
        return m_data[index];
    }

    // generate a new MemoryChunk which is a subset of the current one
    // may be more trouble than it's worth calling this function
    inline MemoryChunk carve_experimental(size_t pos, size_t len) const { return MemoryChunk(m_data + pos, len); }

    // generate a new MemoryChunk with just the remainder of data starting
    // at pos
    inline MemoryChunk remainder(size_t pos) const
    {
        // alert programmer that the resulting memory chunk is invalid
        // pos == m_length results in 0-length memory chunk, technically
        // valid but useless.  pos > m_length results in buffer overflow
        ASSERT_ERROR(false,  pos >= m_length, "pos >= length");
        return MemoryChunk(m_data + pos, m_length - pos);
    }

#ifdef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
    // Would prefer memorychunk itself to be more constant, perhaps we can
    // have a "ProcessedMemoryChunk" which includes a pos in it... ? or maybe
    // instead a ConstMemoryChunk just for those occasions..
    void advance_experimental(size_t pos)
    {
        ASSERT_ERROR(true, pos < length(), "pos >= length");
        m_data += pos; m_length -= pos;
    }
#endif
};


// General purpose ProcessedMemoryChunk, targets regular (non layered)
// MemoryChunk
class ProcessedMemoryChunk : public mem::ProcessedMemoryChunkBase<MemoryChunk>
{
public:
    ProcessedMemoryChunk(const MemoryChunk& chunk) :
            ProcessedMemoryChunkBase(chunk) {}

    ProcessedMemoryChunk(uint8_t* data, size_t length) :
            ProcessedMemoryChunkBase(data, length) {}
};

namespace layer1 {


template <size_t buffer_length>
class ReadOnlyMemoryChunk
{
protected:
    uint8_t buffer[buffer_length];

public:
    typedef size_t size_type;

    size_type length() const { return buffer_length; }

    inline int compare(const void* compare_against, size_type length)
    {
        return ::memcmp(buffer, compare_against, length);
    }

    const uint8_t* data(size_type offset = 0) const { return buffer + offset; }

    inline const uint8_t& operator[](size_type index) const
    {
        return buffer[index];
    }

    pipeline::MemoryChunk::readonly_t subset(size_type length) const
    {
        // TODO: Assert that length doesn't override boundaries

        return pipeline::MemoryChunk::readonly_t(data(), length);
    }
};

template <size_t buffer_length>
class MemoryChunk : public ReadOnlyMemoryChunk<buffer_length>
{
protected:
    typedef ReadOnlyMemoryChunk<buffer_length> base_t;

public:
    typedef size_t size_type;

    size_type length() const { return buffer_length; }

    const uint8_t* data(size_type offset = 0) const { return base_t::buffer + offset; }

    uint8_t* data(size_type offset = 0) { return base_t::buffer + offset; }

    // copies in length bytes from specified incoming buffer
    // does NOT update this->length(), both syntactically and
    // also because layer1 doesn't track that anyway
    inline void memcpy(const uint8_t* copy_from, size_type length)
    {
        ASSERT_ERROR(true, length <= buffer_length, "length <= buffer_length");

        ::memcpy(base_t::buffer, copy_from, length);
    }


    // copies in chunk from incoming buffer
    // does NOT update this->length(), both syntactically and
    // also because layer1 doesn't track that anyway
    inline void copy_from(const pipeline::MemoryChunk::readonly_t& chunk)
    {
        ::memcpy(base_t::buffer, chunk.data(), chunk.length());
    }


    inline void set(uint8_t c) { ::memset(base_t::buffer, c, buffer_length); }

    inline uint8_t& operator[](size_type index)
    {
        return base_t::buffer[index];
    }
};

}

namespace layer2 {
// Variant of layer2.  buffer pointer NOT used, but size field IS used
template<size_t buffer_length, class TSize = size_t>
class MemoryChunk :
        public MemoryChunkBase<TSize>,
        public layer1::MemoryChunk<buffer_length>
{
    typedef layer1::MemoryChunk<buffer_length> base_t;
    typedef MemoryChunkBase<TSize> base2_t;
    typedef typename base2_t::size_type size_type;

public:
    // defaults to exact size of fixed buffer
    MemoryChunk(size_type length = buffer_length) { this->length(length); }

    inline size_type length() const { return base2_t::m_length; }
    inline void length(size_type l) { this->m_length = l; }

    inline void set(uint8_t ch) { base_t::set(ch); }

    // like memcpy but also sets this->length()
    template <typename T>
    inline void set(const T* copy_from, size_type length)
    {
        this->memcpy(reinterpret_cast<const uint8_t*>(copy_from), length);
        this->length(length);
    }

    inline int compare(const void* compare_against)
    {
        return base_t::compare(compare_against, length());
    }
};

}

namespace layer3 {
template<size_t buffer_length>
class MemoryChunk : public pipeline::MemoryChunk
{
    uint8_t buffer[buffer_length];

public:
    MemoryChunk() : pipeline::MemoryChunk(buffer, buffer_length) {}

    inline void memset(uint8_t c) { ::memset(buffer, c, buffer_length); }
};

}


class PipelineMessage : public MemoryChunk
{
    // One might think of Status vs CopiedStatus as WriteStatus vs ReadStatus
public:
    // Expected this will live on a stack or instance field somewhere
    // With this we can retrieve how our message is progressing through
    // the pipelines
    struct Status
    {
    private:
        // message has reached its final destination
        bool _delivered : 1;
        // message has been copied and original no longer
        // needed
        bool _copied : 1;

        // 6 bits of things just for me!
        // TODO: Consider a flag for memory handle
        // TODO: Consider also a cheap-n-nasty RTTI for extended Statuses
        // TODO: Consider a demarcation flag to denote a packet boundary.  This gets a little tricky since
        //       some scenarios have many different boundaries to consider
        uint8_t reserved: 6;
    public:
        bool delivered() const { return _delivered; }

        void delivered(bool value) { _delivered = value; }

        bool copied() const { return _copied; }

        void copied(bool value) { _copied = value; }

        // different from regular copy in that only the message itself has been copied
        // used primarily for by-reference messages vs by-value messages.  Not sure if
        // a) we're gonna even use by-ref messages and b) if we do whether this will be
        // useful
        bool experimental_shallow_copied() const { return false; }

        // 8 bits of things just for you!
        uint8_t user : 8;

        // Consider an extended version of this with a semaphore
    };

    // this is like regular Status except it's merely copied along with each message,
    // thus partially alleviating memory management issue with regular Status*
    // naturally, because of this, copied_status can't be used to report status back to
    // the originator
    struct CopiedStatus
    {
        // // 8 bits of things just for you!
        // note in buffered situations, only the last user status is carried.  This contradicts
        // boundary behavior described below, so plan accordingly when using boundaries
        uint32_t user : 8;

        // boundary indicates a signal to a pipeline consumer (the one doing reads) that
        // possibly an action needs to be taken (boundary reached).  End of chunk is the boundary
        // boundaries tend to be application specific, so use this for boundary types 1-7, 0 being
        // no boundary.  Only restriction is that in buffered situations, larger boundary types will "eat"
        // smaller ones until buffer is read back out again:
        //
        // ...1...1...2...3...1...2...1
        //                ^       ^
        // buffer read ---+     --+
        //
        // becomes
        // ...............3.......2...
        uint32_t boundary : 3;

        // Helper flag to indicate whether get_buffer was utilized, in which case
        // we should take extra measures to avoid buffer allocation and/or copying
        uint32_t is_preferred_experimental : 1;
    } copied_status;

    Status* status;

#ifdef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
    PipelineMessage() : status(NULLPTR) {}
#endif

    PipelineMessage(uint8_t* data, size_t length) :
        MemoryChunk(data, length),
        status(NULLPTR)
    {
        copied_status.user = 0;
        copied_status.boundary = 0;
    }

    PipelineMessage(const MemoryChunk& chunk) : MemoryChunk(chunk), status(NULLPTR) {}

};

}}

#endif //MC_COAP_TEST_MEMORY_CHUNK_H
