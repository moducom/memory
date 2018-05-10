#pragma once

#include "mc/mem/platform.h"
#include "mc/netbuf.h"

#ifdef FEATURE_MC_MEM_LWIP

#include <lwip/netbuf.h>

namespace moducom { namespace coap {

class LwipNetbuf
{
    netbuf* m_netbuf;
    bool m_end;
    typedef pipeline::MemoryChunk chunk_t;
    typedef size_t size_type;
    size_type pos;

    // true for incoming netbufs, out for outgoing
    // consider strongly templatizing instead, but that will cascade out and require
    // datapump to know about two TNetBufs
    bool is_incoming;

public:
    // TODO: eventually phase this out or make friend-protected
    LwipNetbuf() {}

    LwipNetbuf(netbuf* nb, bool is_incoming) :
        m_netbuf(nb),
        m_end(false),
        pos(0),
        is_incoming(is_incoming)
         {}

    const chunk_t chunk() const
    {
        uint8_t* data;
        uint16_t len;

        // TODO: look out for errors
        netbuf_data(m_netbuf, reinterpret_cast<void**>(&data), &len);

        return chunk_t(data, len);
    }

    // FIX: ugly, error prone
    const bool end() const
    {
        return m_end;
    }

    // this retrieves processed data
    // to access processed data, go through chunk()
    uint8_t* unprocessed() { return chunk().data(pos); }

    const uint8_t* processed() const { return chunk().data(); }

    size_type length_unprocessed() const { return chunk().length() - pos; }

    size_type length_processed() const
    {
        if(is_incoming)
            return chunk().length();
        else
            return pos;
    }


    void first()
    {
        netbuf_first(m_netbuf);
    }

    bool next()
    {
        int8_t result = netbuf_next(m_netbuf);

        switch(result)
        {
            case -1: return false;
            case 0: return true;
            case 1:
                m_end = true;
                return true;

            default:
                // TODO: produce some kind of runtime error
                return false;
        }
        return result != -1;
    }

    // TODO: Put in bounds check here
    // potentially most visible and distinctive feature of ProcessedMemoryChunk is this
    // call and its "side affect"
    bool advance(size_type size)
    {
        pos += size;
        return true;
    }
};

}}

#endif
