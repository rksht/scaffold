#include <scaffold/arena_allocator.h>
#include <scaffold/array.h>

using namespace fo;

template <size_t size> struct Block { u32 a[size]; };

void do_thing() {
    ArenaAllocator aa(fo::memory_globals::default_allocator(), 4096);
    fo::Array<Block<512>> blocks(aa);

    const u32 num_iters = 1000;

    fo::reserve(blocks, num_iters);

    for (u32 i = 0; i < num_iters; ++i) {
        fo::push_back(blocks, Block<512>());
        log_info("Pushed block - %u", i);
    }

    log_info("Total allocated after %lf KB iterations, or request for %u bytes = %lu",
             num_iters * 512 / 1024.0,
             num_iters * 512,
             aa.total_allocated());

    fo::Array<ArenaInfo> arena_chain;
    aa.get_chain_info(arena_chain);

    for (u32 i = 0; i < size(arena_chain); ++i) {
        auto &info = arena_chain[i];
        printf("%*cArena %u, size = %.3f, allocated = %.3f\n",
               i * 2,
               ' ',
               i,
               info.buffer_size / 1024.0,
               info.total_allocated / 1024.0);
    }
}

int main() {

    memory_globals::init();

    { do_thing(); }

    memory_globals::shutdown();
}