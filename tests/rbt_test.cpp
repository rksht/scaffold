#include <scaffold/memory.h>
#include <scaffold/rbt.h>

// For testing
#include <algorithm>
#include <iostream>
#include <iterator>
#include <math.h>
#include <string>
#include <vector>

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

    foundation::memory_globals::init();

    {

        // Creating a vector filled with random nodes
        std::vector<Tree::node_type> node_store;
        node_store.reserve(num_nodes);
        std::srand(0xDEADBEEF);
        std::generate_n(std::back_inserter(node_store), num_nodes, [&]() {
            int n = std::rand() % 100;
            return Tree::node_type{n, n * 2};
        });

        for (const auto &node : node_store) {
            std::cout << "//K: " << node._key << "\tV: " << node._val << "\n";
        }

        // Create the tree
        Tree tree(foundation::memory_globals::default_allocator());

        // Insert the node pointers
        for (auto &node : node_store) {
            tree.insert(&node);
        }

        print_dot_format(tree.nil_node(), tree._root);
    }

    foundation::memory_globals::shutdown();
}
