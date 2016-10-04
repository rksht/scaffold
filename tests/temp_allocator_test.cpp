#include <scaffold/array.h>
#include <scaffold/temp_allocator.h>

#include <algorithm>
#include <assert.h>
#include <new>

using namespace foundation;

constexpr auto BUFFER_SIZE = 64u;
using TA = TempAllocator<BUFFER_SIZE>;

using ElemTy = uint64_t;

static constexpr unsigned resize_amount(unsigned bytes) {
    return bytes / sizeof(ElemTy);
}

static char the_allocator alignas(TA)[sizeof(TA)];

int main() {
    memory_globals::init();
    {
        new (the_allocator) TA{};

        Array<ElemTy> arr{*((TA *)the_allocator)};

        array::resize(arr, resize_amount(BUFFER_SIZE));

        array::resize(arr, resize_amount(1 << 20));

        for (auto &n : arr) {
            n = 0xcafebab1;
        }

        assert(std::all_of(arr.begin(), arr.end(),
                           [](auto x) { return x == 0xcafebab1; }));

        ((TA *)the_allocator)->~TA();
    }
    memory_globals::shutdown();
}