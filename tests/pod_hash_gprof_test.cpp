#include <scaffold/memory.h>
#include <scaffold/debug.h>
#include <scaffold/pod_hash.h>
#include <scaffold/pod_hash_usuals.h>

int main() {
    using namespace foundation;
    using namespace pod_hash;

    memory_globals::init();
    {
        PodHash<uint64_t, uint64_t> h{
            memory_globals::default_allocator(),
            memory_globals::default_allocator(),
            [](const uint64_t &key) {
                return murmur_hash_64(&key, sizeof(uint64_t), 0xCAFEBABE);
            },
            [](const uint64_t &key1, const uint64_t &key2) {
                return key1 == key2;
            }};

        const uint32_t MAX_ENTRIES = 8192;

        pod_hash::reserve(h, MAX_ENTRIES);

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            pod_hash::set(h, i, 0xdeadbeeflu);
        }

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            auto ret = pod_hash::get(h, i);
            assert(ret.present);
            assert(ret.value == 0xdeadbeeflu);
            if (!ret.present) {
                log_err("Should not happen - ret.value = %lu", ret.value);
                abort();
            }
        }
        fprintf(stderr, "Done\n");
    }
    memory_globals::shutdown();
}