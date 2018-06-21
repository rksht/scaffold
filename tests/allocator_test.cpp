#include <scaffold/array.h>
#include <scaffold/debug.h>
#include <scaffold/memory.h>

#include <signal.h>

using namespace fo;

int main() {
    memory_globals::init(512);
    {
        Array<u32> arr(memory_globals::default_scratch_allocator());
        reserve(arr, 512);

        log_info("Allocated size = %lu", memory_globals::default_scratch_allocator().allocated_size(arr._data));

        for (u32 i = 0; i < 512; ++i) {
            log_info("Pushing %u", i);
            push_back(arr, i);
        }

        Array<u32> arr1(memory_globals::default_scratch_allocator());
        reserve(arr, 16);

        for (u32 i = 0; i < 128; ++i) {
            log_info("Pusing second %u", i);
            push_back(arr1, i);
        }
    }
    memory_globals::shutdown();
}
