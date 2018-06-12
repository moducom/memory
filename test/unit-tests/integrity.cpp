#include <catch.hpp>

#include "mc/memory-chunk.h"

using namespace moducom::pipeline;

TEST_CASE("Basic memory integrity tests", "[integrity]")
{
    SECTION("A")
    {
        layer1::MemoryChunk<512> buffer;

        buffer[0] = 'a';
        buffer[1] = 'b';

        uint8_t* data = (uint8_t*)buffer.data();
        MemoryChunk _buffer = buffer;
        MemoryChunk __buffer(buffer);
        MemoryChunk ___buffer(data, 512);

        REQUIRE(data[0] == 'a');

        REQUIRE(__buffer.data() == _buffer.data());
        REQUIRE(buffer.data() == data);
        REQUIRE(_buffer.data() == data);

        REQUIRE(_buffer[0] == 'a');
        REQUIRE(___buffer[0] == 'a');

        REQUIRE(___buffer.data() == __buffer.data());

        REQUIRE(buffer.data() == _buffer.data());
    }
}
