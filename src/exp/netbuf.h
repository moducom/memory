#pragma once

#include <cstdlib>

#include "../mc/memory-chunk.h"

namespace moducom { namespace io { namespace experimental {


template <class TMemoryChunk, typename TSize = size_t>
class NetBufMemoryTemplate : public moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk, TSize>
{
public:
    typedef TSize size_t;
    typedef TMemoryChunk chunk_t;
    typedef moducom::pipeline::ProcessedMemoryChunkBase<TMemoryChunk, TSize> base_t;

public:
    NetBufMemoryTemplate() {}

    // Move forward to next net buf once we've exhausted this one
    // (noop for netbuf memory writer, if you're out of space, you're
    // screwed)
    bool next() { return false; }
};

template <class TMemoryChunk>
class NetBufMemoryWriterTemplate : public NetBufMemoryTemplate<TMemoryChunk>
{
    typedef NetBufMemoryTemplate<TMemoryChunk> base_t;

public:
};

template <class TMemoryChunk>
class NetBufMemoryReaderTemplate : public NetBufMemoryTemplate<TMemoryChunk>
{
    typedef NetBufMemoryTemplate<TMemoryChunk> base_t;

public:
};


namespace layer2 {


template <size_t buffer_length>
class NetBufMemoryWriter :
        public NetBufMemoryWriterTemplate<moducom::pipeline::layer1::MemoryChunk<buffer_length > >
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
    public NetBufMemoryWriterTemplate<pipeline::MemoryChunk>
{
    size_t pos;
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

}}}
