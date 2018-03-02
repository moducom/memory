#pragma once

#include <cstdlib>

#include "../mc/memory-chunk.h"

namespace moducom { namespace io { namespace experimental {


template <class TMemoryChunk>
class NetBufMemoryTemplate
{
protected:
    TMemoryChunk chunk;
    size_t pos;

public:
    NetBufMemoryTemplate() : pos(0) {}

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

};


namespace layer2 {


template <size_t buffer_length>
class NetBufMemoryWriter :
        public NetBufMemoryWriterTemplate<moducom::pipeline::layer1::MemoryChunk<buffer_length>>
{
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

}}}