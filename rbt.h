#pragma once

#include <memory> // std::move
#include <assert.h>

/// Namespace rbt contains a non-owning red-black tree implementation.
namespace rbt {

/// Call this before using anything in namespace rbt
void init();

enum RBColor { BLACK = 0, RED };
const int LEFT = 0;
const int RIGHT = 1;

/// The `RBNode` objects are allocated and initialized by the user themselves
/// and then inserted into the tree. The keys of each node must be unique (i.e
/// operator=(k1, k2) returns false). The tree does not own the nodes and will
/// not free them. So don't forget to walk the tree and free the nodes!
template <typename K, typename V> struct RBNode {
    /// Color of node
    RBColor _color;
    /// The parent node
    RBNode<K, V> *_parent;
    /// The two kid nodes
    RBNode<K, V> *_kids[2];

    /// Key
    K _key;
    /// Value
    V _val;

    RBNode(const K &key, const V &val)
        : _color(BLACK), _parent(nullptr), _kids{nullptr, nullptr}, _key(key),
          _val(val) {}

    RBNode(K &&key, V &&val) : _key(std::move(key)), _val(std::move(val)) {}

    RBNode(const RBNode<K, V> &other) = delete;
};

// The nil node shared by any and every instance `RBT<K, V>`
extern struct Nil {
    RBColor _color;
    Nil *_parent;
    Nil *_kids[2];
} g_nil_node;

/// Returns a pointer to the nil node cast to `RBNode<K, V>*`
template <typename K, typename V> static inline RBNode<K, V> *NilNode() {
    return reinterpret_cast<RBNode<K, V> *>(&g_nil_node);
}

/// RBT<K, V> represents a red black tree.
template <typename K, typename V> struct RBT {
    /// The pointer to root
    RBNode<K, V> *_root;

    /// Number of nodes in the tree
    size_t _nr_nodes;

    /// Constructor
    RBT() : _root(NilNode<K, V>()), _nr_nodes(0) {}

    ~RBT() {
        // On deletion, just set these to nullptr to catch any error
        _root = nullptr;
    }

    /// No copy construction. That would require the tree to own the nodes
    /// hence have knowledge of how to allocate and deallocate nodes - which
    /// it does not.
    RBT(const RBT<K, V> &other) = delete;

    /// Move constructor
    RBT(RBT<K, V> &&other) {
        _root = other._root;
        other._root = nullptr;
    }

    /// Insert a node
    void insert(RBNode<K, V> *n);

    /// Removes the node given and returns it (return value same as the node
    /// given)
    RBNode<K, V> *remove(RBNode<K, V> *n);

    /// Returns a pointer to the node with the given key
    RBNode<K, V> *get_node(const K &key);

  private:
    /// Transplants the given node `n2` to the given node `n1`'s position
    void _graft(RBNode<K, V> *n1, RBNode<K, V> *n2);

    /// Rotations
    template <int left, int right> void _rotate(RBNode<K, V> *n);

    /// Bottom-up fixing for insertion
    template <int left, int right>
    inline RBNode<K, V> *_insert_fix(RBNode<K, V> *z);

    /// Bottom-up fixing for deletion
    template <int left, int right>
    inline RBNode<K, V> *_remove_fix(RBNode<K, V> *x);
};

} // namespace rbt

namespace rbt {
template <typename K, typename V>
static inline bool is_root(RBT<K, V> *t, RBNode<K, V> *n) {
    return n->_parent == NilNode<K, V>();
}

template <typename K, typename V>
RBNode<K, V> *RBT<K, V>::get_node(const K &key) {
    auto cur = _root;
    while (cur != NilNode<K, V>()) {
        if (cur->_key == key) {
            return cur;
        }
        if (key < cur->_key) {
            cur = cur->_kids[LEFT];
        } else {
            cur = cur->_kids[RIGHT];
        }
    }
    return nullptr;
}

template <typename K, typename V>
void RBT<K, V>::_graft(RBNode<K, V> *n1, RBNode<K, V> *n2) {
    if (is_root(this, n1)) {
        _root = n2;
    } else if (n1->_parent->_kids[0] == n1) {
        n1->_parent->_kids[0] = n2;
    } else {
        n1->_parent->_kids[1] = n2;
    }
    n2->_parent = n1->_parent;
}

template <typename K, typename V> void RBT<K, V>::insert(RBNode<K, V> *n) {
    if (_root == NilNode<K, V>()) {
        _root = n;
        n->_color = BLACK;
        n->_parent = NilNode<K, V>();
        n->_kids[LEFT] = NilNode<K, V>();
        n->_kids[RIGHT] = NilNode<K, V>();
        return;
    }

    RBNode<K, V> *cur = _root;
    RBNode<K, V> *par = _root;
    int dir = LEFT;
    while (cur != NilNode<K, V>()) {
        assert(cur != nullptr);
        par = cur;
        assert(n->_key != cur->_key);
        if (n->_key < cur->_key) {
            cur = cur->_kids[LEFT];
            dir = LEFT;
        } else {
            cur = cur->_kids[RIGHT];
            dir = RIGHT;
        }
    }
    n->_color = RED;
    n->_parent = par;
    n->_kids[LEFT] = NilNode<K, V>();
    n->_kids[RIGHT] = NilNode<K, V>();
    par->_kids[dir] = n;
    ++_nr_nodes;

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

template <typename K, typename V>
template <int left, int right>
RBNode<K, V> *RBT<K, V>::_insert_fix(RBNode<K, V> *z) {
    auto y = z->_parent->_parent->_kids[right];
    if (y->_color == RED) {
        z->_parent->_color = BLACK;
        y->_color = BLACK;
        z->_parent->_parent->_color = RED;
        z = z->_parent->_parent;
    } else {
        if (z == z->_parent->_kids[right]) {
            z = z->_parent;
            _rotate<left, right>(z);
        }
        z->_parent->_color = BLACK;
        z->_parent->_parent->_color = RED;
        _rotate<right, left>(z->_parent->_parent);
    }
    return z;
}

template <typename K, typename V>
template <int left, int right>
void RBT<K, V>::_rotate(RBNode<K, V> *x) {
    auto y = x->_kids[right];
    x->_kids[right] = y->_kids[left];
    if (y->_kids[left] != NilNode<K, V>()) {
        y->_kids[left]->_parent = x;
    }
    y->_parent = x->_parent;
    if (x->_parent == NilNode<K, V>()) {
        _root = y;
    } else if (x == x->_parent->_kids[left]) {
        x->_parent->_kids[left] = y;
    } else {
        x->_parent->_kids[right] = y;
    }
    y->_kids[left] = x;
    x->_parent = y;
}

template <typename K, typename V>
RBNode<K, V> *RBT<K, V>::remove(RBNode<K, V> *n) {
    auto y = n;
    RBColor orig_color = n->_color;
    RBNode<K, V> *x;
    if (n->_kids[LEFT] == NilNode<K, V>()) {
        x = n->_kids[RIGHT];
        _graft(n, n->_kids[RIGHT]);
    } else if (n->_kids[RIGHT] == NilNode<K, V>()) {
        x = n->_kids[LEFT];
        _graft(n, n->_kids[RIGHT]);
    } else {
        // minimum of n->_kids[RIGHT]
        auto min = n->_kids[RIGHT];
        while (min->_kids[LEFT] != NilNode<K, V>()) {
            min = min->_kids[LEFT];
        }
        y = min;
        orig_color = y->_color;
        x = y->_kids[RIGHT];
        if (y->_parent == n) {
            x->_parent = y;
        } else {
            _graft(y, y->_kids[RIGHT]);
            y->_kids[RIGHT] = n->_kids[RIGHT];
            y->_kids[RIGHT]->_parent = y;
        }
        _graft(n, y);
        y->_kids[LEFT] = n->_kids[LEFT];
        y->_kids[LEFT]->_parent = y;
        y->_color = n->_color;
    }
    if (orig_color == RED) {
        return n;
    }
    // remove fix
    while (x != _root && x->_color == BLACK) {
        if (x == x->_parent->_kids[LEFT]) {
            x = _remove_fix<LEFT, RIGHT>(x);
        } else {
            x = _remove_fix<RIGHT, LEFT>(x);
        }
    }
    x->_color = BLACK;
    return n;
}

template <typename K, typename V>
template <int left, int right>
RBNode<K, V> *RBT<K, V>::_remove_fix(RBNode<K, V> *x) {
    auto w = x->_parent->_kids[right];
    if (w->_color == RED) {
        w->_color = BLACK;
        x->_parent->_color = RED;
        _rotate<left, right>(x->_parent);
        w = x->_parent->_kids[right];
    }
    if (w->_kids[left]->_color == BLACK && w->_kids[right]->_color == BLACK) {
        w->_color = RED;
        x = x->_parent;
    } else {
        if (w->_kids[right]->_color == BLACK) {
            w->_kids[left]->_color = BLACK;
            w->_color = RED;
            _rotate<right, left>(w);
            w = x->_parent->_kids[right];
        }
        w->_color = x->_parent->_color;
        x->_parent->_color = BLACK;
        w->_kids[right]->_color = BLACK;
        _rotate<left, right>(x->_parent);
        x = _root;
    }
    return x;
}
} // namespace rbt
