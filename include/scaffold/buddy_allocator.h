#pragma once

#include <scaffold/memory.h>

#include <scaffold/const_log.h>
#include <scaffold/debug.h>
#include <scaffold/dypackeduintarray.h>
#include <scaffold/memory.h>

/// A "buddy allocator"

namespace fo {

// The algorithm is due to
//
// https://github.com/niklasfrykholm/blog/blob/master/2015/allocation-adventures-3.md
//
// The metadata is kept separately and not in the buffer managed by the allocator itself.

namespace buddy_allocator_internal {

/// Forward declaring the type for headers used to chain the free buddies of a level as a doubly linked list.
/// A BuddyHead itself does not identify a buddy. The list in which it resides in determines the level(hence,
/// the size) of the buddy.
struct BuddyHead;

} // namespace buddy_allocator_internal

class BuddyAllocator : public Allocator {
  private:
    uint64_t _buffer_size;                             // Size of the whole buffer
    uint64_t _leaf_buddy_size;                         // Size of a smallest-buddy
    uint64_t _num_levels;                              // Number of levels
    uint64_t _leaf_buddy_size_power;                   // 2**_leaf_buddy_size_power=_leaf_buddy_size
    uint64_t _num_indices;                             // Number of smallest-buddies
    buddy_allocator_internal::BuddyHead **_free_lists; // Array of free lists for each level
    DyPackedUintArray<uint64_t> _leaf_allocated;       // Is buddy with the given index, allocated?
    DyPackedUintArray<uint64_t> _level_of_leaf;        // At which level is the buddy residing?
    char *_mem;                                        // Pointer to buffer
    Allocator *_main_allocator;                        // The allocator used to allocate the buffer
    Allocator *_extra_allocator;                       // The allocator used to allocate the data structures
    uint64_t _total_allocated;                         // Total allocated at any moment
    uint64_t _unavailable;

  public:
    /// Creates a buddy allocator. It will manage  buddies of size `min_buddy_size` in a buffer of `size`
    /// bytes. `size` must be a power of 2. `main_allocator` is the allocator used to allocate that.
    /// `extra_allocator` is use to allocate the internal data structures.
    BuddyAllocator(uint64_t size, uint64_t min_buddy_size, Allocator &main_allocator,
                   Allocator &extra_allocator = memory_globals::default_allocator());

    /// Destroys the buddy allocator and frees the buffer
    ~BuddyAllocator();

    /// Total size allocated
    uint64_t total_allocated() override;

    /// Returns size allocated for block pointed to by p. p must be obtained from a call to `allocate'
    uint64_t allocated_size(void *p) override;

    /// Allocates a block of `size` bytes (clipped to nearest power of 2). The returned pointer is aligned to
    /// a multiple of `align`.
    void *allocate(uint64_t size, uint64_t align) override;

    /// Deallocates the buddy. If p is null, then it's a nop.
    void deallocate(void *p) override;

  private:
    /// Returns the size of any buddy that resides at the given `level`
    uint64_t _buddy_size_at_level(uint64_t level) const;

    /// Just a helper
    uint64_t _last_level() const { return _num_levels - 1; }

    /// Casts p to BuddyHead*. In debug mode, checks if p is indeed pointing to a buddy head.
    buddy_allocator_internal::BuddyHead *_head_at(void *p);

    /// Returns index of the buddy i.e the offset in units of `min_buddy_size` chunks
    uint64_t _leaf_index(buddy_allocator_internal::BuddyHead *p) const;

    /// Returns the number of min-buddies contained in each buddy of given `level`
    uint64_t _leaves_contained(uint64_t level) const;

    /// Called by the constructor when the total size of memory to manage is not a power of 2
    void _mark_unavailable_buddy();

    /// Breaks the top free buddy residing in the given `level` into two, level must be < _last_level()
    /// (ensured by the assertion in `allocate`). Updates the `free_list` and returns pointer to the first of
    /// the two resulting headers.
    buddy_allocator_internal::BuddyHead *_break_free(uint64_t level);

    /// Pushes a free buddy into the given `level`.
    void _push_free(buddy_allocator_internal::BuddyHead *h, uint64_t level);

    // A check to ensure we did the correct math :)
    void _check_leaf_index(buddy_allocator_internal::BuddyHead *p) const;

    /// Prints which level each buddy is allocated in (Only used for debugging)
    void dbg_print_levels(uint64_t start, uint64_t end) const;

}; // class BuddyAllocator

} // namespace fo
