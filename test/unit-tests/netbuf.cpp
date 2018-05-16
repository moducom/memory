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
    SECTION("Simplistic dynamic allocated NetBuf")
    {
        // TODO: Think I changed my mind again, 'pos' movement
        // through each netbuf chunk (PBUF like) should be managed
        // by writer, not netbuf itself
        NetBufWriter<NetBufDynamicMemory<> > writer;

        writer.write(std::string("Hello"));
        writer.write((const uint8_t*) ":123", 4);

        REQUIRE(writer.size() == 1024 - 9);
    }
    SECTION("Writing strings")
    {
        NetBufWriter<NetBufDynamicMemory<> > writer;
        estd::layer1::string<64> s = "Hello there!";

        writer << s;

        REQUIRE(writer.netbuf().length_processed() == s.size());
    }
}
