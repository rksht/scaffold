#include <stdio.h>
#include <assert.h>
#include "pod_hash.h"
#include "memory.h"
#include "murmur_hash.h"
#include "pod_hash_usuals.h"

using namespace foundation;

struct Data {
    uint64_t id;
    uint64_t hp;
    uint64_t mp;
};

uint64_t Data_hash(const Data &d) {
    return murmur_hash_64(&d, sizeof(Data), 0xDEADBEEF);
}

bool Data_equal(const Data &d1, const Data &d2) {
    return d1.id == d2.id && d1.hp == d2.hp && d1.mp == d2.mp;
}

int main() {
    Data D1 = {100lu, 100lu, 100lu};
    Data D2 = {201lu, 202lu, 203lu};

    memory_globals::init();

    {
        using namespace pod_hash;

        PodHash<Data, uint64_t> h(memory_globals::default_allocator(),
                                            memory_globals::default_allocator(),
                                            Data_hash, Data_equal);

        pod_hash::reserve(h, 512);

        assert(has(h, D1) == false);
        set(h, D1, 0x10lu);
        assert(has(h, D1) == true);
        assert(set_default(h, D2, 0x10lu) == 0x10lu);

        remove(h, D1);
        remove(h, D2);

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            set(h, d, i * i);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(set_default(h, d, 0lu) == i * i);
        }

        printf("Max chain length: %i\n", max_chain_length(h));

        for (const auto &e : h) {
            printf("id = %lu\n", e.key.id);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            remove(h, d);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(has(h, d) == false);
        }

        PodHash<char, uint64_t> h1(
            memory_globals::default_allocator(),
            memory_globals::default_allocator(), usual_hash<char>,
            usual_equal<char>);

        for (char i = 'a'; i < 'z'; ++i) {
            set(h1, i, (uint64_t)(i * i));
        }

        for (char i = 'a'; i < 'z'; ++i) {
            remove(h1, i);
        }
    }
    memory_globals::shutdown();
}
