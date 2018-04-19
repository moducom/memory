//
// Created by malachi on 3/1/18.
//
#include <catch.hpp>

#include <cstring>
#include "platform.h"
#include "exp/netbuf.h"

using namespace moducom::io::experimental;

TEST_CASE("Experimental tests", "[experimental]")
{
    CONSTEXPR size_t buflen = 128;

    SECTION("A")
    {
        layer2::NetBufMemoryWriter<buflen> writer;

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
        typedef moducom::io::experimental::layer2::NetBufMemoryWriter<256> netbuf_t;
        netbuf_t netbuf;
        auto& chunk = netbuf.chunk();
        moducom::pipeline::MemoryChunk chunk2 = netbuf.chunk();

        REQUIRE(chunk.data() == netbuf.unprocessed());
        REQUIRE(chunk2.data() == netbuf.unprocessed());
    }
}
