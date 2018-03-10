#pragma once

#include <cstdlib>

#include "../mc/memory-chunk.h"

namespace moducom { namespace io { namespace experimental {


template <class TMemoryChunk, typename TSize = size_t>
class NetBufMemoryTemplate
{
public:
    typedef TSize size_t;
    typedef TMemoryChunk chunk_t;

protected:

    TMemoryChunk chunk;
    size_t pos;

public:
    NetBufMemoryTemplate() : pos(0) {}

    // managing pos within netbuf has advantage that we can pass netbuf around
    // easily and have this come along for the ride.  Disadvantage is that that
    // might be better suited by a helper struct which wraps netbuf, so that
    // netbuf itself is more lightweigt
    void advance(size_t advance_bytes) { pos += advance_bytes; }

    // Move forward to next net buf once we've exhausted this one
    // (noop for netbuf memory writer, if you're out of space, you're
    // screwed)
    bool next() { return false; }

    size_t length() { return chunk.length(); }
};

template <class TMemoryChunk>
class NetBufMemoryWriterTemplate : public NetBufMemoryTemplate<TMemoryChunk>
{
    typedef NetBufMemoryTemplate<TMemoryChunk> base_t;

public:
    // get data buffer currently available
    moducom::pipeline::MemoryChunk data() const
    {
        // FIX: make a layer1::ReadOnlyMemoryChunk to avoid this
        // nasty typecast
        return moducom::pipeline::MemoryChunk(
                (uint8_t*)base_t::chunk.data(base_t::pos), base_t::chunk.length() - base_t::pos);
    }
};

template <class TMemoryChunk>
class NetBufMemoryReaderTemplate : public NetBufMemoryTemplate<TMemoryChunk>
{
    typedef NetBufMemoryTemplate<TMemoryChunk> base_t;

public:
    // get data buffer currently available
    moducom::pipeline::MemoryChunk::readonly_t data() const
    {
        // FIX: make a layer1::ReadOnlyMemoryChunk to avoid this
        // nasty typecast
        return moducom::pipeline::MemoryChunk::readonly_t(
                (uint8_t*)base_t::chunk.data(base_t::pos), base_t::chunk.length() - base_t::pos);
    }
};


namespace layer2 {


template <size_t buffer_length>
class NetBufMemoryWriter :
        public NetBufMemoryWriterTemplate<moducom::pipeline::layer1::MemoryChunk<buffer_length>>
{
};


// FIX: crappy name and only employed if we ditch inbuilt advance
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
                chunk.data() + pos, nb.length() - pos);
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
        public NetBufMemoryWriterTemplate<moducom::pipeline::layer2::MemoryChunk<buffer_length>>
{
};

class NetBufMemoryReader :
    public NetBufMemoryReaderTemplate<pipeline::MemoryChunk>
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
};

}

}}}
