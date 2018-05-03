//
// Created by malachi on 3/1/18.
//
#include <catch.hpp>

#include <cstring>
#include "platform.h"
#include "exp/netbuf.h"
#include "exp/llpool.h"

using namespace moducom::io::experimental;

TEST_CASE("Experimental tests", "[experimental]")
{
    CONSTEXPR size_t buflen = 128;

    SECTION("A")
    {
        layer2::NetBufMemory<buflen> writer;

        REQUIRE(writer.chunk().length() == buflen);
        REQUIRE(writer.length_unprocessed() == buflen);

        writer.advance(10);

        REQUIRE(writer.chunk().length() == buflen);
        REQUIRE(writer.length_unprocessed() == buflen - 10);

        /*
        layer2::NetBufHelper<layer2::NetBufMemoryWriter<128>> nbh(writer);

        nbh.advance(10);
        auto reduced_len = nbh.data().length();

        REQUIRE(reduced_len == 118); */
    }
    SECTION("netbuf")
    {
        SECTION("layer5")
        {
            moducom::pipeline::layer2::MemoryChunk<buflen> chunk;
            layer5::NetBufMemoryWriter nbmw(chunk);
            layer5::INetBuf& nb = nbmw;
            /*
            layer5::NetBufHelper nbh(nb);

            moducom::pipeline::MemoryChunk c = nbh.data();

            REQUIRE(c.length() == 128); */
        }
    }
    SECTION("netbuf chunk access")
    {
        typedef moducom::io::experimental::layer2::NetBufMemory<256> netbuf_t;
        netbuf_t netbuf;
        auto& chunk = netbuf.chunk();
        moducom::pipeline::MemoryChunk chunk2 = netbuf.chunk();

        REQUIRE(chunk.data() == netbuf.unprocessed());
        REQUIRE(chunk2.data() == netbuf.unprocessed());
    }
    SECTION("Latest memory pool incarnation")
    {
        moducom::mem::experimental::LinkedListPool2<int, 10> pool;
        int sz = sizeof(pool);

        REQUIRE(pool.count_free() == 10);

        int h = pool.allocate(1);

        REQUIRE(h == 0);
        REQUIRE(pool.count_free() == 9);

        pool.lock(h) = 5;

        int h1 = pool.allocate(1);

        REQUIRE(h1 == 1);
        REQUIRE(pool.count_free() == 8);
    }
    SECTION("Latest memory pool incarnation 2")
    {
        moducom::mem::experimental::LinkedListPool2<char[5], 10> pool;
        int sz = sizeof(pool);

        int h = pool.allocate(1);

        REQUIRE(h == 0);

        //pool.lock(h) = 5;

        int h1 = pool.allocate(1);

        REQUIRE(h1 == 1);

        pool.deallocate(h, 1);
    }
}
