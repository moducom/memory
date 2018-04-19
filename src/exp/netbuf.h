#pragma once

#include <cstdlib>

#include "../mc/memory-chunk.h"

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
        public moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk>
#else
        public moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk, TSize>
#endif
{
public:
    typedef TMemoryChunk chunk_t;
#if (defined(FEATURE_CPP_DECLTYPE) && defined(FEATURE_CPP_DECLVAL))
    typedef moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk> base_t;
    typedef decltype(std::declval<TMemoryChunk>().length()) size_type;
#else
    typedef TSize size_t;
    typedef moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk, TSize> base_t;
#endif

public:
    NetBufMemoryTemplate() {}

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

    // outgoing network buffer
    // TODO: for netbuf, modify APIs slightly to be more C++ std lib like, specifically
    // a size/capacity/max_size kind of thing
    netbuf_t m_netbuf;
};


// a thinner wrapper around netbuf, mainly adding convenience methods for
// writes
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
    template <class TNetBufInitParam>
    NetBufWriter(TNetBufInitParam& netbufinitparam) :
            m_netbuf(netbufinitparam)
    {}

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
        netbuf().advance(amount);
        return true;
    }

    // returns available unprocessed bytes
    size_type size() const { return netbuf().length_unprocessed(); }


    size_type write(const void* d, int len)
    {
        if(len > size()) len = size();

        // FIX: ugly, netbuf itself really needs to drop const, at least when
        // it's a writeable netbuf
        memcpy(netbuf().unprocessed(), d, len);
        bool advance_success = advance(len);

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

};

}}}
