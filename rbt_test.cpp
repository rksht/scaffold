#include "rbt.h"

#include "memory.h"
#include <iostream>

using namespace foundation;

const int NR_NODES = 100;

void print_tree(rbt::RBNode<int, int> *nil, rbt::RBNode<int, int> *node) {
    if (node->_kids[rbt::LEFT] != nil) {
        print_tree(nil, node->_kids[rbt::LEFT]);
    }
    std::cout << node->_key << ", " << node->_val << "\n";

    if (node->_kids[rbt::RIGHT] != nil) {
        print_tree(nil, node->_kids[rbt::RIGHT]);
    }
}

int main() {
    memory_globals::init();
    {

        rbt::RBT<int, int> tree{memory_globals::default_allocator()};

        using Int2Int = rbt::RBNode<int, int>;

        for (int i = 0; i < NR_NODES; ++i) {
            rbt::RBNode<int, int> *node = MAKE_NEW(
                memory_globals::default_arena_allocator(), Int2Int, i, i);
            tree.insert(node);
        }

        print_tree(tree.nil_node(), tree._root);
    }

    memory_globals::shutdown();
}
