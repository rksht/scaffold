#include "rbt.h"

#include "memory.h"
#include <iostream>
#include <random>

using namespace foundation;

const int NR_NODES = 1000;

void print_tree(rbt::RBNode<int, int> *nil, rbt::RBNode<int, int> *node) {
    if (node->_kids[rbt::LEFT] != nil) {
        print_tree(nil, node->_kids[rbt::LEFT]);
        std::cout << node->_key << " -> " << node->_kids[rbt::LEFT]->_key
                  << ";\n";
    }

    std::cout << node->_key << " [fillcolor = "
              << (node->_color == rbt::BLACK ? "grey" : "red") << "];\n";

    if (node->_kids[rbt::RIGHT] != nil) {
        print_tree(nil, node->_kids[rbt::RIGHT]);
        std::cout << node->_key << " -> " << node->_kids[rbt::RIGHT]->_key
                  << ";\n";
    }
}
void print_dot_format(rbt::RBNode<int, int> *nil, rbt::RBNode<int, int> *root) {
    std::cout << "digraph rbt {\n";
    print_tree(nil, root);
    std::cout << "}\n";
}

int main() {
    memory_globals::init();
    {
        std::default_random_engine rng;
        std::uniform_int_distribution<int> d;

        rbt::RBT<int, int> tree{memory_globals::default_allocator()};

        using Int2Int = rbt::RBNode<int, int>;

        for (int i = 0; i < NR_NODES; i += 5) {
            rbt::RBNode<int, int> *node =
                MAKE_NEW(memory_globals::default_arena_allocator(), Int2Int,
                         i + d(rng) % 5, i);

            std::cout << "// Parent of nil node = "
                      << (tree.nil_node()->_parent == nullptr
                              ? "NULL"
                              : std::to_string(tree.nil_node()->_parent->_key))
                      << "\n";
            std::cout
                << "// Lchild "
                << (tree.nil_node()->_kids[rbt::LEFT] == nullptr
                        ? "NULL"
                        : std::to_string(
                              tree.nil_node()->_parent->_kids[rbt::LEFT]->_key))
                << "\n";

            std::cout << "// Rchild "
                      << (tree.nil_node()->_kids[rbt::RIGHT] == nullptr
                              ? "NULL"
                              : std::to_string(tree.nil_node()
                                                   ->_parent->_kids[rbt::RIGHT]
                                                   ->_key))
                      << "\n";

            tree.insert(node);
        }

        print_dot_format(tree.nil_node(), tree._root);
    }

    memory_globals::shutdown();
}
