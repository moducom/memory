
#include <catch.hpp>

#include "MemoryPool.h"
#include "mc/memory-pool.h"
#ifdef ENABLE_COAP
#include "../coap-token.h"
#endif
#include "mc/objstack.h"

using namespace moducom::dynamic;

#ifdef ENABLE_COAP
// Using TestToken because I am not convinced I want to embed "is_allocated" into layer2::Token
// somewhat harmless, but confusing out of context (what does is_allocated *mean* if token is
// never used with the pool code)
class TestToken : public moducom::coap::layer2::Token
{
public:
    TestToken() {}

    TestToken(const moducom::pipeline::MemoryChunk::readonly_t& chunk)
    {

        ASSERT_ERROR(true, chunk.length() <= 8, "chunk.length <= 8");
        copy_from(chunk);
    }

    ~TestToken() { length(0); }

    bool is_active() const { return length() > 0; }
};
#endif

TEST_CASE("Low-level memory pool tests", "[mempool-low]")
{
    SECTION("Index1 memory pool")
    {
        MemoryPool<> pool;

        int handle = pool.allocate(100);

        REQUIRE(pool.get_free() == (32 * (128 - 1) - 128));
    }
    SECTION("Index2 memory pool")
    {
        MemoryPool<> pool(IMemory::Indexed2);
        typedef MemoryPoolIndexed2HandlePage::handle_t handle_t;

        SECTION("Allocations")
        {
            int allocated_count = 1;
            int unallocated_count = 1;

            int handle = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == allocated_count);

            allocated_count++;

            int handle2 = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == allocated_count);
            REQUIRE(pool.get_allocated_handle_count(false) == unallocated_count);
            REQUIRE(pool.get_free() == 32 * (128 - 1) -
                                       allocated_count * (32 * 4) - // allocated

                                       // unallocated siphons a little memory off the top too, because a PageData
                                       // is being tracked in the page pool
                                       unallocated_count * sizeof(handle_t::PageData)
            );
        }
        SECTION("Locking")
        {
            REQUIRE(pool.get_allocated_handle_count() == 0);

            int handle = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == 1);

            pool.lock(handle);
            pool.unlock(handle);

            REQUIRE(pool.get_allocated_handle_count() == 1);

            pool.free(handle);

            REQUIRE(pool.get_allocated_handle_count() == 0);
        }
    }
#ifdef ENABLE_COAP
    SECTION("Traditional memory pool")
    {
        PoolBase<TestToken, 8> pool;
        TestToken hardcoded;

        hardcoded.set("1234", 4);

        REQUIRE(pool.count() == 0);

        TestToken* allocated = pool.allocate(hardcoded);

        REQUIRE(pool.count() == 1);

        pool.free(allocated);

        REQUIRE(pool.count() == 0);
    }
#endif
    SECTION("Object Stack")
    {
        moducom::pipeline::layer2::MemoryChunk<512> chunk;
        ObjStack os(chunk);

        int* val = new (os) int;

        void* buffer = os.alloc(10);

        REQUIRE(os.length() == 512 - sizeof(int) - 10
#ifdef FEATURE_MC_MEM_OBJSTACK_CHECK
                               - sizeof(ObjStack::Descriptor) * 2
#endif
                                );

#ifdef FEATURE_MC_MEM_OBJSTACK_GNUSTYLE
        os.free(buffer);
#else
        os.free(10);
#endif
        os.del(val);

        REQUIRE(os.length() == 512);
    }
    SECTION("LinkedListPool")
    {
        typedef moducom::mem::LinkedListPool<int, 4> llpool_t;
        typedef llpool_t::item_t item_t;
        llpool_t pool;

        int* item = pool.allocate();

        *item = 5;

        const auto& allocated = pool.allocated();
        auto iterator = allocated.begin();

        REQUIRE(iterator.lock().value() == 5);
        iterator++;
        REQUIRE(iterator == allocated.end());
        REQUIRE(!allocated.empty());

        pool.deallocate(item);

        REQUIRE(allocated.empty());
    }
    SECTION("Experimental pool slot management")
    {
        typedef int value_type;
        typedef moducom::mem::experimental::intrusive_node_pool_traits node_traits_t;
        typedef typename node_traits_t::test_node_allocator_t<value_type> node_allocator_t;
        typedef typename node_allocator_t::node_type node_type;
        typedef estd::forward_list<value_type, node_traits_t> list2_t;

        list2_t list;
        node_type value;

        value.value() = 5;

        list.push_front(value);

        // following doesn't quite work yet, compiler tells me I'm
        // trying to convert int& to intrusive_node_pool_node_type<int>,
        // but nearly as I can tell we should in fact be starting with intrusive_node_pool_node_type<int>*
        //REQUIRE(list.front() == 5);
    }
}
