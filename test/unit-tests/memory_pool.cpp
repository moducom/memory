
#include <catch.hpp>

#include "MemoryPool.h"
#include "mc/memory-pool.h"
#ifdef ENABLE_COAP
#include "../coap-token.h"
#endif
#include "mc/objstack.h"

using namespace moducom::dynamic;


// useful because we prefer to keep direct allocator value rather than
// reference around in node_traits so that stateless allocators use no
// instance space
template <class TAllocator>
struct AllocatorRefHelper
{
    TAllocator& a;

    AllocatorRefHelper(TAllocator& a) : a(a) {}

    typedef typename TAllocator::pointer pointer;
    typedef typename TAllocator::handle_type handle_type;
    typedef typename TAllocator::value_type value_type;
    typedef typename TAllocator::const_void_pointer const_void_pointer;

    typedef typename TAllocator::lock_counter lock_counter;

    handle_type allocate(size_t size) { return a.allocate(size); }

    void deallocate(handle_type p, size_t size)
    {
        a.deallocate(p, size);
    }

    pointer lock(handle_type h) { return a.lock(h); }

    void unlock(handle_type h) { a.unlock(h); }

    static CONSTEXPR handle_type invalid()
    {
        return TAllocator::invalid();
    }

#ifdef FEATURE_CPP_ALIASTEMPLATE
    template <class TValue>
    using typed_handle = estd::typed_handle<TValue, TAllocator>;
#endif
};


// in attempt to re-use inbuilt next pointer
// unlike regular node traits, TValue must be
// explicitly specified because LinkedListPool itself needs to know
// it to know slot size in pool

// so far shaping up to look very much like inlinevalue_node_traits
template <class TValue, size_t slot_count>
struct LinkedListPoolNodeTraits
{
    typedef moducom::mem::LinkedListPool<TValue, slot_count> allocator_t;
    typedef estd::experimental::forward_node_base node_type;
    typedef estd::experimental::forward_node_base node_type_base;
    typedef node_type* node_handle;
    typedef node_type* node_pointer;

    template <class TValue2>
    struct node_allocator_t
    {
        static CONSTEXPR bool can_emplace() { return true; }

        // TODO: assert TValue2 == TValue

        allocator_t a;

        node_allocator_t(allocator_t* a) {}

        typedef typename allocator_t::item_t node_type;
        typedef node_type* node_handle;
        typedef node_type* node_pointer;
        typedef const TValue2& nv_ref_t;

        node_pointer alloc(TValue& value)
        {
            node_pointer p = a.allocate(sizeof(node_type));

            p->value()(value);

            return p;
        }

#ifdef FEATURE_CPP_MOVESEMANTIC
        node_pointer alloc_move(TValue&& value)
        {
            node_pointer p = a.allocate(sizeof(node_type));

            new (&p->value()) TValue(std::forward<TValue>(value));

            return p;
        }
#endif

#ifdef FEATURE_CPP_VARIADIC
        template <class ...TArgs>
        node_pointer alloc_emplace(TArgs&&...args)
        {
            node_pointer p = a.allocate(sizeof(node_type));

            new (&p->value()) TValue(args...);

            return p;
        }
#endif

        node_pointer lock(node_type_base* h)
        {
            // FIX
            return reinterpret_cast<node_pointer>(h);
        }

        void unlock(node_type_base* h) {}
    };

    static CONSTEXPR node_handle null_node() { return NULLPTR; }

    static void set_next(node_type& set_on, node_handle next)
    {
        set_on.next(next);
    }


    static node_pointer next(node_type& n)
    {
        return n.next();
    }

    // NOTE: Something about this I think needs to vary to reuse the underlying
    // node next pointer.  Not 100% sure though
    template <class TValue2>
    static TValue2& value_exp(typename node_allocator_t<TValue2>::node_type& node)
    {
        return node.value();
    }
};

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
        typedef typename node_traits_t::node_allocator_t<value_type> node_allocator_t;
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
    SECTION("LinkedListPool as allocator for other linked list")
    {
        typedef moducom::mem::LinkedListPool<int, 10> llpool_t;
        typedef AllocatorRefHelper<llpool_t> allocator_t;
        typedef estd::inlinevalue_node_traits<
                estd::experimental::forward_node_base,
                allocator_t> node_traits_t;

        llpool_t pool;
        allocator_t allocator(pool);

        // FIX: allocator_t initialization still clumsy, because we like
        // having 'empty' allocators we can pass around.  We are
        // interrupting purely inline value allocators from comfortably
        // existing
        estd::forward_list<int, node_traits_t> list(&allocator);

        REQUIRE(list.empty());

        list.push_front(5);

        REQUIRE(!list.empty());

        auto i = list.begin();

        REQUIRE(*i == 5);

        // nearly there, this has a const qualifier drop somewhere
        //REQUIRE(list.front() == 5);
    }
    SECTION("Using LinkedListPoolNodeTraits")
    {
        typedef moducom::mem::LinkedListPool<int, 10> llpool_t;
        typedef AllocatorRefHelper<llpool_t> allocator_t;
        typedef LinkedListPoolNodeTraits<int, 10> node_traits_t;
        //typedef typename node_traits_t::allocator_t allocator_t;

        llpool_t pool;
        allocator_t allocator(pool);

        estd::forward_list<int, node_traits_t> list;

        REQUIRE(list.empty());

        list.push_front(5);

        REQUIRE(!list.empty());

        auto i = list.begin();

        REQUIRE(*i == 5);

    }
}
