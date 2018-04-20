//
// Created by malachi on 2/25/18.
//

// this is a *traditional* memory pool, i.e. array of fixed size chunks vs memory_pool.h which is our
// advanced virtual memory engine

#pragma once

#include <stdlib.h>
#include "array-helper.h"
#include <estd/forward_list.h>
#include "mem/platform.h"

namespace moducom { namespace dynamic {


// lean heavily on placement new, for smarter pool items
template <class T>
struct DefaultPoolItemTrait
{
    static bool is_allocated(const T& item)
    {
        return item.is_active();
    }


    static bool is_free(const T& item)
    {
        return !item.is_active();
    }

    template <class TArg1>
    static void allocate(T& item, TArg1 arg1)
    {
        new (&item) T(arg1);
    }


    static void free(T& item)
    {
        item.~T();
    }

    static void initialize(T& item)
    {
        item.~T();
    }
};


template <class T>
struct ExplicitPoolItemTrait
{
    static bool is_allocated(const T& item)
    {
        return item.is_active();
    }


    static bool is_free(const T& item)
    {
        return !item.is_active();
    }


    template <class TArg1>
    static void allocate(T& item, TArg1 arg1)
    {
        item.allocate(arg1);
    }


    // unallocate item, returning it to a free status
    static void free(T& item)
    {
        item.free();
    }

    // zero out item, initializing it to a free status
    static void initialize(T& item)
    {
        item.initialize();
    }
};


// FIX: Name obviously needs repair
template <class T, class TTraits = DefaultPoolItemTrait<T > >
class PoolBaseBase
{
    typedef TTraits traits_t;

protected:
    template <class TArg1>
    inline static T* allocate(TArg1 arg1, T* items, size_t max_count)
    {
        for(int i = 0; i < max_count; i++)
        {
            T* candidate = &items[i];

            if(traits_t::is_free(*candidate))
            {
                new (candidate) T(arg1);
                return candidate;
            }
        }

        return NULLPTR;
    }

    inline static size_t count(const T* items, size_t max_count)
    {
        size_t c = 0;

        for(int i = 0; i < max_count; i++)
        {
            const T& item = items[i];

            if(TTraits::is_allocated(item))
                c++;
        }

        return c;
    }

    inline static void free(T* items, size_t max_count, T* item)
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(item == &candidate)
            {
                traits_t::free(candidate);
                return;
            }
        }
    }

};


// To replace non fixed-array pools, but still are traditional pools
// i.e. at RUNTIME are fixed count and in a fixed position, once
// the class is initialized
// TODO: Combine with FnFactory also
template <class T, class TTraits = DefaultPoolItemTrait<T > >
class OutOfBandPool
{
    typedef TTraits traits_t;
    const T* items;
    const int max_count;
    typedef PoolBaseBase<T, TTraits> base_t;

public:
    OutOfBandPool(const T* items, int max_count) :
        items(items), max_count(max_count) {}

    template <class TArg1>
    T* allocate(TArg1 arg1)
    {
        return base_t::allocate(arg1, items, max_count);
    }

    // returns number of allocated items
    size_t count() const
    {
        return base_t::count(items, max_count);
    }

    // returns number of free slots
    size_t free() const
    {
        return max_count - count();
    }
};


template <class T, size_t max_count, class TTraits = DefaultPoolItemTrait<T > >
class PoolBase : PoolBaseBase<T, TTraits>
{
    typedef TTraits traits_t;
    typedef experimental::ArrayHelperBase<T> array_helper_t;
    typedef PoolBaseBase<T, TTraits> base_t;

    // pool items themselves
    T items[max_count];

public:
    struct Iter
    {
        T& value;

        Iter(T& v) : value(v) {}

        operator T&() const { return value; }
    };

    // TODO: complete once I determine how useful this is in a pre-C++11 environment
    Iter begin()
    {
        Iter i(items[0]);

        return i;
    }


    Iter end() const
    {
        Iter i(items[max_count - 1]);

        return i;
    }

    PoolBase()
    {
        // Not needed because explicit rigid arrays do an auto constructor call on
        // their items
        //array_helper_t::construct(items, max_count);
        // however, many items are "dumb" and have no inherent knowledge of their own
        // validity, so we do need to signal somehow that they are unallocated
        for(int i = 0; i < max_count; i++)
            traits_t::initialize(items[i]);
    }

#ifdef __CPP11__
    template <class ... TArgs>
    T& allocate(TArgs...args1)
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(traits_t::is_free(candidate))
            {
                new (&candidate) T(args1...);
                return candidate;
            }
        }
    }
#else
    template <class TArg1>
    T* allocate(TArg1 arg1)
    {
        return base_t::allocate(arg1, items, max_count);
    }

    T* allocate()
    {
        for(int i = 0; i < max_count; i++)
        {
            T& candidate = items[i];

            if(traits_t::is_free(candidate))
            {
                new (&candidate) T();
                return &candidate;
            }
        }

        return NULLPTR;
    }
#endif


    void free(T* item)
    {
        base_t::free(items, max_count, item);
    }

    // returns number of allocated items
    size_t count() const
    {
        return base_t::count(items, max_count);
    }

    // returns number of free slots
    size_t free() const
    {
        return max_count - count();
    }

    typedef OutOfBandPool<T, traits_t> oobp_t;

    // TODO: Improve naming
    oobp_t out_of_band() const
    {
        oobp_t oobp(items, max_count);
        return oobp;
    }

    operator oobp_t() const { return out_of_band(); }
};

}

namespace mem {

namespace experimental {


class intrusive_node_pool_node
{
    typedef uint8_t node_handle;

    node_handle m_next;

public:
    void next(const node_handle& n) { m_next = n; }
    node_handle next() const { return m_next; }

};


template <class TValue>
class intrusive_node_pool_node_type : public intrusive_node_pool_node
{
    typedef TValue value_type;

    value_type m_value;

public:
    value_type& value() { return m_value; }

    const value_type& value() const { return m_value; }
};

// this is where we have a pool of T, so in this case T
// and node are a bit different, but still no allocation goes on
// (operates very similar to forward_node)
template <class T, size_t slots, typename TSize = size_t>
class intrusive_node_pool_allocator
{
public:
    typedef TSize size_type;
    typedef uint8_t handle_type;
    typedef T value_type;
    typedef intrusive_node_pool_node_type<value_type> node_type;

    typedef node_type& nv_ref_t;
    typedef node_type* node_pointer;

private:
    node_type pool[slots];

public:
    // FIX: chicken and egg issue, this call expects to find an unallocated
    // item in the pool, but the whole point of this allocator is to be locker
    // which translates slot# into pointers, so that an external party can actually
    // search through the linked list to find a slot.  Kinda complex, I know
    //template <typename TValue>
    handle_type alloc(node_type& value)
    {
        ptrdiff_t d = ((uint8_t*)&value) - ((uint8_t*) pool);

        return d / sizeof(value_type);
        //return &value;
    }

    void dealloc(node_pointer node) {}

    node_pointer lock(handle_type h) { return &pool[h]; }
    void unlock(handle_type h) {}

    intrusive_node_pool_allocator(void* = NULLPTR) {}
};


class intrusive_node_pool_traits
{
public:
    typedef uint8_t node_handle;

    typedef estd::nothing_allocator allocator_t;

    static CONSTEXPR node_handle null_node() { return 0xFF; }

    static void set_next(intrusive_node_pool_node& set_on, node_handle& next)
    {
        set_on.next(next);
    }

    template <class TValue>
    static TValue& value_exp(intrusive_node_pool_node_type<TValue>& node)
    {
        return node.value();
    }

#ifdef FEATURE_CPP_ALIASTEMPLATE
    template <class TValue>
    using node_allocator_t = intrusive_node_pool_allocator<TValue, 10>;
#endif
};

};

template <class T, size_t slots>
class LinkedListPool
{
public:
    // +++ experimental uint8 handle variety
    // TODO: Utilize a variant of forward_node which uses a uint8_t handle
    // since they all will be living in items slots
    //typedef experimental::non_allocating_node_lock_pool_resolver<T, slots> node_allocator_t;
    typedef experimental::intrusive_node_pool_traits node_traits_t;
    typedef typename node_traits_t::node_allocator_t<T> node_allocator_t;
    typedef typename node_allocator_t::node_type node_type;
    typedef estd::forward_list<node_type, node_traits_t> list2_t;
    // ---

    typedef estd::experimental::forward_node<T> item_t;
    typedef estd::forward_list<item_t> list_t;

private:
    item_t items[slots];

    node_allocator_t node_allocator;

    list_t m_allocated;
    list_t m_free;

    //list2_t m_allocated2;

    item_t& allocate_internal()
    {
        item_t& slot = m_free.front();
        m_free.pop_front();
        m_allocated.push_front(slot);
        return slot;
    }

    void deallocate_internal(item_t& item)
    {
        m_allocated.remove(item, true);
        m_free.push_front(item);
    }

public:
    const list_t& allocated() const { return m_allocated; }

    LinkedListPool()
    {
        // initialize all slots as free
        item_t* prev = NULLPTR;

        for(item_t& item : items)
        {
            if(prev == NULLPTR)
                m_free.push_front(item);
            else
                prev->next(&item);

            prev = &item;
        }
    }

    T* allocate()
    {
        if(m_free.empty()) return NULLPTR;

        return &allocate_internal().value();
    }


    void deallocate(T* v)
    {
        // FIX: Some real trickery here, doing pointer math to figure out
        // where value actually sits in item, then sliding v backward
        // so tht it becomes an item_t.  Obviously fragile if we deviate
        // from using forward_node
        item_t* item = NULLPTR;
        uint8_t* offset = (uint8_t*) &item->value();
        item = (item_t*)(((uint8_t*)v) - offset);

        // TODO: utilize this in a static accessor on forward_node itself
        //offsetof(item_t, m_value);

        deallocate_internal(*item);
    }

    // allocator_traits conformant methods
    typedef item_t* handle_type;
    typedef T value_type;
    typedef value_type* pointer;
    typedef const void* const_void_pointer;

    typedef estd::nothing_allocator::lock_counter lock_counter;

    static CONSTEXPR handle_type invalid() { return NULLPTR; }

    handle_type allocate(size_t size)
    {
        if(m_free.empty()) return invalid();

        ASSERT_WARN(false, size < sizeof(item_t), "LinkedListPool requested size smaller than pool item");
        ASSERT_ERROR(false, size > sizeof(item_t), "LinkedListPool requested size larger than pool item");

        return &allocate_internal();
    }

    void deallocate(handle_type p, size_t size)
    {
        ASSERT_WARN(false, size < sizeof(item_t), "LinkedListPool requested size smaller than pool item");
        ASSERT_ERROR(false, size > sizeof(item_t), "LinkedListPool requested size larger than pool item");

        deallocate(*p);
    }

    T* lock(handle_type p)
    {
        return &p->value();
    }

    void unlock(handle_type p) {}
};


}}
