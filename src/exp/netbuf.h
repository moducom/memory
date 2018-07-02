#pragma once

#include <cstdlib>

#include "../mc/memory-chunk.h"
#include <estd/string.h>

namespace moducom { namespace mem {

template <class TNetBuf>
struct netbuf_traits
{
    typedef size_t size_type;

    // Make this specialized for YOUR particular netbuf type
    static CONSTEXPR size_type minimum_chunk_size() { return sizeof(int); }
    // Netbufs by nature are multichunk.  If your implementation is not, indicate as
    // such here
    static CONSTEXPR bool single_chunk() { return false; }
};


}}

namespace moducom { namespace io { namespace experimental {

// netbufs are inherently read and write, so wrap them up
// with reader and writer to create compiler-friendly divisions
template <class TMemoryChunk
//#if !(defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL))
        , typename TSize = size_t
//#endif
        >
class NetBufMemoryTemplate :
#if (defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL))
        public moducom::mem::ProcessedMemoryChunkBase<TMemoryChunk>
#else
        public moducom::mem::ProcessedMemoryChunkBase<TMemoryChunk, TSize>
#endif
{
public:
    typedef TMemoryChunk chunk_t;
#if (defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL))
    typedef moducom::mem::ProcessedMemoryChunkBase<TMemoryChunk> base_t;
    typedef decltype(std::declval<TMemoryChunk>().length()) size_type;
#else
    typedef TSize size_t;
    typedef moducom::mem::ProcessedMemoryChunkBase<TMemoryChunk, TSize> base_t;
#endif

public:
    NetBufMemoryTemplate() {}

    template <class TMemoryChunkInitParam>
    NetBufMemoryTemplate(const TMemoryChunkInitParam& p) :
        base_t(p)
    {}

    // Move forward to next net buf once we've exhausted this one
    // (noop for netbuf memory writer, if you're out of space, you're
    // screwed)
    bool next() { return false; }

    // Placeholder for moving back to first buffer.  For this simple NetBufMemory, it's a noop
    void first() {}

    // returns whether we're at the last 'next' chunk
    // (false means next() call will reveal additional chunk)
    bool end() const { return true; }

protected:
};


template < ::std::size_t default_size = 1024, class TAllocator = ::std::allocator<uint8_t> >
class NetBufDynamicMemory : public mem::ProcessedMemoryChunkBase<moducom::pipeline::MemoryChunk>
{
    typedef mem::ProcessedMemoryChunkBase<moducom::pipeline::MemoryChunk> base_t;
    //typedef allocator_traits<TAllocator> at_t;
    TAllocator a;

public:
    NetBufDynamicMemory() :
        base_t(a.allocate(default_size), default_size)
    {
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    NetBufDynamicMemory(NetBufDynamicMemory&& move_from) :
        base_t(std::move(move_from)),
        a(move_from.a)
    {

    }
#endif


    ~NetBufDynamicMemory()
    {
        if(chunk().data() != NULLPTR)
            a.deallocate(chunk().data(), default_size);
    }

    // Move forward to next net buf once we've exhausted this one
    // (noop for netbuf memory writer, if you're out of space, you're
    // screwed)
    bool next() { return false; }

    // Placeholder for moving back to first buffer.  For this simple NetBufMemory, it's a noop
    void first() {}
};

namespace layer2 {


template <size_t buffer_length>
class NetBufMemory :
        public NetBufMemoryTemplate<moducom::pipeline::layer1::MemoryChunk<buffer_length > >
{
};


// FIX: crappy name and only employed if we ditch inbuilt advance
// NOTE: Not in use, we're heavily favoring inbuilt advance
template <class TNetBufMemoryTemplate>
class NetBufHelper
{
    typedef TNetBufMemoryTemplate nb_t;
    typedef typename nb_t::size_t size_t;
    typedef typename nb_t::chunk_t chunk_t;

    nb_t& nb;
    size_t pos;

public:
    NetBufHelper(nb_t& nb) : nb(nb), pos(0) {}

    // get data buffer currently available
    moducom::pipeline::MemoryChunk data() const
    {
        const moducom::pipeline::MemoryChunk& chunk = nb.data();
        // FIX: make a layer1::ReadOnlyMemoryChunk to avoid this
        // nasty typecast
        return moducom::pipeline::MemoryChunk(
                nb.data() + pos, nb.length() - pos);
    }

    bool advance(size_t len)
    {
        if(pos + len > nb.length())
            return false;

        pos += len;
        return true;
    }
};

}


namespace layer3 {

template <size_t buffer_length>
class NetBufMemoryWriter :
    public NetBufMemoryTemplate<pipeline::MemoryChunk>
{
    size_t pos;
};

class NetBufMemoryReader :
    public NetBufMemoryTemplate<pipeline::MemoryChunk>
{
    pipeline::MemoryChunk chunk;
    size_t pos;

public:
    
};

}

namespace layer5 {

class INetBuf
{
public:
    virtual bool next() = 0;

    //virtual moducom::pipeline::MemoryChunk data() const = 0;
};


class NetBufMemoryWriter : public INetBuf
{
    pipeline::MemoryChunk chunk;

public:
    NetBufMemoryWriter(const pipeline::MemoryChunk& chunk) : chunk(chunk) {}

    virtual bool next() OVERRIDE { return false; }

    //virtual pipeline::MemoryChunk data() const OVERRIDE { return chunk; }
};


/** Phased out, advancing is done within netbuf itself
class NetBufHelper
{
    INetBuf& nb;
    size_t pos;

public:
    NetBufHelper(INetBuf& nb) :
            nb(nb),
            pos(0)
    {}

    bool advance(size_t len)
    {
        len += pos;
        // TODO: Do comparison on len/pos
        return  true;
    }

    inline pipeline::MemoryChunk data() const
    {
        pipeline::MemoryChunk chunk = nb.data();
        pipeline::MemoryChunk chunk2(chunk.data() + pos, chunk.length() - pos);
        return chunk2;
    }
};
 */

}


template <class TNetBuf>
class NetBufReader
{
protected:
    typedef TNetBuf netbuf_t;

    // incoming network buffer
    // TODO: for netbuf, modify APIs slightly to be more C++ std lib like, specifically
    // a size/capacity/max_size kind of thing
    netbuf_t m_netbuf;

    typedef int size_type;

public:
    template <class TNetBufInitParam>
    NetBufReader(TNetBufInitParam& netbufinitparam) :
            m_netbuf(netbufinitparam)
    {}

    bool advance(size_type amount)
    {
        // TODO: add true/false on advance to underlying netbuf itself to aid in runtime
        // detection of boundary failure
        return netbuf().advance(amount);
    }

    const netbuf_t& netbuf() const
    {
        return this->m_netbuf;
    }

    // returns available processed bytes (typically should be same as chunk size)
    size_type size() const { return netbuf().length_processed(); }

    const uint8_t* data() { return netbuf().processed(); }
};


// a thinner wrapper around netbuf, mainly adding convenience methods for
// writes
// NOTE: If we do, once again, go back to the writer managing 'pos' variable, we could
// theoretically make NetBufWriter more of a 'MemoryChunkWriter'
template <class TNetBuf>
class NetBufWriter
{
protected:
    typedef TNetBuf netbuf_t;

    // outgoing network buffer
    // TODO: for netbuf, modify APIs slightly to be more C++ std lib like, specifically
    // a size/capacity/max_size kind of thing
    netbuf_t m_netbuf;

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufWriter(TArgs&&...args) :
        m_netbuf(std::forward<TArgs>(args)...) {}
#else
    template <class TNetBufInitParam>
    NetBufWriter(TNetBufInitParam& netbufinitparam) :
            m_netbuf(netbufinitparam)
    {}
#endif

    NetBufWriter() {}

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    const netbuf_t& netbuf() const
    {
        return this->m_netbuf;
    }

    netbuf_t& netbuf()
    {
        return this->m_netbuf;
    }

#if defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL)
    typedef decltype(std::declval<TNetBuf>().length_unprocessed()) size_type;
#else
    // this represents TNetBuf size_type.  Can't easily pull this out of netbuf directly
    // due to the possibility TNetBuf might be a reference (will instead have to do fancy
    // C++11 decltype/declval things to deduce it)
    typedef int size_type;
#endif

    bool advance(size_type amount)
    {
        // TODO: add true/false on advance to underlying netbuf itself to aid in runtime
        // detection of boundary failure
        return netbuf().advance(amount);
    }

    // returns available unprocessed bytes
    size_type size() const { return netbuf().length_unprocessed(); }

    uint8_t* data() { return netbuf().unprocessed(); }

    // TODO: next should return a tri-state, success, fail, or pending
    bool next() { return netbuf().next(); }

    // TODO: make a netbuf-native version of this call, and/or do
    // some extra trickery to ensure chunk() is always efficient
    // NOTE: Be advised max_size is only chunk size, not 'total' size
    //       it seems max_size *should* be total size, perhaps we can
    //       call chunk-size 'capacity'
    size_type max_size() { return netbuf().chunk().size; }


    size_type write(const void* d, int len)
    {
        if(len > size())
        {
            size_type expand_by = size() - len;
            // TODO: Attempt an expand, one of the perks of a netbuf
            // However, do it more near the end, since and expand is not
            // gaurunteed to be contiguous
            //netbuf().expand(expand_by);
            len = size();
        }

        memcpy(data(), d, len);

#ifndef ESP_PLATFORM
        // Generates annoying warnings for ESP 
        bool advance_success = 
#endif
        advance(len);

        ASSERT_ERROR(true, advance_success, "Problem advancing through netbuf");

        // TODO: decide if we want to issue a next() call here or externally
        return len;
    }

    template <class TString>
    size_type write(TString s)
    {
        int copied = s.copy((char*)netbuf().unprocessed(), this->size());

        this->advance(copied);

        ASSERT_ERROR(false, copied > s.length(), "Somehow copied more than was available!");

        return copied;
    }

    template <int N>
    size_type write(uint8_t (&d) [N])
    {
        return write(d, N);
    }

    template <int N>
    size_type write(const uint8_t (&d) [N])
    {
        return write(d, N);
    }

    bool putchar(uint8_t byte)
    {
        netbuf().unprocessed()[0] = byte;
        return this->advance(1);
    }
};


template <class TNetBuf>
NetBufWriter<TNetBuf>& operator <<(NetBufWriter<TNetBuf>& nbw, uint8_t byte)
{
    nbw.putchar(byte);
}

template <class TNetBuf, class TChar, class TTraits, class TAllocator, class TStringTraits>
NetBufWriter<TNetBuf>& operator <<(NetBufWriter<TNetBuf>& nbw,
                                   const estd::basic_string<TChar, TTraits, TAllocator, TStringTraits>& str)
{
    nbw.write(str);
    return nbw;
}

}}}
