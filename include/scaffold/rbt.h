#pragma once

// Want to support all types, not just POD types.

#include <scaffold/array.h>
#include <scaffold/memory.h>
#include <scaffold/temp_allocator.h>

#include <assert.h>
#include <functional>
#include <type_traits>
#include <vector>

namespace fo {

// Fwd
template <typename Key, typename T> struct RBNode;

namespace rbt {

enum Color : unsigned { BLACK = 0, RED = 1 };

enum Arrow : unsigned { LEFT = 0, RIGHT = 1 };

template <typename Key, typename T> struct RBNode; // Fwd decl

namespace internal {

// We keep the kid pointers here
template <typename Key, typename T> struct ChildPointers {
    RBNode<Key, T> *_childs[2] = {nullptr, nullptr};
    RBNode<Key, T> *_parent = nullptr;
    Color _color = BLACK;
};

} // namespace internal

/// Represents a node stored in the tree
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

/// Represents a red-black tree based map.
template <typename Key, typename T> struct RBTree {
    using key_type = Key;
    using mapped_type = T;
    using value_type = RBNode<Key, T>;

    Allocator *_allocator;
    internal::ChildPointers<Key, T> *_nil;
    RBNode<Key, T> *_root;

    RBTree(Allocator &allocator);
    ~RBTree();

    // If used like a copy constructor i.e you don't provide an allocator, the new tree will use the allocator
    // used by `o`
    RBTree(const RBTree &o, Allocator *allocator = nullptr);

    RBTree &operator=(const RBTree &o);

    // The rbt just moved-from should not be used for any further operations except destruction.
    RBTree(RBTree &&o);
    RBTree &operator=(RBTree &&o); // Same as above
};

template <template <class, class> typename TreeOrNode, typename Key, typename T, bool is_const>
using KT = typename std::conditional<is_const, const TreeOrNode<Key, T>, TreeOrNode<Key, T>>::type;

template <typename Key, typename T, bool is_const> struct Iterator {
    using TreeType = KT<RBTree, Key, T, is_const>;
    using NodeType = KT<RBNode, Key, T, is_const>;

    TreeType *_tree;
    NodeType *_node;

    Iterator() = delete;
    Iterator(TreeType *tree, NodeType *node)
        : _tree(tree)
        , _node(node) {}

    Iterator(const Iterator &) = default;
    Iterator(Iterator &&other) = default;

    // Both ++ and -- have a time complexity of O(log n) where n is number of currently in tree.

    Iterator &operator++();
    Iterator operator++(int);

    Iterator &operator--();
    Iterator operator--(int);

    NodeType *operator->() const { return _node; }
    NodeType &operator*() const { return *_node; }
};

} // namespace rbt

// Default Implementation

namespace rbt {

template <typename Key, typename T> bool is_nil_node(const RBTree<Key, T> &rbt, const RBNode<Key, T> *node) {
    return static_cast<const internal::ChildPointers<Key, T> *>(node) == rbt._nil;
}

namespace internal {

template <typename Key, typename T> void delete_all_nodes(RBTree<Key, T> &tree) {
    if (tree._allocator == nullptr) {
        return;
    }

    if (is_nil_node(tree, tree._root)) {
        make_delete(*tree._allocator, tree._nil);
        // ^ Important to call the delete on _nil, not _root since type
        tree._root = nullptr;
        tree._nil = nullptr;
        return;
    }

    TempAllocator1024 ta(memory_globals::default_allocator());
    Array<RBNode<Key, T> *> stack(ta);
    array::reserve(stack, 512 / sizeof(RBNode<Key, T> *));

    array::push_back(stack, tree._root);

    while (array::size(stack) != 0) {
        RBNode<Key, T> *p = array::back(stack);
        array::pop_back(stack);
        for (u32 i = 0; i < 2; ++i) {
            if (!is_nil_node(tree, p->_childs[i])) {
                array::push_back(stack, p->_childs[i]);
            }
        }
        make_delete(*tree._allocator, p);
    }

    make_delete(*tree._allocator, tree._nil);
    tree._root = nullptr;
    tree._nil = nullptr;
}

template <typename Key, typename T> void copy_tree(RBTree<Key, T> &tree, const RBTree<Key, T> &other) {
    // Make the nil node
    tree._nil = make_new<RBNode<Key, T>>(*tree._allocator);

    // Make the root node
    if (!is_nil_node(other, other._root)) {
        tree._root = make_new<RBNode<Key, T>>(*tree._allocator, other._root.k, other._root.v);
    } else {
        tree._root = tree._nil;
        return;
    }

    std::vector<const RBNode<Key, T> *> others_stack;
    std::vector<RBNode<Key, T> *> wips_stack;

    others_stack.push_back(other._root);
    wips_stack.push_back(tree._root);

    while (others_stack.size() != 0) {
        auto wips_top = wips_stack.back();
        auto others_top = others_stack.back();

        wips_stack.pop_back();
        others_stack.pop_back();

        wips_top->_color = others_top->_color;

        for (u32 i = 0; i < 2; ++i) {
            if (rbt::is_nil_node(others_top->_childs[i])) {
                wips_top->_childs[i] = static_cast<RBNode<Key, T> *>(tree._nil);
            } else {
                wips_top->_childs[i] =
                    make_new<RBNode<Key, T>>(*tree._allocator, others_top->k, others_top->v);
                others_stack.push_back(others_top->_childs[i]);
                wips_stack.push_back(wips_top->_childs[i]);
            }
        }
    }
}

template <typename Key, typename T, bool is_const>
KT<RBNode, Key, T, is_const> *find(KT<RBTree, Key, T, is_const> &rbt, const Key &k) {
    auto cur_node = rbt._root;
    while (!is_nil_node(rbt, cur_node)) {
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

template <int left, int right, typename Key, typename T> void rotate(RBTree<Key, T> &t, RBNode<Key, T> *x) {
    auto y = x->_childs[right];
    x->_childs[right] = y->_childs[left];
    if (!is_nil_node(t, y->_childs[left])) {
        y->_childs[left]->_parent = x;
    }
    y->_parent = x->_parent;
    if (is_nil_node(t, x->_parent)) {
        t._root = y;
    } else if (x == x->_parent->_childs[left]) {
        x->_parent->_childs[left] = y;
    } else {
        x->_parent->_childs[right] = y;
    }
    y->_childs[left] = x;
    x->_parent = y;
}

template <typename Key, typename T>
void transplant(RBTree<Key, T> &t, RBNode<Key, T> *n1, RBNode<Key, T> *n2) {
    if (n1 == t._root) {
        t._root = n2;
    } else if (n1->_parent->_childs[0] == n1) {
        n1->_parent->_childs[0] = n2;
    } else {
        n1->_parent->_childs[1] = n2;
    }
    n2->_parent = n1->_parent;
}

template <int left, int right, typename Key, typename T>
RBNode<Key, T> *insert_fix(RBTree<Key, T> &t, RBNode<Key, T> *z) {
    auto y = z->_parent->_parent->_childs[right];
    if (y->_color == RED) {
        z->_parent->_color = BLACK;
        y->_color = BLACK;
        z->_parent->_parent->_color = RED;
        z = z->_parent->_parent;
    } else {
        if (z == z->_parent->_childs[right]) {
            z = z->_parent;
            rotate<left, right>(t, z);
        }
        z->_parent->_color = BLACK;
        z->_parent->_parent->_color = RED;
        rotate<right, left>(t, z->_parent->_parent);
    }
    return z;
}

template <int left, int right, typename Key, typename T>
RBNode<Key, T> *remove_fix(RBTree<Key, T> &t, RBNode<Key, T> *x) {
    auto w = x->_parent->_childs[right];
    if (w->_color == RED) {
        w->_color = BLACK;
        x->_parent->_color = RED;
        rotate<left, right>(t, x->_parent);
        w = x->_parent->_childs[right];
    }
    if (w->_childs[left]->_color == BLACK && w->_childs[right]->_color == BLACK) {
        w->_color = RED;
        x = x->_parent;
    } else {
        if (w->_childs[right]->_color == BLACK) {
            w->_childs[left]->_color = BLACK;
            w->_color = RED;
            rotate<right, left>(t, w);
            w = x->_parent->_childs[right];
        }
        w->_color = x->_parent->_color;
        x->_parent->_color = BLACK;
        w->_childs[right]->_color = BLACK;
        rotate<left, right>(t, x->_parent);
        x = t._root;
    }
    return x;
}

// Returns the next in-order node for the given `node`. Given node must not be pointing to `nil` or be
// nullptr.
template <typename Key, typename T, bool is_const>
KT<RBNode, Key, T, is_const> *next_inorder_node(KT<RBTree, Key, T, is_const> &t,
                                                KT<RBNode, Key, T, is_const> *node) {
    // Leaf node? Keep going up parent pointers as long as node is a right child
    if (is_nil_node(t, node->_childs[LEFT]) && is_nil_node(t, node->_childs[RIGHT])) {
        while (node->parent->_childs[RIGHT] == node) {
            node = node->_parent;
        }
        return node->_parent;
    }

    // Not leaf node. Get the minimum node of right subtree.
    node = node->_childs[RIGHT];
    if (is_nil_node(t, node)) {
        return node;
    }
    while (!is_nil_node(t, node->_childs[LEFT])) {
        node = node->_childs[LEFT];
    }
    return node;
}

template <typename Key, typename T, bool is_const>
KT<RBNode, Key, T, is_const> *prev_inorder_node(KT<RBTree, Key, T, is_const> &t,
                                                KT<RBNode, Key, T, is_const> *node) {
    // Leaf node? Keep going up pointers as long as node is a left child
    if (is_nil_node(t, node->_childs[LEFT]) && is_nil_node(t, node->_childs[RIGHT])) {
        while (node->parent->_childs[LEFT] == node) {
            node = node->_parent;
        }
        return node->_parent;
    }

    // Not leaf node. Get the maximum of left subtree
    node = node->_childs[LEFT];
    if (is_nil_node(t, node)) {
        return node;
    }
    while (!is_nil_node(t, node->_childs[RIGHT])) {
        node = node->_childs[RIGHT];
    }
    return node;
}

} // namespace internal
} // namespace rbt

namespace rbt {

template <typename Key, typename T> RBTree<Key, T>::RBTree(Allocator &allocator) {
    _allocator = &allocator;
    _nil = make_new<internal::ChildPointers<Key, T>>(*_allocator);
    _nil->_childs[LEFT] = reinterpret_cast<RBNode<Key, T> *>(_nil);
    _nil->_childs[RIGHT] = reinterpret_cast<RBNode<Key, T> *>(_nil);
    _nil->_parent = reinterpret_cast<RBNode<Key, T> *>(_nil);
    // _nil->_parent = nullptr;
    // _nil->_childs[LEFT] = nullptr;
    // _nil->_childs[RIGHT] = nullptr;
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
        return *this;
    }

    assert(_allocator != nullptr && "Cannot copy into moved-from/deleted tree");

    rbt::internal::delete_all_nodes(*this);
    rbt::internal::copy_tree(*this, other);

    return *this;
}

template <typename Key, typename T> RBTree<Key, T> &RBTree<Key, T>::operator=(RBTree &&other) {
    if (this == other) {
        return *this;
    }

    rbt::internal::delete_all_nodes(*this);

    _allocator = other._allocator;
    _root = other._root;
    _nil = other._nil;

    other._allocator = nullptr;

    return *this;
}

template <typename Key, typename T> RBTree<Key, T>::~RBTree() {
    if (_allocator) {
        rbt::internal::delete_all_nodes(*this);
        _allocator = nullptr;
    }
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> begin(KT<RBTree, Key, T, is_const> &t) {
    return Iterator<Key, T, is_const>(t._root);
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> end(KT<RBTree, Key, T, is_const> &t) {
    using NodeType = typename Iterator<Key, T, is_const>::NodeType;
    return Iterator<Key, T, is_const>(reinterpret_cast<NodeType *>(t._nil));
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const>::Iterator(Iterator<Key, T, is_const>::TreeType *tree,
                                     Iterator<Key, T, is_const>::NodeType *node)
    : _tree(tree)
    : _node(node) {}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> &Iterator<Key, T, is_const>::operator++() {
    _node = internal::next_inorder_node(_tree, _node);
    return *this;
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> Iterator<Key, T, is_const>::operator++(int) {
    Iterator<Key, T, is_const> saved(*this);
    this->operator++();
    return saved;
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> &Iterator<Key, T, is_const>::operator--() {
    _node = internal::prev_inorder_node(_tree, _node);
    return *this;
}

template <typename Key, typename T, bool is_const>
Iterator<Key, T, is_const> Iterator<Key, T, is_const>::operator--(int) {
    Iterator<Key, T, is_const> saved(*this);
    this->operator--();
    return saved;
}

template <typename Key, typename T, bool is_const_pointer> struct Result {
    bool key_was_present = false;
    KT<RBNode, Key, T, is_const_pointer> *node = nullptr;
};

template <typename Key, typename T> Result<Key, T, false> get(RBTree<Key, T> &rbt, const Key &k) {
    auto node = internal::find<Key, T, false>(rbt, k);
    if (is_nil_node(rbt, node)) {
        return Result<Key, T, false>();
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
    if (is_nil_node(rbt, rbt._root)) {
        rbt._root = make_new<RBNode<Key, T>>(*rbt._allocator, std::move(k), std::move(v));
        rbt._root->_childs[0] = reinterpret_cast<RBNode<Key, T> *>(rbt._nil);
        rbt._root->_childs[1] = reinterpret_cast<RBNode<Key, T> *>(rbt._nil);
        rbt._root->_parent = reinterpret_cast<RBNode<Key, T> *>(rbt._nil);
        return Result<Key, T, false>{false, rbt._root};
    }

    Result<Key, T, false> result;

    auto cur = rbt._root;
    auto par = rbt._root;
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

    auto n = make_new<RBNode<Key, T>>(*rbt._allocator, std::move(k), std::move(v));

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
            n = internal::insert_fix<LEFT, RIGHT>(rbt, n);
        } else {
            n = internal::insert_fix<RIGHT, LEFT>(rbt, n);
        }
    }
    rbt._root->_color = BLACK;
    return result;
}

// Removes the node with the given key.
template <typename Key, typename T> Result<Key, T, false> remove(RBTree<Key, T> &t, Key k) {
    auto n = internal::find<Key, T, false>(t, k);

    if (is_nil_node(t, n)) {
        return Result<Key, T, false>{};
    }

    assert(n->k == k);

    auto y = n;
    auto orig_color = n->_color;
    RBNode<Key, T> *x;
    if (is_nil_node(t, n->_childs[LEFT])) {
        x = n->_childs[RIGHT];
        transplant(t, n, n->_childs[RIGHT]);
    } else if (is_nil_node(t, n->_childs[RIGHT])) {
        x = n->_childs[LEFT];
        transplant(t, n, n->_childs[LEFT]);
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
            transplant(t, y, y->_childs[RIGHT]);
            y->_childs[RIGHT] = n->_childs[RIGHT];
            y->_childs[RIGHT]->_parent = y;
        }
        transplant(t, n, y);
        y->_childs[LEFT] = n->_childs[LEFT];
        y->_childs[LEFT]->_parent = y;
        y->_color = n->_color;
    }

    if (orig_color != RED) {
        // remove fix
        while (x != t._root && x->_color == BLACK) {
            if (x == x->_parent->_childs[LEFT]) {
                x = internal::remove_fix<LEFT, RIGHT>(t, x);
            } else {
                x = internal::remove_fix<RIGHT, LEFT>(t, x);
            }
        }
        x->_color = BLACK;
    }

    // Delete the node
    make_delete(*t._allocator, n);

    Result<Key, T, false> result{};
    result.key_was_present = true;
    return result;
}

} // namespace rbt

} // namespace fo