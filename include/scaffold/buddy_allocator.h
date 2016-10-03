#pragma once

#include "const_log.h"
#include "debug.h"
#include "memory.h"
#include "smallintarray.h"

#include <assert.h>
#include <limits>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// A "buddy allocator"

namespace foundation {

// Note -
//
// How to know the size of the buddy the user is requesting to deallocate(),
// and how to efficiently find adjacent free buddies of a level that can
// satisfy a size to allocate:
//
// The largest number of buddies that can be 'unfree' at any instant is 2 **
// (_num_levels - 1). Now, each of these buddies is mapped onto a unique
// address in the buffer. So using this addresses as indices we need 2 **
// (num_buddies - 1) indices to uniquely identify the _start_ position of a
// buddy.
//
// But not the level from where this buddy was allocated - Remember, the size
// of the allocated buddy is just a function of the level and it is one- to-
// one. As a buddy with index 0 can have size (buffer_size >> l) for any
// level l = 0 to (_num_levels - 1). To store the level l of the a buddy whose
// index is i, we use a bitset (SmallIntArray) and set all of the indices'
// level to 0 on initialization and set the level for buddy i to the proper l
// when we allocate the buddy. This way on deallocation, we get the
// corresponding index i from the pointer passed and the level from the
// bitset.

/// The implementation proper of the `BuddyAllocator` class template.
template <uint64_t buffer_size, uint32_t _min_buddy_size>
class BuddyAllocator : public Allocator {

  private:
    /// The type for headers used to thread the free buddies of a level as a
    /// doubly linked list.
    struct BuddyHead {
        BuddyHead *_next;
        BuddyHead *_prev;

        BuddyHead() : _next(nullptr), _prev(nullptr) {}
        BuddyHead(BuddyHead *next, BuddyHead *prev)
            : _next(next), _prev(prev) {}

        /// Removes this header from the free list given
        void remove_self_from_list(BuddyHead **free_lists, int level) {
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

  private:
    /// Returns the size of any buddy that resides at the given `level`
    static inline constexpr uint64_t _buddy_size_at_level(uint32_t level) {
        return buffer_size >> (uint64_t)level;
    }

  private:
    static constexpr uint32_t _num_levels =
        log2_ceil(buffer_size / _min_buddy_size) + 1;

    /// (static) Last level (for convenience)
    static constexpr uint32_t _last_level = _num_levels - 1;

    /// (static) Minimum buddy size power
    static constexpr uint32_t _min_buddy_size_power =
        log2_ceil(_min_buddy_size);

    /// (static) Number of buddy indices
    static constexpr uint32_t _num_indices = 1 << _last_level;

  public:
    /// (static) Alignment
    static constexpr uint32_t _buddy_alignment = _min_buddy_size;

  private:
    /// Pointers to free lists of each level
    BuddyHead *_free_lists[_num_levels];

    /// Bitset where each bit represents the possible start address of a buddy
    /// and the corresponding bit a buddy with that address is allocated. Note
    /// that a buddy is uniquely determined by a start address and a level, the
    /// latter determinining the size of the buddy, so we do need to store the
    /// level of any allocated buddy alongside as the API for deallocate only
    /// takes the pointer to the start address of the buddy to be deallocated
    SmallIntArray<1, _num_indices, uint32_t> _index_allocated;

    /// Level at which the buddy - denoted by its i:is residing
    SmallIntArray<log2_ceil(_num_levels), _num_indices, uint32_t>
        _level_of_index;

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

    static_assert(_num_levels > 0, "_num_levels must be > 0");

    static_assert(_min_buddy_size >= sizeof(BuddyHead),
                  "minimum buddy size is not sufficient with this config");

    static_assert((_min_buddy_size & (_min_buddy_size - 1)) == 0,
                  "_min_buddy_size should be a power of 2");

    // Make sure the buddy allocator's buffer is a power of 2
    static_assert(clip_to_power_of_2(buffer_size) == buffer_size,
                  "buffer size must be a power of 2");

  public:
    BuddyAllocator(Allocator &backing_allocator)
        : _backing(&backing_allocator), _mem(nullptr) {
        _initialize();

        debug("BuddyAllocator::Initialized - buffer_size = %lu "
              "_min_buddy_size = %u, _min_buddy_size_power = %u, "
              "_num_levels = %u, num_indices = %u\n--",
              buffer_size, _min_buddy_size, _min_buddy_size_power, _num_levels,
              1 << _last_level);
    }

    ~BuddyAllocator() {
        _backing->deallocate(_mem);
        if (_total_allocated != 0) {
            log_err("BuddyAllocator::Leaking memory?! - Total Allocated = %lu",
                    _total_allocated);
            assert(0);
        }
    }

    uint32_t total_allocated() override { return _total_allocated; }

    uint32_t allocated_size(void *p) override {
        const uint64_t idx = _buddy_index((BuddyHead *)p);
        const uint32_t level = _level_of_index.get(idx);
        return _buddy_size_at_level(level);
    }

    /// Allocation implementation
    void *allocate(uint32_t size, uint32_t align) override {
        (void)align; // unused
        size = clip_to_power_of_2(size);
        assert(size >= _min_buddy_size &&
               "Cannot allocate a buddy size smaller than the min");

        assert(_buddy_alignment % align == 0 &&
               "Aligned looser than _buddy_alignment");

        int level = _last_level;
        debug("Allocating Buddy...");
        while (true) {
            const uint64_t buddy_size = _buddy_size_at_level((uint32_t)level);

            // Either buddies are not big enough  or none are free, either
            // way, need to go to upper level
            if (buddy_size < size || _free_lists[level] == nullptr) {
                --level;
                assert(level >= 0 && "Size requested too big");
                continue;
            }

            // OK, at least 1 free and buddy size is actually a greater power
            // of 2, break it in half and put the buddies in the lower level
            // just below, and check the lower level
            if (buddy_size > size) {
                uint32_t index = _buddy_index(_free_lists[level]);
                print_level_map(index, index + _buddies_contained(level));
                debug("BuddyAlloc::Allocate::Break i:%u - level - %d", index,
                      level);
                _break_free(level);
                print_level_map(index, index + _buddies_contained(level));
                ++level;
                print_level_map();
            } else if (buddy_size == size) {
                // Got the buddy with the exact size
                BuddyHead *h = _free_lists[level];
                const uint64_t index = _buddy_index(h);

                if (_level_of_index.get(index) != (uint32_t)level) {
                    log_err("BuddyAlloc::Bad i:%lu, Freelist level = %d, "
                            "Stored level "
                            "= %u",
                            index, level, _level_of_index.get(index));
                    assert(0 && "See error");
                }

                const uint64_t next_index = _buddies_contained(level);
                debug("BuddyAlloc::Found exact size(%lu buddies) TO ALLOC - "
                      "i:%lu",
                      next_index, index);
                print_level_map(index, next_index >= _num_indices ? _num_indices
                                                                  : next_index);
                h->remove_self_from_list(_free_lists, level);
                // Set each to allocated status
                for (uint32_t b = index, e = index + _buddies_contained(level);
                     b < e; ++b) {
                    _index_allocated.set(b, 1);
                }
                _total_allocated += size;

                log_info("BuddyAllocator::Allocated::Level - %u, i:%lu (Size = "
                         "%u)\n--",
                         level, index, size);
                print_level_map(index, next_index);
                return (void *)h;
            }
        }
    }

    /// Deallocation implementation
    void deallocate(void *p) override {
        if (p == nullptr) {
            return;
        }

        // Corresponding header pointer - make it meaningless for confidence.
        BuddyHead *h = _head_at(p);
        h->destroy();

        _check_buddy_index(h);

        // Get the index of the buddy as known from the address and then the
        // level and size of this buddy
        uint32_t idx = _buddy_index(h);
        const uint32_t original_level = _level_of_index.get(idx);
        uint32_t level = original_level;

        assert(_index_allocated.get(idx));

        const uint64_t size = _buddy_size_at_level((uint32_t)level);

        if (!((int64_t)_total_allocated - (int64_t)size >= 0)) {
            log_err("BuddyAlloc::Unallocated index (index_allocated = %d? "
                    "i:%u, level - %d, size - %lu, _total_allocated - %lu",
                    int(_index_allocated.get(idx)), idx, level, size,
                    _total_allocated);
            assert(0 && "See error");
        }

        print_level_map(idx, idx + 20);

        _index_allocated.set(idx, 0);
        _total_allocated -= size;

        // Put h back into free list
        h->destroy();
        _push_free(h, level);

        // Try to keep merging
        BuddyHead *left = h;
        BuddyHead *right = nullptr;
        BuddyHead *tmp = nullptr;

        debug(
            "BuddyAlloc::deallocate - set initial block headed by i:%u to free",
            idx);
        print_level_map(idx,
                        idx + 20 >= _num_indices ? _num_indices : idx + 20);

        while (level >= 1) {
            debug("BuddyAlloc::MERGE START - %d, level - %d",
                  original_level - level, level);
            print_level_map();
            const uint64_t size = _buddy_size_at_level(level);
            const uint32_t buddies_inside = _buddies_contained(level);
            uint32_t left_idx = _buddy_index(left);
            uint32_t right_idx = left_idx + buddies_inside;
            // Suppose that a buddy head is pointing to a buddy currently
            // located at level 'n'. The level can be 'chunked' into
            // _buddies_contained(n) buddies of size _buddy_size_at_level(n). We
            // want to know the index of the buddy our current buddy head is
            // pointing at level n, if we see level n as such an array. If that
            // index is odd, the buddy we should merge with is located to th
            // eleft, otherwise it's to the right.
            const uint32_t level_idx = left_idx / (1 << (_last_level - level));
            if (level_idx % 2 != 0) {
                tmp = left;
                left = (BuddyHead *)((char *)left - size);
                right = tmp;
                left_idx = left_idx - buddies_inside;
                right_idx = left_idx + buddies_inside;
            } else {
                right = (BuddyHead *)((char *)left + size);
            }

            // One of the buddies left or right is guaranteed to be free, but
            // we don't track which to keep it all simple. Same argument for
            // level - one of them is guaranteed to be in the current `level`.
            if (!_index_allocated.get(left_idx) &&
                !_index_allocated.get(right_idx) &&
                _level_of_index.get(left_idx) ==
                    _level_of_index.get(right_idx)) {
                debug("\t BEFORE MERGE - left i:%u and right i:%u", left_idx,
                      right_idx);

                // Print the surrounding buddy status
                print_level_map(idx, idx + 20 >= _num_indices ? _num_indices
                                                              : idx + 20);

                left->remove_self_from_list(_free_lists, level);
                right->remove_self_from_list(_free_lists, level);
                --level;
                _push_free(left, level);

                debug("BuddyAlloc::AFTER MERGE - left i:%u and right i:%u",
                      left_idx, right_idx);
                print_level_map(idx, idx + 20 >= _num_indices ? _num_indices
                                                              : idx + 20);

            } else {
                debug("BuddyAlloc::DONE MERGING");
                break;
            }
        }
        debug("Done deallocating (and possible merge) Prev level = %i, Cur "
              "level = %i i:%u, Size = -%lu)\n--",
              original_level, level, idx, size);
    }

    /// Prints which level each buddy is allocated in (Only used for debugging)
    void print_level_map(uint32_t start = 0,
                         uint32_t end = _num_indices) const {
#ifdef BUDDY_ALLOC_LEVEL_LOGGING
        using namespace string_stream;
        Buffer b(memory_globals::default_allocator());

        fprintf(stderr, "+--------LEVEL MAP--------+\n");

        for (auto i = start; i < end; ++i) {
            b << i << "=" << _level_of_index.get(i) << "\t";
            tab(b, 8);
            if (array::size(b) >= 80) {
                fprintf(stderr, "%s\n", c_str(b));
                array::clear(b);
            }
        }
        if (array::size(b) != 0) {
            fprintf(stderr, "%s\n", c_str(b));
            array::clear(b);
        }
        fprintf(stderr, "\n--\n");

        for (uint32_t i = start; i < end; ++i) {
            b << i << "-" << (_index_allocated.get(i) ? "[x]" : "[]") << "\t";
            tab(b, 8);
            if (array::size(b) >= 80) {
                fprintf(stderr, "%s\n", c_str(b));
                array::clear(b);
            }
        }
        if (array::size(b) != 0) {
            fprintf(stderr, "%s\n", c_str(b));
        }
        fprintf(stderr, "\n--\n");

        for (uint32_t i = 0; i < _num_levels; ++i) {
            BuddyHead *h = _free_lists[i];
            while (h) {
                fprintf(stderr, "%lu at %u\n", _buddy_index(h), i);
                h = h->_next;
            }
        }
        fprintf(stderr, "+-------END-------+\n**\n");
#else
        (void)start;
        (void)end;
#endif
    }

  private:
    /// Allocates the buffer and sets up the free lists.
    void _initialize() {
        _mem = _backing->allocate(buffer_size, _buddy_alignment);
        assert(_mem != nullptr && "backing allocator failed?!");
        _total_allocated = 0;
        // All free lists should be empty on initialization except...
        memset(_free_lists, 0, sizeof(_free_lists));
        // ... the top level free list
        ((BuddyHead *)_mem)->destroy();
        _push_free((BuddyHead *)_mem, 0);
    }

    /// Returns pointer into buffer at the given offset
    void *_mem_at(uint64_t off) const { return (char *)_mem + off; }

    /// Returns the offset of the buddy
    uint64_t _off_at(void *p, int level) const {
        return ((char *)p - (char *)_mem) >> level;
    }

    BuddyHead *_head_at(void *p) {
#ifndef NDEBUG
        assert(p >= _mem);
        uint64_t diff = (char *)p - (char *)_mem;
        int mod = diff % _min_buddy_size;
        assert(mod == 0);
#endif
        return (BuddyHead *)p;
    }

    /// Returns index of the buddy i.e the offset in units of `min_buddy_size`
    /// chunks
    uint32_t _buddy_index(BuddyHead *p) const {
        assert((char *)p >= (char *)_mem);
        assert((char *)p <= (char *)_mem + buffer_size);
        const uint64_t diff = (char *)p - (char *)_mem;
        // debug("diff = %lu", diff);
        return uint32_t(diff >> _min_buddy_size_power);
    }

    constexpr uint32_t _buddies_contained(uint32_t level) const {
        return 1 << (_last_level - level);
    }

    /// Breaks the top free buddy into two, level must be < _last_level
    /// (ensured by the assertion in `allocate`). Updates the `free_list` and
    /// returns pointer to the first of the two resulting headers.
    BuddyHead *_break_free(uint32_t level) {
        const uint32_t new_level = level + 1;

        BuddyHead *h_level = _free_lists[level];
        BuddyHead *h1 = h_level;
        BuddyHead *h2 =
            (BuddyHead *)((char *)h_level + _buddy_size_at_level(new_level));

#ifndef NDEBUG
        if (!(_level_of_index.get(_buddy_index(h1)) == level)) {
            log_err("h1's level(index = %u) = %u, but found in level %u",
                    _buddy_index(h1), _level_of_index.get(_buddy_index(h1)),
                    level);
            assert(0 && "See error");
        }
        if (!(_level_of_index.get(_buddy_index(h2)) == level)) {
            log_err("h2's(index = %u) level = %u, but found in level %u",
                    _buddy_index(h2), _level_of_index.get(_buddy_index(h2)),
                    level);
            assert(0 && "See error");
        }
#endif

        h_level->remove_self_from_list(_free_lists, level);

        // `_push_free` will assert this
        h1->destroy();
        h2->destroy();
        _push_free(h2, new_level);
        _push_free(h1, new_level);

        return h1;
    }

    void _push_free(BuddyHead *h, uint32_t level) {
        assert(h->is_meaningless() && "Must be meaningless");
        assert(h != _free_lists[level] && "This really should not happen");

        h->_next = _free_lists[level];
        h->_prev = nullptr;
        if (_free_lists[level] != nullptr) {
            _free_lists[level]->_prev = h;
        }
        _free_lists[level] = h;
        const uint32_t index = _buddy_index(h);
        const uint32_t last = index + _buddies_contained(level);
        debug("BuddyAlloc::Setting levels - h = %p, level = %u, index = %u, "
              "last = %u",
              h, level, index, last);
        _level_of_index.set_range(index, last, level);
        for (uint32_t b = index; b < last; ++b) {
            _index_allocated.set(b, 0);
        }
    }

    // A check to ensure we did the correct math :)
    void _check_buddy_index(BuddyHead *p) {
        uint32_t index = _buddy_index(p);
        uint32_t level = _level_of_index.get(index);
        uint32_t buddies_inside = _buddies_contained(level);

        uint32_t mod = index % buddies_inside;
        if (mod != 0) {
            log_err("Buddy block of size %u can never begin at index %u and be "
                    "at level %u",
                    buddies_inside << _min_buddy_size_power, index, level);
            abort();
        }
    }
}; // class BuddyAllocator

} // namespace foundation
