#include <scaffold/debug.h>
#include <scaffold/queue.h>

#include <algorithm>
#include <assert.h>

using namespace foundation;

int main() {
    memory_globals::init();

    {
        Queue<int> q1(memory_globals::default_allocator());
        Queue<int> q2(memory_globals::default_allocator());

        auto pushback = [](Queue<int> &q) {
            for (int i = 0; i < 1024; ++i) {
                queue::push_back(q, i);
            }
        };

        pushback(q1);

        for (int i = 0; i < 1024; ++i) {
            assert(*queue::begin_front(q1) == i);
            queue::pop_front(q1);
        }

        assert(queue::size(q1) == 0);

        pushback(q2);

        assert(queue::size(q2) == 1024);

        std::swap(q1, q2);

        assert(queue::size(q2) == 0);
        assert(queue::size(q1) == 1024);
    }

    memory_globals::shutdown();

    return 0;
}