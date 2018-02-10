// Often we want a Red-Black BST that is easily 'augmentable'. I think the best
// way to do this is to simply write a usual red-black tree and modify it by
// hand. Trying to create some kind of 'plugin' interface will make it
// unnecessarily complex. So here goes. The nodes will fully expose the key
// along with the mapped value. It is the user's responsibilty to not tamper
// with the key. Allowing only POD-ish types, as this makes implementing and
// editing the structure even easier.

#include <scaffold/memory.h>
#include <scaffold/temp_allocator.h>
#include <functional>
#include <type_traits>

namespace fo {

// Fwd
template <typename Key, typename T> struct RBNode;

namespace rbt {

enum Color : unsigned { BLACK = 0, RED = 1 };

enum Arrow : unsigned { LEFT = 0, RIGHT = 1};

namespace internal {

template <typename T> struct TypeIsSupported {
    static constexpr bool value = std::is_trivially_copy_assignable<T>::value &&
                                  std::is_trivially_copyable<T>::value &&
                                  std::is_trivially_destructible<T>::value;
};

// We keep the kid pointers here
template <typename Key, typename T> struct RBNodeKids {
    RBNode<Key, T> *_kids[2] = {}; // You don't usually need to use this, except if you are walking the rbt
    RBNode<Key, T> *_parent = nullptr;
    Color _color = BLACK;
};

} // ns rbt::internal
} // ns rbt

template <typename Key, typename T> struct RBNode : public internal::RBNodeKids<Key, T> {
    Key k;
    T v;

    RBNode() = default;

    RBNode(const Key &k, const T &v)
        : internal::RBNodeKids<Key, T, RBNode>()
        , k(k)
        , v(v) {}
};

template <typename Key, typename T> struct RBTree {
    static_assert(rbt::internal::TypeIsSupported<Key>::value && rbt::internal::TypeIsSupported<T>::value,
                  "Only support trivial-ish types");

    using key_type = Key;
    using mapped_type = T;
    using value_type = RBNode<Key, T>;

    Allocator *_allocator;
    RBNodeKids<Key, T> *_nil;
    RBNode<Key, T> *_root;

    RBTree(Allocator &allocator);
    ~RBTree();

    /// If used like a copy constructor i.e you don't provide an allocator,
    /// the new tree will use the allocator used by `o`
    RBTree(const RBTree &o, Allocator *allocator = nullptr);

    RBTree &operator=(const RBTree &o);

    /// The rbt just moved-from should not be used for any further operations
    /// except destruction.
    RBTree(RBTree &&o);
    RBTree &operator=(RBTree &&o); // Same as above
};

// Default Implementation

namespace rbt {

namespace internal {

template <typename Key, typename T> void delete_all_nodes(RBTree<K, T> &tree) {
    if (_allocator == nullptr) {
        return;
    }

    if (tree._root == _nil) {
        MAKE_DELETE(_allocator, RBNode<Key, T>, _nil);
        tree._root = nullptr;
        tree._nil = nullptr;
        return;
    }

    TempAllocator128 ta(memory_globals::default_allocator());
    Array<RBNode<Key, T> *> stack(ta);
    array::reserve(stack, 512 / sizeof(RBNode<K, V> *));

    array::push_back(stack, tree._root);

    while (!array::size(stack) != 0) {
        RBNode<Key, T> *p = array::back(stack);
        array::pop_back(stack);
        for (uint32_t i = 0; i < 2; ++i) {
            if (!rbt::is_nil_node(tree, p->_kids[i])) {
                array::push_back(stack, p->_kids[i]);
            }
        }
        MAKE_DELETE(tree._allocator, RBNode<Key, T>, p);
    }

    MAKE_DELETE(tree._allocator, RBNodeKids<Key, T>, tree._nil);
    tree._root = nullptr;
    tree._nil = nullptr;
}

template <typename Key, typename T> void copy_tree(RBTree<K, T> &tree, const RBTree<K, T> &other) {
    // Make the nil node
    tree._nil = MAKE_NEW(tree._allocator, RBNode<Key, T>);

    // Make the root node
    if (!rbt::is_nil_node(other._root)) {
        tree._root = MAKE_NEW(tree._allocator, RBNode<Key, T>, other._root.k, other._root.v);
    } else {
        tree._root = tree._nil;
        return;
    }

    TempAllocator512 ta_1(memory_globals::default_allocator());
    TempAllocator512 ta_2(memory_globals::default_allocator());
    Array<const RBNode<Key, T> *> others_stack(ta_1);
    Array<RBNode<Key, T> *> wips_stack(ta_2);

    array::reserve(others_stack, 512 / sizeof(const RBNode<Key, T> *));
    array::push_back(others_stack, other._root);

    array::reserve(wips_stack, 512 / sizeof(RBNode<Key, T> *));
    array::push_back(wips_stack, tree._root);

    while (array::size(others_stack) != 0) {
        RBNode<Key, T> *wips_top = array::back(wips_stack);
        const RBNode<Key, T> *others_top = array::back(others_stack);

        array::pop_back(wips_stack);
        array::pop_back(others_stack);

        wips_top->_color = others_top->_color;

        for (uint32_t i = 0; i < 2; ++i) {
            if (rbt::is_nil_node(others_top->_kids[i])) {
                wips_top->_kids[i] = static_cast<RBNode<Key, T> *>(tree._nil);
            } else {
                wips_top->_kids[i] = MAKE_NEW(tree._allocator, RBNode<Key, T>, others_top->k, others_top->v);

                array::push_back(others_stack, others_top->_kids[i]);
                array::push_back(wips_stack, wips_top->_kids[i]);
            }
        }
    }
}

} // namespace fo::rbt::internal

} // namespace fo::rbt

namespace fo {

template <typename Key, typename T>
RBTree<Key, T>::RBTree(Allocator &allocator)
    : _allocator(&allocator) {
    _nil = MAKE_NEW(_allocator, RBNodeKids<Key, T>);
    _root = nullptr;
}

template <typename Key, typename T> RBTree<Key, T>::RBTree(const RBTree &other, Allocator *allocator) {
    _allocator = allocator ? allocator : o._allocator;

    rbt::internal::copy_tree(*this, other);
}

template <typename Key, typename T> RBTree<Key, T>::RBTree(RBTree &&other) {
    if (this == &other) {
        return;
    }

    _allocator = other._allocator;
    _root = other._root;
    _nil = other._nil;

    other._allocator = nullptr;
}

template <typename Key, typename T> RBTree<Key, T> &RBTree<Key, T>::operator=(const RBTree &other) {
    if (this == other) {
        return;
    }

    rbt::internal::delete_all_nodes(*this);
    rbt::internal::copy_tree(*this, other);

    return *this;
}

template <typename Key, typename T> RBTree<Key, T> &RBTree<Key, T>::operator=(RBTree &&other) {
    if (this == other) {
        return;
    }

    rbt::internal::delete_all_nodes(*this);

    _allocator = other._allocator;
    _root = other._root;
    _nil = other._nil;

    other._allocator = nullptr;

    return *this;
}

template <typename Key, typename T> RBTree<Key, T>::~RBTree() {
    rbt::internal::delete_all_nodes(*this);
    tree._allocator = nullptr;
}

namespace rbt {

namespace internal {

template <template class TreeOrNode, typename Key, typename T, bool const_pointer_to_node>
using Pointer =
    typename std::conditional<return_const_pointer, const RBNode<Key, T> *, TreeOrNode<Key, T> *>::type;

template <typename Key, typename T, bool return_const_pointer>
Pointer<RBNode, Key, T, return_const_pointer> find(Pointer<RBTree, Key, T, return_const_pointer> rbt,
                                                   Key &&k) {
    auto cur_node = rbt->_root;
    while ((void *)cur_node != (void *)rbt->_nil) {
        if (cur_node->k == k) {
            return cur_node;
        }
        if (cur_node->k < k) {
            cur_node = cur_node->_kids[1];
        } else {
            cur_node = cur_node->_kids[0];
        }
    }

    return cur_node;
}

} // namespace rbt::internal

template <typename Key, typename T> bool is_nil_node(const RBTree<Ket, T> &rbt, const RBNode<Key, T> &node) {
    return (const void *)rbt._nil == (const void *)&node;
}

template <typename Key, typename T, bool is_const_pointer> struct Result {
    bool key_was_present = false;
    Pointer<Key, T, is_const_pointer> node = nullptr;
};

template <typename Key, typename T> Result<Key, T, false> get(RBTree<Key, T> &rbt, Key &&k) {
    auto node = internal::find<Key, T, false>(rbt, k);
    if (is_nil_node(rbt, *node)) {
        return Result<Key, T>();
    }
    return Result<Key, T, false>{true, node};
}

template <typename Key, typename T> Result<Key, T, true> get_const(const RBTree<Key, T> &rbt, Key &&k) {
    auto node = internal::find<Key, T, true>(rbt, k);
    if (is_nil_node(rbt, *node)) {
        return Result<Key, T, true>();
    }
    return Result<Key, T, true>{true, node};
}

template <typename Key, typename T> Result<Key, T, true> get(const RBTree<Key, T> &rbt, Key &&k) {
    return get_const(rbt, k);
}

template <typename Key, typename T> Result<Key, T, false> set(RBTree<Key, T> &rbt, Key &&k, Value &&v) {
    if (is_nil_pointer(rbt, rbt._root)) {
        rbt._root = MAKE_NEW(rbt._allocator, RBNode<Key, T>, std::forward(k), std::forward(v));
        rbt._root->_kids[0] = static_cast<RBNode<Key, T> *>(rbt->_nil);
        rbt._root->_kids[1] = static_cast<RBNode<Key, T> *>(rbt->_nil);
        return Result<Key, T, false>{false, rbt._root};
    }

    auto cur = _root;
    auto par = _root;
    Arrow dir = LEFT;
    while (!is_nil_node(cur)) {
        assert(cur != nullptr);
        par = cur;
        if (k < cur->k) {
            cur = cur->_kids[LEFT];
            dir = LEFT;
        } else if (k > cur->k) {
            cur = cur->_kids[RIGHT];
            dir = RIGHT;
        } else {
            cur->k = std::forward(k);
            cur->v = std::foward(v);
            return;
        }
    }

    auto n = MAKE_NEW(rbt._allocator, RBNode<Key, T>, std::forward(k), std::forward(v));

    n->_color = RED;
    n->_parent = par;
    n->_kids[LEFT] = static_cast<RBNode<Key, T> *>(rbt._nil);
    n->_kids[RIGHT] = static_cast<RBNode<Key, T> *>(rbt._nil);
    par->_kids[dir] = n;

    // bottom-up fix
    while (n->_parent->_color == RED) {
        if (n->_parent == n->_parent->_parent->_kids[LEFT]) {
            n = _insert_fix<LEFT, RIGHT>(n);
        } else {
            n = _insert_fix<RIGHT, LEFT>(n);
        }
    }
    _root->_color = BLACK;
}

// Removes the node with the given key. The `node` field of the returned struct is always nullptr.
template <typename Key, typename T> Result<Key, T> remove(RBTree<Key, T> &rbt, Key &&k);

} // namespace fo::rbt

} // namespace fo
