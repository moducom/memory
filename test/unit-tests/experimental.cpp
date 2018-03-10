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
    SECTION("A")
    {
        layer2::NetBufMemoryWriter<128> writer;

        auto chunk = writer.data();

        REQUIRE(chunk.length() == 128);
        REQUIRE(writer.length() == 128);

        layer2::NetBufHelper<layer2::NetBufMemoryWriter<128>> nbh(writer);

        nbh.advance(10);
        auto reduced_len = nbh.data().length();

        REQUIRE(reduced_len == 118);
    }
}
