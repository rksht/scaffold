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

        reserve(h, 512);

        assert(has(h, D1) == false);
        set(h, D1, uint64_t(0x10lu));
        assert(has(h, D1) == true);
        assert(set_default(h, D2, uint64_t(0x10lu)) == 0x10lu);

        remove(h, D1);
        remove(h, D2);

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            set(h, d, i * i);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(set_default(h, d, uint64_t(0lu)) == i * i);
        }

        log_info("Max chain length: %i\n", max_chain_length(h));

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
            PodHash<Data, uint64_t>::iterator res = get(h, d);
            assert(res != end(h) && res->value == i * i);
            res->value = i * i * i;
            remove(h, d);
        }

        for (uint64_t i = 0; i < 1000; ++i) {
            Data d = {i, i, i};
            assert(!has(h, d));
        }

        PodHash<char, uint64_t> h1(memory_globals::default_allocator(), memory_globals::default_allocator(),
                                   usual_hash<char>, usual_equal<char>);

        for (char i = 'a'; i < 'z'; ++i) {
            set(h1, i, (uint64_t)(i * i));
        }

        for (char i = 'a'; i < 'z'; ++i) {
            remove(h1, i);
        }

        PodHash<uint32_t, Data> h2(memory_globals::default_allocator(), memory_globals::default_allocator(),
                                   usual_hash<uint32_t>, usual_equal<uint32_t>);

        for (uint32_t i = 0; i < 100; ++i) {
            log_info("i = %u", i);
            auto &data = h2[i];
            assert(data.id == 0);
            assert(data.hp == 0);
            assert(data.mp == 0);
        }
    }
    memory_globals::shutdown();
}
