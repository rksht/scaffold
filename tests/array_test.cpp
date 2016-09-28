#include <scaffold/array.h>
#include <scaffold/debug.h>

#include <algorithm>
#include <assert.h>

using namespace foundation;

int main() {
    memory_globals::init();
    {
        Array<int> arr1(memory_globals::default_allocator());
        Array<int> arr2(memory_globals::default_allocator());

        for (int i = 0; i < 1024; ++i) {
            array::push_back(arr1, 1);
        }

        for (int i = 0; i < 5120; ++i) {
            array::push_back(arr2, 2);
        }

        std::swap(arr1, arr2);

        assert(array::size(arr2) == 1024);
        assert(array::size(arr1) == 5120);

        assert(std::all_of(std::cbegin(arr1), std::cend(arr1),
                           [](int i) { return i == 2; }));

        assert(std::all_of(std::cbegin(arr2), std::cend(arr2),
                           [](int i) { return i == 1; }));

        // Move
        arr1 = std::move(arr2);
        assert(array::size(arr1) == 1024);
        assert(array::size(arr2) == 0);
        assert(std::all_of(std::cbegin(arr1), std::cend(arr1),
                           [](int i) { return i == 1; }));

        // Reuse arr2
        array::resize(arr2, 2000);
        for (int i = 0; i < 2000; ++i) {
            arr2[i] = 0xbeef;
        }
        assert(array::size(arr2) == 2000);
        assert(std::all_of(std::cbegin(arr2), std::cend(arr2),
                           [](int i) { return i == 0xbeef; }));

// Resize in debug mode is zero-filled
#ifndef NDEBUG
        array::resize(arr1, 2048);
        assert(
            std::all_of(std::cbegin(arr1) + 1024, std::cend(arr1), [](int i) {
                if (i != 0) {
                    debug("i == %d", i);
                }
                return i == 0;
            }));
#else
        array::resize(arr1, 2048);
#endif
    }
    memory_globals::shutdown();
}
