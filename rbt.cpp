#include "rbt.h"

namespace rbt {

struct Nil g_nil_node;

void init() {
    g_nil_node._color = BLACK;
    g_nil_node._parent = nullptr;
    g_nil_node._kids[0] = nullptr;
    g_nil_node._kids[1] = nullptr;
}

} // namespace rbt

#ifdef RBT_TEST

#include "memory.h"
#include <iostream>

using namespace foundation;

const int NR_NODES = 100;

void print_tree(rbt::RBNode<int, int> *node) {
    if (node->_kids[rbt::LEFT] != rbt::NilNode<int, int>()) {
        print_tree(node->_kids[rbt::LEFT]);
    }
    std::cout << node->_key << ", " << node->_val << "\n";

    if (node->_kids[rbt::RIGHT] != rbt::NilNode<int, int>()) {
        print_tree(node->_kids[rbt::RIGHT]);
    }
}

int main() {
    memory_globals::init();
    auto &alloc = memory_globals::default_arena_allocator();

    rbt::init();
    rbt::RBT<int, int> tree;

    using Int2Int = rbt::RBNode<int, int>;

    for (int i = 0; i < NR_NODES; ++i) {
        rbt::RBNode<int, int> *node = MAKE_NEW(alloc, Int2Int, i, i);
        tree.insert(node);
    }

    print_tree(tree._root);
    memory_globals::shutdown();
}

#endif
