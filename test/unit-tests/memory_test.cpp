
#include <catch.hpp>

#include "MemoryPool.h"
//#include "mc/string.h"

// Not really using it, and isn't behaving on macOS
//#include <malloc.h>

using namespace moducom::dynamic;

struct Tester1
{
    char t[10];

    virtual ~Tester1()
    {
        t[0] = 3;
    }
};

struct Tester2 : public Tester1
{
    virtual ~Tester2()
    {
        t[1] = 4;
    }
};

TEST_CASE("General memory tests", "[memory]")
{
    SECTION("Free/not free checks")
    {
        // just dynamically allocating this one for fun
        IMemory* memory = new Memory();

        IMemory::handle_opaque_t handle = memory->allocate("Hi", 10, 3);

        char* value = (char*) memory->lock(handle);

        REQUIRE(strcmp(value, "Hi") == 0);

        memory->unlock(handle);

#ifdef __GNUC__
//        struct mallinfo mi = mallinfo();
#endif

        // FIX: no destructor is gonna get called cuz not yet a virtual
        delete memory;
    }
    SECTION("overloaded new 1")
    {
        // although not marked as such, overloaded new is currently experimental

        // just dynamically allocating this one for fun
        IMemory* memory = new Memory();
        IMemory::handle_opaque_t h;

        Tester1* t = new (*memory, &h) Tester2;

        // Doesn't seem to actually call memory free itself
        //delete (*memory, h, t);

        t->~Tester1();

        memory->free(h);

        delete memory;
    }
    SECTION("overloaded new 2")
    {
        // although not marked as such, overloaded new is currently experimental

        // just dynamically allocating this one for fun
        IMemory* memory = new Memory();
        MemoryTuple tuple(*memory);

        Tester1* t = new (tuple) Tester2;

        tuple.del(t);

        delete memory;
    }
}
