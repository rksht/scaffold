#include <scaffold/debug.h>
#include <scaffold/memory.h>
#include <scaffold/murmur_hash.h>
#include <scaffold/pod_hash.h>
#include <scaffold/pod_hash_usuals.h>

#include <assert.h>
#include <stdio.h>

using namespace fo;

struct Data {
    uint64_t id;
    uint64_t hp;
    uint64_t mp;
};

uint64_t Data_hash(const Data &d) { return murmur_hash_64(&d, sizeof(Data), 0xDEADBEEF); }

bool Data_equal(const Data &d1, const Data &d2) { return d1.id == d2.id && d1.hp == d2.hp && d1.mp == d2.mp; }

int main() {
    Data D1 = {100lu, 100lu, 100lu};
    Data D2 = {201lu, 202lu, 203lu};

    memory_globals::init();

    {
        PodHash<Data, uint64_t> h(memory_globals::default_allocator(), memory_globals::default_allocator(),
                                  Data_hash, Data_equal);

        pod_hash::reserve(h, 512);

        assert(pod_hash::has(h, D1) == false);
        pod_hash::set(h, D1, uint64_t(0x10lu));
        assert(pod_hash::has(h, D1) == true);
        assert(pod_hash::set_default(h, D2, uint64_t(0x10lu)) == 0x10lu);

        pod_hash::remove(h, D1);
        pod_hash::remove(h, D2);

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            pod_hash::set(h, d, i * i);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(pod_hash::set_default(h, d, uint64_t(0lu)) == i * i);
        }

        log_info("Max chain length: %i\n", pod_hash::max_chain_length(h));

        PodHash<Data, uint64_t> new_hash = h;

        uint64_t i = 0;
        for (const auto &e : h) {
            assert(e.value == i * i);
            i++;
        }

        i = 0;
        for (const auto &e : new_hash) {
            assert(e.value == i * i);
            i++;
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            PodHash<Data, uint64_t>::iterator res = pod_hash::get(h, d);
            assert(res != end(h) && res->value == i * i);
            res->value = i * i * i;
            pod_hash::remove(h, d);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(!pod_hash::has(h, d));
        }

        PodHash<char, uint64_t> h1(memory_globals::default_allocator(), memory_globals::default_allocator(),
                                   usual_hash<char>, usual_equal<char>);

        for (char i = 'a'; i < 'z'; ++i) {
            pod_hash::set(h1, i, (uint64_t)(i * i));
        }

        for (char i = 'a'; i < 'z'; ++i) {
            pod_hash::remove(h1, i);
        }
    }
    memory_globals::shutdown();
}
