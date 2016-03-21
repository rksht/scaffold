/// Use foundation::MallocAllocator as operator new
#include "memory.h"
#include <vector>
#include <new>
#include <cstdlib>

void *operator new(std::size_t size) {
    return foundation::memory_globals::default_allocator().allocate(size, 16);
}

void operator delete(void *p) throws std::bad_alloc {
    foundation::memory_globals::default_allocator().deallocate(p);
}

int main() {
    foundation::memory_globals::init();

    {
        std::vector<uint64_t> v(10000);
        std::vector<uint64_t> v1(800);
    }

    foundation::memory_globals::shutdown();
}
