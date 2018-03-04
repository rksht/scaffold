#pragma once

// Want to support all types, not just POD types.

#include <functional>
#include <scaffold/memory.h>
#include <scaffold/temp_allocator.h>
#include <type_traits>

namespace fo {

// Fwd
template <typename Key, typename T> struct RBNode;

namespace rbt {

enum Color : unsigned { BLACK = 0, RED = 1 };

enum Arrow : unsigned { LEFT = 0, RIGHT = 1 };

namespace internal {

// We keep the kid pointers here
template <typename Key, typename T> struct ChildPointers {
    RBNode<Key, T> *_childs[2] = {}; // You don't usually need to use this, except if you are walking the rbt
    RBNode<Key, T> *_parent = nullptr;
    Color _color = BLACK;
};

} // namespace internal

template <typename Key, typename T> struct RBNode : public internal::ChildPointers<Key, T> {
    Key k;
    T v;

    RBNode() = default;

    RBNode(const Key &k, const T &v)
        : internal::ChildPointers<Key, T>()
        , k(k)
        , v(v) {}

    RBNode(Key &&k, T &&v)
        : internal::ChildPointers<Key, T>()
        , k(std::move(k))
        , v(std::move(v)) {}

    RBNode(const RBNode<Key, T> &) = delete;
    RBNode(RBNode<Key, T> &&) = delete;
};

template <typename Key, typename T> struct RBTree {
    using key_type = Key;
    using mapped_type = T;
    using value_type = RBNode<Key, T>;

    Allocator *_allocator;
    internal::ChildPointers<Key, T> *_nil;
    RBNode<Key, T> *_root;

    RBTree(Allocator &allocator);
    ~RBTree();

    /// If used like a copy constructor i.e you don't provide an allocator, the new tree will use the
    /// allocator used by `o`
    RBTree(const RBTree &o, Allocator *allocator = nullptr);

    RBTree &operator=(const RBTree &o);

    /// The rbt just moved-from should not be used for any further operations except destruction.
    RBTree(RBTree &&o);
    RBTree &operator=(RBTree &&o); // Same as above
};

} // namespace rbt

// Default Implementation

namespace rbt {

template <template <class, class> typename TreeOrNode, typename Key, typename T, bool const_pointer_to_node>
using OnKeyAndT =
    typename std::conditional<const_pointer_to_node, const TreeOrNode<Key, T>, TreeOrNode<Key, T>>::type;

namespace internal {

template <typename Key, typename T> void delete_all_nodes(RBTree<Key, T> &tree) {
    if (_allocator == nullptr) {
        return;
    }

    if (static_cast<ChildPointers<Key, T> *>(tree._root) == _nil) {
        MAKE_DELETE(_allocator, ChildPointers<Key, T>, _nil);
        tree._root = nullptr;
        tree._nil = nullptr;
        return;
    }

    TempAllocator1024 ta(memory_globals::default_allocator());
    Array<RBNode<Key, T> *> stack(ta);
    array::reserve(stack, 512 / sizeof(RBNode<K, V> *));

    array::push_back(stack, tree._root);

    while (!array::size(stack) != 0) {
        RBNode<Key, T> *p = array::back(stack);
        array::pop_back(stack);
        for (uint32_t i = 0; i < 2; ++i) {
            if (!rbt::is_nil_node(tree, p->_childs[i])) {
                array::push_back(stack, p->_childs[i]);
            }
        }
        MAKE_DELETE(tree._allocator, RBNode<Key, T>, p);
    }

    MAKE_DELETE(tree._allocator, ChildPointers<Key, T>, tree._nil);
    tree._root = nullptr;
    tree._nil = nullptr;
}

template <typename Key, typename T> void copy_tree(RBTree<Key, T> &tree, const RBTree<Key, T> &other) {
    // Make the nil node
    tree._nil = MAKE_NEW(tree._allocator, RBNode<Key, T>);

    // Make the root node
    if (!rbt::is_nil_node(other, other._root)) {
        tree._root = MAKE_NEW(tree._allocator, RBNode<Key, T>, other._root.k, other._root.v);
    } else {
        tree._root = tree._nil;
        return;
    }

    std::vector<const RBNode<Key, T> *> others_stack;
    std::vector<RBNode<Key, T> *> wips_stack;

    others_stack.push_back(other._root);
    wips_stack.push_back(tree._root);

    while (others_stack.size() != 0) {
        auto wips_top = wips_back.back();
        auto others_top = others_stack.back();

        wips_stack.pop_back();
        others_stack.pop_back();

        wips_top->_color = others_top->_color;

        for (uint32_t i = 0; i < 2; ++i) {
            if (rbt::is_nil_node(others_top->_childs[i])) {
                wips_top->_childs[i] = static_cast<RBNode<Key, T> *>(tree._nil);
            } else {
                wips_top->_childs[i] =
                    MAKE_NEW(tree._allocator, RBNode<Key, T>, others_top->k, others_top->v);

                others_stack.push_back(others_top->_childs[i]);
                wips_stack.push_back(wips_top->_childs[i]);
            }
        }
    }
}

template <typename Key, typename T, bool return_const_pointer>
OnKeyAndT<RBNode, Key, T, return_const_pointer> *find(OnKeyAndT<RBTree, Key, T, return_const_pointer> *rbt,
                                                      const Key &k) {
    auto cur_node = rbt->_root;
    while (!is_nil_node(*rbt, cur_node)) {
        if (cur_node->k == k) {
            return cur_node;
        }
        if (cur_node->k < k) {
            cur_node = cur_node->_childs[1];
        } else {
            cur_node = cur_node->_childs[0];
        }
    }

    return cur_node;
}

template <typename Key, typename T> RBNode<Key, T> *nil_node(RBTree<Key, T> &t) {
    return static_cast<RBNode<Key, T> *>(t._nil);
}

template <typename Key, typename T, int left, int right> void _rotate(RBTree<Key, T> &t, RBNode<Key, T> *x) {
    auto y = x->_childs[right];
    x->_childs[right] = y->_childs[left];
    if (!is_nil_node(t, y->_childs[left])) {
        y->_childs[left]->_parent = x;
    }
    y->_parent = x->_parent;
    if (is_nil_node(t, x->_parent)) {
        _root = y;
    } else if (x == x->_parent->_childs[left]) {
        x->_parent->_childs[left] = y;
    } else {
        x->_parent->_childs[right] = y;
    }
    y->_childs[left] = x;
    x->_parent = y;
}

template <typename Key, typename T>
void _transplant(RBTree<Key, T> &t, RBNode<Key, T> *n1, RBNode<Key, T> *n2) {
    if (n1 == t._root) {
        _root = n2;
    } else if (n1->_parent->_childs[0] == n1) {
        n1->_parent->_childs[0] = n2;
    } else {
        n1->_parent->_childs[1] = n2;
    }
    n2->_parent = n1->_parent;
}

template <typename Key, typename T, int left, int right>
RBNode<Key, T> *_insert_fix(RBTree<Key, T> &t, RBNode<Key, T> *z) {
    auto y = z->_parent->_parent->_childs[right];
    if (y->_color == RED) {
        z->_parent->_color = BLACK;
        y->_color = BLACK;
        z->_parent->_parent->_color = RED;
        z = z->_parent->_parent;
    } else {
        if (z == z->_parent->_childs[right]) {
            z = z->_parent;
            _rotate<left, right>(t, z);
        }
        z->_parent->_color = BLACK;
        z->_parent->_parent->_color = RED;
        _rotate<right, left>(t, z->_parent->_parent);
    }
    return z;
}

template <typename Key, typename T, int left, int right>
void _remove_fix(RBTree<Key, T> &t, RBNode<Key, T> *n) {
    auto w = x->_parent->_childs[right];
    if (w->_color == RED) {
        w->_color = BLACK;
        x->_parent->_color = RED;
        _rotate<left, right>(t, x->_parent);
        w = x->_parent->_childs[right];
    }
    if (w->_childs[left]->_color == BLACK && w->_childs[right]->_color == BLACK) {
        w->_color = RED;
        x = x->_parent;
    } else {
        if (w->_childs[right]->_color == BLACK) {
            w->_childs[left]->_color = BLACK;
            w->_color = RED;
            _rotate<right, left>(t, w);
            w = x->_parent->_childs[right];
        }
        w->_color = x->_parent->_color;
        x->_parent->_color = BLACK;
        w->_childs[right]->_color = BLACK;
        _rotate<left, right>(t, x->_parent);
        x = t._root;
    }
    return x;
}

} // namespace internal

} // namespace rbt

namespace rbt {

template <typename Key, typename T> RBTree<Key, T>::RBTree(Allocator &allocator) {
    _allocator = &allocator;
    _nil = MAKE_NEW(*_allocator, internal::ChildPointers<Key, T>);
    _root = reinterpret_cast<RBNode<Key, T> *>(_nil);
}

template <typename Key, typename T> RBTree<Key, T>::RBTree(const RBTree &other, Allocator *allocator) {
    _allocator = allocator ? allocator : other._allocator;
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
    if (tree._allocator) {
        rbt::internal::delete_all_nodes(*this);
        tree._allocator = nullptr;
    }
}

template <typename Key, typename T> bool is_nil_node(const RBTree<Key, T> &rbt, const RBNode<Key, T> *node) {
    return static_cast<const internal::ChildPointers<Key, T> *>(node) == rbt._nil;
}

template <typename Key, typename T, bool is_const_pointer> struct Result {
    bool key_was_present = false;
    OnKeyAndT<RBNode, Key, T, is_const_pointer> *node = nullptr;
};

template <typename Key, typename T> Result<Key, T, false> get(RBTree<Key, T> &rbt, const Key &k) {
    auto node = internal::find<Key, T, false>(rbt, k);
    if (is_nil_node(rbt, node)) {
        return Result<Key, T>();
    }
    return Result<Key, T, false>{true, node};
}

template <typename Key, typename T> Result<Key, T, true> get_const(const RBTree<Key, T> &rbt, const Key &k) {
    auto node = internal::find<Key, T, true>(rbt, std::move(k));
    if (is_nil_node(rbt, node)) {
        return Result<Key, T, true>();
    }
    return Result<Key, T, true>{true, node};
}

template <typename Key, typename T> Result<Key, T, true> get(const RBTree<Key, T> &rbt, Key k) {
    return get_const(rbt, k);
}

template <typename Key, typename T> Result<Key, T, false> set(RBTree<Key, T> &rbt, Key k, T v) {
    if (is_nil_pointer(rbt, rbt._root)) {
        rbt._root = MAKE_NEW(rbt._allocator, RBNode<Key, T>, std::forward(k), std::forward(v));
        rbt._root->_childs[0] = static_cast<RBNode<Key, T> *>(rbt->_nil);
        rbt._root->_childs[1] = static_cast<RBNode<Key, T> *>(rbt->_nil);
        return Result<Key, T, false>{false, rbt._root};
    }

    Result<Key, T, false> result;

    auto cur = _root;
    auto par = _root;
    Arrow dir = LEFT;
    while (!is_nil_node(rbt, cur)) {
        assert(cur != nullptr);
        par = cur;
        if (k < cur->k) {
            cur = cur->_childs[LEFT];
            dir = LEFT;
        } else if (k > cur->k) {
            cur = cur->_childs[RIGHT];
            dir = RIGHT;
        } else {
            cur->k = std::move(k);
            cur->v = std::move(v);
            result.key_was_present = true;
            result.node = cur;
            return result;
        }
    }

    auto n = MAKE_NEW(rbt._allocator, RBNode<Key, T>, std::forward(k), std::forward(v));

    result.key_was_present = false;
    result.node = n;

    n->_color = RED;
    n->_parent = par;
    n->_childs[LEFT] = static_cast<RBNode<Key, T> *>(rbt._nil);
    n->_childs[RIGHT] = static_cast<RBNode<Key, T> *>(rbt._nil);
    par->_childs[dir] = n;

    // bottom-up fix
    while (n->_parent->_color == RED) {
        if (n->_parent == n->_parent->_parent->_childs[LEFT]) {
            n = _insert_fix<LEFT, RIGHT>(n);
        } else {
            n = _insert_fix<RIGHT, LEFT>(n);
        }
    }
    _root->_color = BLACK;
    return result;
}

// Removes the node with the given key.
template <typename Key, typename T> Result<Key, T, false> remove(RBTree<Key, T> &t, Key k) {
    auto n = internal::find(&rbt, std::move(k));

    if (n == static_cast<RBNode<Key, T> *>(t._nil)) {
        return Result<Key, T, false>{};
    }

    auto y = n;
    RBColor orig_color = n->_color;
    RBNode<K, V> *x;
    if (is_nil_node(t, n->_childs[LEFT])) {
        x = n->_childs[RIGHT];
        _transplant(t, n, n->_childs[RIGHT]);
    } else if (is_nil_node(t, n->_childs[RIGHT])) {
        x = n->_childs[LEFT];
        _transplant(t, n, n->_childs[RIGHT]);
    } else {
        // minimum of n->_childs[RIGHT]
        auto min = n->_childs[RIGHT];
        while (!is_nil_node(t, min->_childs[LEFT])) {
            min = min->_childs[LEFT];
        }
        y = min;
        orig_color = y->_color;
        x = y->_childs[RIGHT];
        if (y->_parent == n) {
            x->_parent = y;
        } else {
            _transplant(t, y, y->_childs[RIGHT]);
            y->_childs[RIGHT] = n->_childs[RIGHT];
            y->_childs[RIGHT]->_parent = y;
        }
        _transplant(t, n, y);
        y->_childs[LEFT] = n->_childs[LEFT];
        y->_childs[LEFT]->_parent = y;
        y->_color = n->_color;
    }
    if (orig_color == RED) {
        return n;
    }
    // remove fix
    while (x != t._root && x->_color == BLACK) {
        if (x == x->_parent->_childs[LEFT]) {
            x = _remove_fix<LEFT, RIGHT>(x);
        } else {
            x = _remove_fix<RIGHT, LEFT>(x);
        }
    }
    x->_color = BLACK;

    // Delete the node
    MAKE_DELETE(t._allocator, RBNode<Key, T>, n);

    Result<Key, T, false> result{};
    result.key_was_present = true;
    return result;
}

} // namespace rbt

} // namespace fo
