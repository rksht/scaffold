#pragma once

#include "debug.h"
#include "memory.h"
#include "const_log.h"
#include "smallintarray.h"
#include "jeayeson/jeayeson.hpp"

#include <stdint.h>
#include <limits>
#include <bitset>
#include <map>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/// A "buddy allocator"

namespace foundation {

// How to know the size of the buddy the user is requesting to deallocate(),
// and how to efficiently find adjacent free buddies of a level that can
// satisfy a size to allocate:
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

/// The implementation proper of the `BuddyAllocator` class template.
template <uint64_t buffer_size, uint32_t num_levels = 32>
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
    /// (static) Last level (for convenience)
    static constexpr uint32_t _last_level = num_levels - 1;

    /// (static) Minimum buddy size
    static constexpr uint64_t _min_buddy_size =
        _buddy_size_at_level(_last_level);

    /// (static) Minimum buddy size power
    static constexpr uint32_t _min_buddy_size_power =
        log2_ceil(_min_buddy_size);

    /// (static) Number of buddy indices
    static constexpr uint32_t _num_indices = 1 << _last_level;

  public:
    /// (static) Alignment
    static constexpr uint32_t _buddy_alignment = buffer_size / _min_buddy_size;

  private:
    /// Pointers to free lists of each level
    BuddyHead *_free_lists[num_levels];

    /// Bitset where each bit represents the possible start address of a buddy
    /// and the corresponding bit a buddy with that address is allocated. Note
    /// that a buddy is uniquely determined by a start address and a level, the
    /// latter determinining the size of the buddy, so we do need to store the
    /// level of any allocated buddy alongside as the API for deallocate only
    /// takes the pointer to the start address of the buddy to be deallocated
    std::bitset<_num_indices> _index_allocated;

    /// Level at which the buddy - denoted by its index - is residing
    SmallIntArray<log2_ceil(num_levels), _num_indices> _level_of_index;

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

    static_assert(_min_buddy_size >= sizeof(BuddyHead),
                  "minimum buddy size is not sufficient with this config");

    // Make sure the buddy allocator's buffer is a power of 2
    static_assert(clip_to_power_of_2(buffer_size) == buffer_size,
                  "buffer size must be a power of 2");

  public:
    BuddyAllocator(Allocator &backing_allocator)
        : _backing(&backing_allocator), _mem(nullptr) {
        _initialize();

        debug("Buddy allocator initialized - _min_buddy_size = %lu, "
              "_min_buddy_size_power = %u, num_levels = %u, num_indices = "
              "%u\n--",
              _min_buddy_size, _min_buddy_size_power, num_levels,
              1 << _last_level);
    }

    ~BuddyAllocator() {
        _backing->deallocate(_mem);
        if (_total_allocated != 0) {
            log_err("BuddyAllocator - Leaking memory?! - Total Allocated = %lu",
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
        while (true) {
            const uint64_t buddy_size = _buddy_size_at_level((uint32_t)level);
            // Either buddies are not big enough  or none are free, either
            // way, need to go to upper level
            if (buddy_size < size || _free_lists[level] == nullptr) {
                --level;
                assert(level >= 0 && "Size requested too big");
                continue;
            }

            // OK, at least 1 free and buddy size is actually a greater power of
            // 2, break it in half and put the buddies in the lower level just
            // below, and check the lower level
            if (buddy_size > size) {
                debug("BuddyAllocator - Break index - %u - level - %d",
                      _buddy_index(_free_lists[level]), level);
                BuddyHead *h = _break_free(level);
                (void)h;
                ++level;
#ifndef NDEBUG
                print_level_map();
#endif
            } else if (buddy_size == size) {
                // Got the buddy with the exact size
                BuddyHead *h = _free_lists[level];
                const uint64_t index = _buddy_index(h);

                if (_level_of_index.get(index) != (uint32_t)level) {
                    log_err(
                        "Bad index - %lu, Freelist level = %d, Stored level "
                        "= %u",
                        index, level, _level_of_index.get(index));
                    assert(0 && "See error");
                }

                h->remove_self_from_list(_free_lists, level);
                _index_allocated[index] = true;
                _total_allocated += size;
                debug("BuddyAllocator - Allocated - Level - %u, Index - %lu "
                      "(Size = %u)\n--",
                      level, index, size);
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
        // In the merging loop, `h` will always point to the lesser(one to the
        // left) of the two consecutive buddies being merged.
        BuddyHead *h = _head_at(p);
        h->destroy();

        // Get the index of the buddy as known from the address and then the
        // level and size of this buddy
        uint32_t idx = _buddy_index(h);
        const int original_level = (int)_level_of_index.get(idx);
        int level = original_level;

        assert(_index_allocated[idx] == true);

        const uint64_t size = _buddy_size_at_level((uint32_t)level);

        if (!((int64_t)_total_allocated - (int64_t)size >= 0)) {
            log_err("Unallocated index (index_allocated = %d? Index - %u, "
                    "level - %d, size - %lu, "
                    "_total_allocated - %lu",
                    int(_index_allocated[idx]), idx, level, size,
                    _total_allocated);
            assert(0 && "See error");
        }
        _index_allocated[idx] = false;
        _total_allocated -= size;

        // Put h back into free list
        h->destroy();
        _push_free(h, level);

        // Try to keep merging
        BuddyHead *left = h;
        BuddyHead *right = nullptr;
        BuddyHead *tmp = nullptr;

        while (level >= 1) {
            debug("Merge iteration - %d, level - %d", original_level - level,
                  level);
#ifndef NDEBUG
            print_level_map();
#endif
            const uint64_t size = _buddy_size_at_level((uint32_t)level);
            const uint32_t buddies_inside = _num_indices >> level;
            uint32_t left_idx = _buddy_index(left);
            uint32_t right_idx = left_idx + buddies_inside;
            if (left_idx % 2 != 0) {
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
            if (!_index_allocated[left_idx] && !_index_allocated[right_idx] &&
                _level_of_index.get(left_idx) ==
                    _level_of_index.get(right_idx)) {
                debug("Merging - left - %u and right - %u", left_idx,
                      right_idx);
                left->remove_self_from_list(_free_lists, level);
                right->remove_self_from_list(_free_lists, level);
                --level;
                _push_free(left, level);
            } else {
                break;
            }
        }
        debug("BuddyAllocator - Deallocated(Prev level = %i, New level = %i "
              "Index - %u, Size = -%lu)\n--",
              original_level, level, idx, size);
    }

    void print_level_map() const { _level_of_index.print(); }

    json_map get_json_tree() const {
        json_map top;
        json_array children;
        _json_collect(children, 0, 0);
        top["name"] = "top";
        top["children"] = children;
        return top;
    }

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
        ((BuddyHead *)_mem)->destroy();
        _push_free((BuddyHead *)_mem, 0);
    }

    /// Returns pointer into buffer at the given offset
    void *_mem_at(uint64_t off) { return (char *)_mem + off; }

    /// Returns the offset of the buddy
    uint64_t _off_at(void *p, int level) {
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
    uint32_t _buddy_index(BuddyHead *p) {
        assert((char *)p >= (char *)_mem);
        assert((char *)p <= (char *)_mem + buffer_size);
        const uint64_t diff = (char *)p - (char *)_mem;
        // debug("diff = %lu", diff);
        return uint32_t(diff >> _min_buddy_size_power);
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
            log_err("h2's(index = %lu) level = %u, but found in level %u",
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
        const uint32_t last = index + (_num_indices >> level);
        debug("Setting levels - h = %p, level = %u, index = %u, last = %u", h,
              level, index, last);
        _level_of_index.set_range(index, last, level);
        _index_allocated[index] = false;
    }

    /// BAD, doesn't do it correctly. Re-do.
    void _json_collect(json_array &arr, uint32_t level,
                       uint32_t start_index) const {
        uint64_t buddy_size = _buddy_size_at_level(level);
        if ((_level_of_index.get(start_index) == level &&
             !_index_allocated[start_index]) ||
            level == _last_level) {
            std::cout << start_index << " resides at level " << level
                      << "\n-----\n";
            arr.push_back(json_map{{"name", std::to_string(level)},
                                   {"size", buddy_size}});

            print_level_map();
            return;
        }
        std::cout << "level - " << level << std::endl;
        uint32_t next_level = level + 1;
        uint32_t adj_index =
            start_index + _buddy_size_at_level(next_level) / _min_buddy_size;
        std::cout << "(start, adj) = " << start_index << ", " << adj_index
                  << "\n---\n";
        assert(adj_index < 1 << _last_level);
        json_array children;
        _json_collect(children, next_level, start_index);
        _json_collect(children, next_level, adj_index);
        arr.push_back(json_map{{"children", children}});
    }
};

} // namespace foundation