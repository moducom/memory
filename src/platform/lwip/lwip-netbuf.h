#pragma once

#include "mc/mem/platform.h"
#include "mc/netbuf.h"

#ifdef FEATURE_MC_MEM_LWIP

#include <lwip/netbuf.h>

namespace moducom { namespace coap {

// here's an end-to-end netbuf usage example, however it seems pretty terrible:
// http://www.ecoscentric.com/ecospro/doc/html/ref/lwip-api-sequential-netconn-send.html
// problems include:
// 1) netconn_delete is called on a netbuf
// 2) netbuf_free is never called to get rid of the alloc
// 3) example doesn't make clear what happens with the 'data' data when issuing the
//    2nd netconn_send call
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
    //LwipNetbuf() {}
    // NOTE: Not sure I like specifying size here
    LwipNetbuf(uint16_t size = 1024) :
        m_end(false),
        pos(0),
        is_incoming(false) // FIX: assumes outgoing packet , since incoming are often allocated by tranport-aware code
        // but outgoing are often allocated by transport-unaware code
    {
        m_netbuf = netbuf_new();
        netbuf_alloc(m_netbuf, size);
    }

    LwipNetbuf(netbuf* nb, bool is_incoming) :
        m_netbuf(nb),
        m_end(false),
        pos(0),
        is_incoming(is_incoming)
         {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    LwipNetbuf(LwipNetbuf&& move_from) :
        m_netbuf(move_from.m_netbuf),
        m_end(move_from.m_end),
        pos(move_from.pos),
        is_incoming(move_from.is_incoming)
    {
        move_from.m_netbuf = NULLPTR;
    }
#endif


    ~LwipNetbuf()
    {
        // FIX: checking is_incoming flag only so that we deallocate
        // netbufs we ourselves allocated.  Seems very much not good
        if(m_netbuf != NULLPTR && is_incoming)
        {
            netbuf_free(m_netbuf);
            netbuf_delete(m_netbuf);
        }
    }

    netbuf* native() const { return m_netbuf; }

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
