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
    }
}