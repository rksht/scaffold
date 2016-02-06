#pragma once

#include "memory.h"
#include "const_log.h"
#include "smallintarray.h"

#include <stdint.h>
#include <limits>
#include <bitset>
#include <string.h>
#include <stdio.h>
#include <cassert>

/// A "buddy allocator"

namespace foundation {

// The two main problems are: How to know the size of the buddy the user
// is requesting to deallocate(), and how to efficiently find adjacent free
// buddies of a level that can satisfy a size to allocate.
//
// The largest number of buddies that can be 'unfree' at any instant is 2 **
// (num_levels - 1). Now, each of these buddies is mapped onto a unique
// address in the buffer. So using this addresses as indices we need 2 **
// (num_buddies - 1) indices to uniquely identify the _start_ position of a
// buddy.
//
// But not the level from where this buddy was allocated - Remember, the size
// of the allocated buddy is just a function of the level and it is one- to-
// one. As a buddy with index 0 can have size (buffer_size >> l) for any
// level l = 0 to (num_levels - 1). To store the level l of the a buddy whose
// index is i, we use a bitset (SmallIntArray) and set all of the indices'
// level to 0 on initialization and set the level for buddy i to the proper l
// when we allocate the buddy. This way on deallocation, we get the
// corresponding index i from the pointer passed and the level from the
// bitset.

namespace _internal {

/// Wrapper class to contain the definition of a header struct that is the
/// same for any instance of the `BuddyAllocator` templace class
class BuddyHeader {
  protected:
    /// The type for headers used to thread the free buddies of a level as a
    /// doubly linked list.
    struct Header {
        Header *_next;
        Header *_prev;

        Header() : _next(nullptr), _prev(nullptr) {}
        Header(Header *next, Header *prev) : _next(next), _prev(prev) {}

        /// Removes this header from the free list given
        void remove_self_from_list(Header **free_lists, int level) {
            if (_prev) {
                _prev->_next = _next;
                if (_next) {
                    _next->_prev = _prev;
                }
            } else {
                // Need to set the free list to _next if it was the head of the
                // list
                free_lists[level] = _next;
                if (_next) {
                    _next->_prev = nullptr;
                }
            }
            destroy();
        }

        /// Make the header meaninglesss
        void destroy() {
            _next = nullptr;
            _prev = nullptr;
        }

        bool is_meaningless() { return _next == nullptr && _prev == nullptr; }
    };
};
} // namespace internal

/// The implementation proper of the `BuddyAllocator` class template
template <uint64_t buffer_size, uint32_t num_levels = 32>
class BuddyAllocator : public Allocator, public _internal::BuddyHeader {
  private:
    /// Returns the size of any buddy that resides at the given `level`
    static inline constexpr uint64_t _buddy_size_at_level(uint32_t level) {
        return buffer_size >> (uint64_t)level;
    }

  private:
    /// (static) Last level (for convenience)
    static constexpr uint32_t _last_level = num_levels - 1;

    /// (static) Minimum buddy size
    static constexpr uint64_t _min_buddy_size =
        _buddy_size_at_level(_last_level);

    /// (static) Minimum buddy size power
    static constexpr uint32_t _min_buddy_size_power =
        log2_ceil(_min_buddy_size);

  public:
    /// (static) Alignment
    static constexpr uint32_t _buddy_alignment = buffer_size / _min_buddy_size;

  private:
    /// Pointers to free lists of each level
    Header *_free_lists[num_levels];

    /// Bitset where each bit represents the possible start address of a buddy
    /// and the corresponding bit a buddy with that address is allocated. Note
    /// that a buddy is uniquely determined by a start address and a level, the
    /// latter determinining the size of the buddy, so we do need to store the
    /// level of any allocated buddy alongside as the API for deallocate only
    /// takes the pointer to the start address of the buddy to be deallocated
    std::bitset<(1 << _last_level)> _index_allocated;

    /// Level at which the buddy - denoted by its index - is residing
    SmallIntArray<log2_ceil(num_levels), 1 << _last_level> _level_of_index;

    /// To support total_allocated() call
    uint64_t _total_allocated;

    /// Backing allocator to use
    Allocator *_backing;

    /// Buffer start
    void *_mem;

    /// We always specify all sizes in units of bytes (except buddy indices).
    /// The maximum buffer size is 4GB, with is 2 << 32, and not possible to
    /// represent using uint32_t. So we always use uint64_t for specifying the
    /// size in bytes and uint32_t for other stuff like level number and index.
    static_assert(buffer_size <= uint64_t(4) << 30,
                  "Buffer size should not exceed 4-GB");

    static_assert(num_levels > 0, "num_levels must be > 0");

    static_assert(_min_buddy_size >= sizeof(Header),
                  "minimum buddy size is not sufficient with this config");

    // Make sure the buddy allocator's buffer is a power of 2
    static_assert(clip_to_power_of_2(buffer_size) == buffer_size,
                  "buffer size must be a power of 2");

  public:
    BuddyAllocator(Allocator &backing_allocator)
        : _backing(&backing_allocator), _mem(nullptr) {
        _initialize();

        printf("Buddy allocator initialized - _min_buddy_size = %lu, "
               "_min_buddy_size_power = %lu, num_levels = %u, num_indices = "
               "%u\n--\n",
               _min_buddy_size, _min_buddy_size_power, num_levels,
               1 << _last_level);
    }

    ~BuddyAllocator() {
        _backing->deallocate(_mem);
        assert(_total_allocated == 0 && "Leaked memory!");
    }

    uint32_t total_allocated() override { return _total_allocated; }

    uint32_t allocated_size(void *p) override {
        const uint64_t idx = _buddy_index(p);
        const uint32_t level = _level_of_index.get(idx);
        return _buddy_size_at_level(level);
    }

    /// Allocation implementation
    void *allocate(uint32_t size, uint32_t align) override {
        (void)align; // unused
        printf("Size = %u, ", size);
        size = clip_to_power_of_2(size);
        printf("Size = %u, \n", size);
        assert(size >= _min_buddy_size &&
               "Cannot allocate a buddy size smaller than the min");

        assert(_buddy_alignment % align == 0 &&
               "Aligned looser than _buddy_alignment");

        if (size > buffer_size) {
            printf("Warning - size of memory requested is larger than buffer "
                   "size(=%lu)\n",
                   buffer_size);
        }

        int level = _last_level;
        while (true) {
            const uint64_t buddy_size = _buddy_size_at_level((uint32_t)level);
            // Either buddies are not big enough  or none are free, either
            // way, need to go to upper level
            if (buddy_size < size || _free_lists[level] == nullptr) {
                --level;
                assert(level >= 0 && "Size of memory requested is larger than "
                                     "the whole buffer size");
                continue;
            }

            // OK, at least 1 free and buddy size is actually a greater power of
            // 2, break it in half and put the buddies in the lower level just
            // below, and check the lower level
            if (buddy_size > size) {
                printf("BuddyAllocator - Break - level - %d\n", level);
                Header *h = _break_free(level);
                (void)h;
                ++level;
            } else if (buddy_size == size) {
                // Got the buddy with the exact size
                void *addr = (void *)_free_lists[level];
                const uint64_t index = _buddy_index(addr);

#if 0
                if (_level_of_index.get(index) != level) {
                    printf("Bad level? %d Index = %lu\n", level, index);
                    assert(0);
                }
#endif

                _index_allocated[index] = true;
                _pop_free(level);
                _total_allocated += size;
                printf("BuddyAllocator - Allocated - Level - %u, Index - %u "
                       "(Size = %u)\n--\n",
                       level, index, size);
                assert(addr >= _mem);
                return addr;
            }
        }
    }

    /// Deallocation implementation
    void deallocate(void *p) override {
        if (p == nullptr) {
            return;
        }

        // Corresponding header pointer - make it meaningless for confidence
        Header *h = (Header *)p;
        h->destroy();

        // Get the index of the buddy as known from the address and then the
        // level and size of this buddy
        uint64_t idx = _buddy_index(p);
        const int this_level = (int)_level_of_index.get(idx);
        int level = this_level;

        _index_allocated[idx] = false;

        _total_allocated -= _buddy_size_at_level((uint32_t)level);

        while (level > 0) {
            printf("%d iteration\n", this_level - level + 1);
            uint64_t size = _buddy_size_at_level((uint32_t)level);

            // Check if the adjacent levels can be repeatedly merged
            Header *m = nullptr;

            // If the buddy's index is even then buddy to right is the candidate
            // for merging, otherwise it's the one to the left
            if (idx % 2 == 0) {
                m = (Header *)((char *)h + size);
            } else {
                m = (Header *)((char *)h - size);
                h = m;
            }
            // Is it free and also at the same level? If so, merge the two and
            // put the resulting buddy in the upper level. Remember that h is
            // not in the free list in the first round of the loop as buddy
            // corresponding with h was allocated.
            const uint32_t adj_idx = _buddy_index(m);
            printf("BuddyAllocator - Try to merge (Level - %d), indexes - (%u, "
                   "%u)\n",
                   level, idx, adj_idx);
            fflush(stdout);
            if (!_index_allocated[adj_idx] &&
                _level_of_index.get(adj_idx) == (uint32_t)level) {
                printf(
                    "BuddyAllocator - Merge (Level - %d), indexes - (%u, %u)\n",
                    level, idx, adj_idx);

                // Remove both the buddies from the lists
                m->remove_self_from_list(_free_lists, level);
                if (level < this_level) {
                    h->remove_self_from_list(_free_lists, level);
                }
                // The new level
                --level;
                _push_free(h, level);
                _level_of_index.set(idx, (uint32_t)level);
                _level_of_index.set(adj_idx, (uint32_t)level);
                idx = _buddy_index((void *)h);
            } else {
                // Could not merge with any - break out
                h->destroy();
                _push_free(h, (uint32_t)level);
                _index_allocated[idx] = false;
                break;
            }
        }
        printf("BuddyAllocator - Deallocated(Prev level = %i, New level = %i "
               "Index - %u)\n--\n",
               this_level, level, idx);
    }

    void print_level_map() { _level_of_index.print(); }

  private:
    /// Allocates the buffer and sets up the free lists.
    void _initialize() {
        _mem = _backing->allocate(buffer_size, _buddy_alignment);
        assert(_mem != nullptr && "backing allocator failed?!");
        _total_allocated = 0;

// No need to do this as
// (i) bitset sets all bits to 0
// (ii) SmallIntArray sets all bits to 0
#if 0
        const uint64_t num_buddies = 1 << (num_levels - 1);
        for (uint64_t i = 0; i < num_buddies; ++i) {
            _index_allocated[i] = 0;
            _level_of_index[i] = 0;
        }
#endif

        // All free lists should be empty on initialization except...
        memset(_free_lists, 0, sizeof(_free_lists));
        // ... the top level free list
        ((Header *)_mem)->destroy();
        _push_free((Header *)_mem, 0);
    }

    /// Returns pointer into buffer at the given offset
    void *_mem_at(uint64_t off) { return (char *)_mem + off; }

    /// Returns the offset of the buddy
    uint64_t _off_at(void *p, int level) {
        return ((char *)p - (char *)_mem) >> level;
    }

    /// Returns index of the buddy i.e the offset in units of `min_buddy_size`
    /// chunks
    uint32_t _buddy_index(void *p) {
        assert(p >= _mem);
        const uint64_t diff = (char *)p - (char *)_mem;
        return uint32_t(diff >> _min_buddy_size_power);
    }

    /// Breaks the top free buddy into two, level must be < _last_level
    /// (ensured by the assertion in `allocate`). Updates the `free_list` and
    /// returns pointer to the first of the two resulting headers.
    Header *_break_free(uint32_t level) {
        const uint32_t new_level = level + 1;

        Header *h_level = _free_lists[level];
        Header *h1 = h_level;
        Header *h2 =
            (Header *)((char *)h_level + (_buddy_size_at_level(new_level)));

        _pop_free(level);
        _level_of_index.set(_buddy_index((void *)h1), new_level);
        _level_of_index.set(_buddy_index((void *)h2), new_level);

        assert(_level_of_index.get(_buddy_index((void *)h1)) == new_level);

        // `_push_free` will assert this
        h1->destroy();
        h2->destroy();

        _push_free(h2, new_level);
        _push_free(h1, new_level);
        return h1;
    }

    void _push_free(Header *h, uint32_t level) {
        assert(h->is_meaningless() && "Must be meaningless");

        h->_next = _free_lists[level];
        h->_prev = nullptr;
        if (_free_lists[level] != nullptr) {
            _free_lists[level]->_prev = h;
        }
        _free_lists[level] = h;
    }

    void _pop_free(uint32_t level) {
        if (_free_lists[level] != nullptr) {
            _free_lists[level] = _free_lists[level]->_next;
            if (_free_lists[level] != nullptr) {
                _free_lists[level]->_prev = nullptr;
            }
        }
    }
};
}
