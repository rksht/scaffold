// Often we want a Red-Black BST that is easily 'augmentable'. I think the best
// way to do this is to simply write a usual red-black tree and modify it by
// hand. Trying to create some kind of 'plugin' interface will make it
// unnecessarily complex. So here goes. The nodes will fully expose the key
// along with the mapped value. It is the user's responsibilty to not tamper
// with the key. Allowing only POD-ish types, as this makes implementing and
// editing the structure even easier.

#include "memory.h"
#include <type_traits>

namespace fo {

namespace rbt {
namespace internal {
template <typename T> struct TypeIsSupported {
    static constexpr bool value = std::is_trivially_copy_assignable<T>::value &&
                                  std::is_trivially_copyable<T>::value &&
                                  std::is_trivially_destructible<T>::value;
};
} // ns internal
} // ns rbt

template <typename Key, typename T> struct RBTreeNode {
    Key k;
    T v;

    RBTreeNode *kids[2];
};

template <typename Key, typename T> struct RBTree {
    static_assert(rbt::internal::TypeIsSupported<Key>::value && rbt::internal::TypeIsSupported<T>::value,
                  "Only support trivial-ish types");

    using key_type = Key;
    using mapped_type = T;
    using value_type = RBTreeNode<Key, T>;

    Allocator *_allocator;
    RBTreeNode *_root;

    RBTree(Allocator &allocator);
    ~RBTree();
    RBTree(const RBTree &o);
    RBTree(RBTree &&o); // Leaves the other RBT in a state as if just constructed
    RBTree &operator=(const RBTree &o);
    RBTree &operator=(RBTree &&o); // Same as above
};

// Operations. Three minimal operations are enough.

namespace rbt {

template <typename Key, typename T> struct GetResult {
    bool key_was_present;
    RBTreeNode<Key, T> *node;
};

template <typename Key, typename T> GetResult<Key, T> get(RBTree<Key, T> &rbt, Key &&k);

template <typename Key, typename T> GetResult<Key, T> get_const(const RBTree<Key, T> &rbt, Key &&k);

template <typename Key, typename T> GetResult<Key, T> get(const RBTree<Key, T> &rbt, Key &&k);

// Removes the node with the given key. The `node` field of the returned struct is always nullptr.
template <typename Key, typename T> GetResult<Key, T> remove(RBTree<Key, T> &rbt, Key &&k);

} // ns rbt

} // ns fo

// Default Implementation

namespace fo {

namespace rbt {
}
}
