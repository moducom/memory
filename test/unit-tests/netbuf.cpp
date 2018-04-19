#include <catch.hpp>

#include "exp/netbuf.h"

using namespace moducom::io::experimental;


TEST_CASE("NetBuf tests")
{
    SECTION("A")
    {
        NetBufWriter<layer2::NetBufMemory<256>> writer;

        writer.write(std::string("Hello"));
        writer.write((const uint8_t*) ":123", 4);

        REQUIRE(writer.size() == 256 - 9);
    }
}