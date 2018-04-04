#include "catch.hpp"

#include "platform.h"
#include "mc/memory-chunk.h"

using namespace moducom::pipeline;

TEST_CASE("Memory chunk tests", "[memory-chunk]")
{
    SECTION("ProcessedMemoryChunk")
    {
        layer1::MemoryChunk<128> chunk;
        ProcessedMemoryChunk<MemoryChunk> pmc(chunk);

        pmc.advance(10);

        REQUIRE(chunk.data(10) == pmc.data());
    }
}
