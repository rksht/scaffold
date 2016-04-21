#include "rbt.h"
#include "memory.h"
#include "arena.h"

#include <iostream>
#include <random>

using namespace foundation;
using namespace rbt;
using Tree = RBT<int, int>;
const size_t num_nodes = 100;

int id = 0;

void print_tree(const RBNode<int, int> *nil, const RBNode<int, int> *node,
                int node_id) {
    if (node->_kids[LEFT] != nil) {
        std::cout << node_id << " -> " << ++id << ";\n";
        print_tree(nil, node->_kids[LEFT], id);
    }

    std::cout << node_id
              << " [fillcolor = " << (node->_color == BLACK ? "grey" : "red")
              << ", label = " << node_id << "];\n";

    if (node->_kids[RIGHT] != nil) {
        std::cout << node_id << " -> " << ++id << ";\n";
        print_tree(nil, node->_kids[RIGHT], id);
    }
}

void print_dot_format(const RBNode<int, int> *nil,
                      const RBNode<int, int> *root) {
    std::cout << "digraph rbt {\n";
    id = 1;
    print_tree(nil, root, 1);
    std::cout << "}\n";
}

int main() {
    memory_globals::init();
    {
        std::default_random_engine rng;
        std::uniform_int_distribution<int> d;

        rbt::RBT<int, int> tree{memory_globals::default_allocator()};

        using Int2Int = rbt::RBNode<int, int>;

        for (int i = 0; i < num_nodes; i += 5) {
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
