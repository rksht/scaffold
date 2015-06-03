#include <stdio.h>
#include <assert.h>
#include "pod_hash.h"
#include "memory.h"
#include "murmur_hash.h"

using namespace foundation;

struct Data {
    uint64_t id;
    uint64_t hp;
    uint64_t mp;
};

uint64_t Data_hash(const Data* d)
{
    return murmur_hash_64(d, sizeof(Data), 0xDEADBEEF);
}

bool operator==(const Data& d1, const Data& d2)
{
    return d1.id == d2.id && d1.hp == d2.hp && d1.mp == d2.mp;
}

int main()
{
    Data D1 = {100lu, 100lu, 100lu};
    Data D2 = {201lu, 202lu, 203lu};
    Data D3 = {90lu, 90lu, 90lu};

    memory_globals::init();
    {
        pod_hash::Hash<Data, uint64_t> h(memory_globals::default_allocator(),
                memory_globals::default_allocator(), Data_hash);
        assert(pod_hash::has(h, D1) == false);
        pod_hash::set(h, D1, 0x10lu);
        assert(pod_hash::has(h, D1) == true);
        assert(pod_hash::set_default(h, D2, 0x10lu) == 0x10lu);
        assert(pod_hash::get_default(h, D2, 0x100lu) == 0x10lu);

        pod_hash::remove(h, D1);
        pod_hash::remove(h, D2);

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            pod_hash::set(h, d, i * i);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(pod_hash::get_default(h, d, 0lu) == i * i);
        }

        printf("Max chain length: %i\n", pod_hash::max_chain_length(h));

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            pod_hash::remove(h, d);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(pod_hash::has(h, d) == false);
        }
    }
    memory_globals::shutdown();
}
