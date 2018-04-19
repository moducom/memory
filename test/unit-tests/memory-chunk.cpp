#include "catch.hpp"

#include "platform.h"
#include "mc/memory-chunk.h"

using namespace moducom::pipeline;

TEST_CASE("Memory chunk tests", "[memory-chunk]")
{
    SECTION("ProcessedMemoryChunkBase layer 1")
    {
        layer1::MemoryChunk<128> chunk;
        ProcessedMemoryChunkBase<MemoryChunk> pmc(chunk);

        pmc.advance(10);

        REQUIRE(chunk.data(10) == pmc.unprocessed());
    }
    SECTION("ProcessedMemoryChunkBase layer 3")
    {
        layer3::MemoryChunk<128> chunk;
        ProcessedMemoryChunkBase<MemoryChunk&> pmc(chunk);

        pmc.advance(10);

        // Following doesn't compile yet due to unexpected setter
        // in MemoryChunk
        REQUIRE(chunk.data(10) == pmc.unprocessed());
    }
}
