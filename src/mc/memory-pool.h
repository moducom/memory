//
// Created by malachi on 2/25/18.
//

// this is a *traditional* memory pool, i.e. array of fixed size chunks vs memory_pool.h which is our
// advanced virtual memory engine

#pragma once

#include <stdlib.h>
#include "array-helper.h"
#include <estd/forward_list.h>

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

template <class T, size_t slots>
class LinkedListPool
{
public:
    typedef estd::experimental::forward_node<T> item_t;
    typedef estd::forward_list<item_t> list_t;

private:
    item_t items[slots];

    list_t m_allocated;
    list_t m_free;

public:

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

    item_t* allocate()
    {
        if(m_free.empty()) return NULLPTR;

        item_t& slot = m_free.front();
        m_allocated.push_front(slot);

        return &slot;
    }


    void deallocate(item_t* item)
    {
        //m_allocated.remove(*item);
        m_free.push_front(*item);
    }
};


}}
