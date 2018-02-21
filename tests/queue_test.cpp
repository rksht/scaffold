#include <scaffold/debug.h>
#include <scaffold/queue.h>

#include <algorithm>
#include <assert.h>

using namespace fo;

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

        Queue<int> q3(memory_globals::default_allocator());

        queue::reserve(q3, 10);

        queue::push_back(q3, 0);
        queue::push_back(q3, 1);
        queue::push_back(q3, 2);
        queue::push_back(q3, 3);
        queue::push_back(q3, 4);
        queue::push_back(q3, 5);

        // [0, 1, 2, 3, 4, 5]

        queue::pop_front(q3);
        queue::pop_front(q3);

        // [2, 3, 4, 5]

        queue::push_back(q3, 6);
        queue::push_back(q3, 7);
        queue::push_back(q3, 8);

        // [2, 3, 4, 5, 6, 7, 8]

        auto ex = queue::get_extent(q3, 0, 5);

        assert(ex.first_chunk_size + ex.second_chunk_size == 5);

        assert(ex.first_chunk_size == 5);

        assert(ex.first_chunk[0] == 2);
        assert(ex.first_chunk[1] == 3);
        assert(ex.first_chunk[2] == 4);
        assert(ex.first_chunk[3] == 5);
        assert(ex.first_chunk[4] == 6);

        // [-, -, 2, 3, 4, 5, 6, 7, 8, -]

        queue::push_back(q3, 9);
        queue::push_back(q3, 10);
        queue::push_back(q3, 11);

        // [10, 11, 2, 3, 4, 5, 6, 7, 8, 9]
        // [2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

        assert(queue::size(q3) == 10);

        ex = queue::get_extent(q3, 2, 10);

        assert(ex.first_chunk_size + ex.second_chunk_size == 10);
        assert(ex.first_chunk_size == 8);

        assert(ex.first_chunk[0] == 2);
        assert(ex.first_chunk[1] == 3);
        assert(ex.first_chunk[2] == 4);
        assert(ex.first_chunk[3] == 5);
        assert(ex.first_chunk[4] == 6);
        assert(ex.first_chunk[5] == 7);
        assert(ex.first_chunk[6] == 8);
        assert(ex.first_chunk[7] == 9);

        assert(ex.second_chunk[0] == 10);
        assert(ex.second_chunk[1] == 11);
    }

    memory_globals::shutdown();

    return 0;
}