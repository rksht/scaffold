#include <scaffold/rbt.h>

using namespace fo;

int main() {
    memory_globals::init();
    {
        fo::rbt::RBTree<int, int> rbt(memory_globals::default_allocator());
    }

    memory_globals::shutdown();
}
