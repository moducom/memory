#include <catch.hpp>

#include "platform.h"
#include "mc/memory-chunk.h"

using namespace moducom::pipeline;

TEST_CASE("Basic memory integrity tests", "[integrity]")
{
    SECTION("A")
    {
        layer1::MemoryChunk<512> buffer;
        MemoryChunk _buffer = buffer;
        MemoryChunk __buffer(buffer);

        REQUIRE(__buffer.data() == _buffer.data());
        REQUIRE(buffer.data() == _buffer.data());
    }
}
