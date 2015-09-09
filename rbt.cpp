#include "rbt.h"

namespace rbt {
    struct Nil g_nil_node;

    void init() {
        g_nil_node._color = BLACK;
        g_nil_node._parent = nullptr;
        g_nil_node._kids[0] = nullptr;
        g_nil_node._kids[1] = nullptr;
    }
}