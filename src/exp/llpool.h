#pragma once

#include <cstddef> // for size_t

#include <estd/forward_list.h>
#include <estd/array.h>

#include <iterator>
#include <algorithm>

namespace moducom { namespace mem { namespace experimental {

// mainly useful for 8-bit CPUs which don't have to necessarily
// pack Node OR general free-form memory buffer pools.
// The padding will depend on the contents of T
// good to note since if we did a byte[] cast to T it might throw off
// all the alginment, which would be pretty bad
template <class T, size_t N>
class LinkedListPool2
{
    typedef uint8_t _node_handle;

    static _node_handle CONSTEXPR eol() { return -1; }

    typedef estd::experimental::forward_node_base_base<_node_handle> node_base_t;

    // do this trickery so that node_base_t *follows* T value, as this
    // provides the best possible chance for favorable alignment/packing
    // http://www.catb.org/esr/structure-packing/
    struct _Node
    {
        T value;
    };

    class Node :
            public _Node,
            public node_base_t
    {
    public:
        // NOTE: This will be forcefully called on init, which is not ultimately desired
        // TODO: Make a protected default constructor for those situations which really
        // actually want one
        Node() : node_base_t(eol()) {}
    };

    // in theory we should be able to use inlineref_node_traits but it requires
    // an allocator itself, so kind of a chicken and the egg scenario.  A bit more
    // manual with intrusive_node_traits, but not too bad and importantly... it
    // works and is not kludgey
    //typedef estd::forward_list<T, estd::intrusive_node_traits<Node> > list_t;

    // spoke too soon above.  intrusive_node_traits very specifically uses pointers, because
    // without an allocator onhand it can't resolve handle types

    //list_t free_items;

    Node items[N];

    _node_handle m_front;

    Node& front()
    {
        return items[m_front];
    }

public:
    typedef _node_handle handle_type;
    typedef T value_type;

    bool is_full() const
    {
        return m_front == eol();
    }

    size_t count_free() const
    {
        if(is_full()) return 0;

        int counter = 1;

        const Node* i = &items[m_front];

        while(i->next() != eol())
        {
            counter++;
            i = &items[i->next()];
        }

        return counter;
    }

    LinkedListPool2() : m_front(0)
    {
        // initialize all free pointers
        for(int i = 0; i < N - 1; i++)
            items[i].next(i + 1);

        items[N - 1].next(eol());
    }

    handle_type allocate(size_t count)
    {
        // TODO: assert count == 1 always
        handle_type retval = m_front;

        // remove front free element from element chain
        m_front = front().next();
        //front().next(front().next());

        return retval;
    }


    value_type& lock(handle_type h)
    {
        return items[h].value;
    }


    void unlock(handle_type h)
    {

    }


    void deallocate(handle_type h, size_t count)
    {
        // TODO: assert count == 1 always

        handle_type current_front = m_front;

        // make incoming deallocating h the new head of the 'free' list
        m_front = h;

        front().next(current_front);

    }
};


template <class T>
struct LinkedListPool3Node : public estd::experimental::forward_node_base
{
    T value;
};


// 6/13/2018
// like the 10th crack at this
// This one works, and let's keep this simple.  For allocator compatibility, I want to contain
// this in a different class
template <class T, size_t N, class TContainer = estd::array<LinkedListPool3Node<T>, N> >
class LinkedListPool3
{
public:
    typedef typename TContainer::value_type node_t;

private:
    typedef TContainer array_t;
    typedef estd::intrustive_forward_list<node_t> list_t;

    list_t free_nodes;
    array_t raw;

public:
    LinkedListPool3()
    {
        typename array_t::iterator i = raw.begin();
        node_t* current = &(*i);

        free_nodes.push_front(*current);

        while(++i != raw.end())
        {
            node_t& next = *i;

            current->next(&next);
            current = &next;
        }
    }

    node_t* alloc()
    {
        node_t& front = free_nodes.front();
        free_nodes.pop_front();
        return &front;
    }


    // NOTE: behavior is undefined if incoming node_t is NOT
    // contained in raw
    void free(node_t* node)
    {
        free_nodes.push_front(*node);
    }

    size_t max_size() const { return N; }

    // returns number of unallocated slots
    size_t available() const
    {
        size_t count = 0;
        typename list_t::iterator i = free_nodes.begin();

        for(; i != free_nodes.end(); i++) count++;

        return count;
        //return std::count(free_nodes.begin(), free_nodes.end());

        //return std::distance(free_nodes.begin(), free_nodes.end());
    }
};

}}}
