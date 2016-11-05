#pragma once

#include <scaffold/memory.h>

#include "const_log.h"
#include "debug.h"
#include "dysmallintarray.h"
#include "memory.h"

/// A "buddy allocator"

namespace foundation {

// Note -
//
// How to know the size of the buddy the user is requesting to deallocate(),
// and how to efficiently find adjacent free buddies of a level that can
// satisfy a size to allocate:
//
// The largest number of buddies that can be 'unfree' at an instant is 2 **
// (_num_levels - 1). Now, each of these buddies is mapped onto a unique
// address in the buffer. So using this addresses as indices we need 2 **
// (num_buddies - 1) indices to uniquely identify the _start_ position of a
// buddy.
//
// But not the level from where this buddy was allocated - Remember, the size
// of the allocated buddy is just a function of the level and it is one-to-
// one correspondence. As a buddy with index 0 can have size (_buffer_size >>
// l) for any level l = 0 to (_num_levels - 1). To store the level l of the a
// buddy whose index is i, we use a bitset (SmallIntArray) and set all of the
// indices' level to 0 on initialization and set the level for buddy i to the
// proper l when we allocate the buddy. This way on deallocation, we get the
// corresponding index i from the pointer passed and the level from the
// bitset.

namespace _internal {
/// Forwardd declaring the type for headers used to chain the free buddies of
/// a level as a doubly linked list. A BuddyHead itself does not identify a
/// buddy. The list in which it resides in determines the level(hence, the
/// size) of the buddy.
struct BuddyHead;
}

class BuddyAllocator : public Allocator {
  private:
    uint64_t _buffer_size;                      // Size of the whole buffer
    uint64_t _min_buddy_size;                   // Size of a smallest-buddy
    uint64_t _num_levels;                       // Number of levels
    uint64_t _min_buddy_size_power;             // 2**_min_buddy_size_power=_min_buddy_size
    uint64_t _num_indices;                      // Number of smallest-buddies
    _internal::BuddyHead **_free_lists;         // Array of free lists for each level
    DySmallIntArray<uint64_t> _index_allocated; // Is buddy with the given index, allocated?
    DySmallIntArray<uint64_t> _level_of_index;  // At which level is the buddy residing?
    void *_mem;                                 // Pointer to buffer
    Allocator *_main_allocator;                 // The allocator used to allocate the buffer
    Allocator *_extra_allocator;                // The allocator used to allocate the data structures
    uint64_t _total_allocated;                  // Total allocated at any moment

  public:
    /// Creates a buddy allocator. It will manage  buddies of size
    /// `min_buddy_size` in a buffer of `size` bytes. `size` must be a power
    /// of 2. `main_allocator` is the allocator used to allocate that.
    /// `extra_allocator` is use to allocate the internal data structures.
    BuddyAllocator(uint64_t size, uint64_t min_buddy_size, Allocator &main_allocator,
                   Allocator &extra_allocator = memory_globals::default_allocator());

    /// Destroys the buddy allocator and frees the buffer
    ~BuddyAllocator();

    /// Total size allocated
    uint64_t total_allocated() override;
    /// Returns size allocated for block pointed to by p. p must be obtained
    /// from a call to `allocate'
    uint64_t allocated_size(void *p) override;
    /// Allocates a block of `size` bytes (clipped to nearest power of 2). The
    /// returned pointer is aligned to a multiple of `align`.
    void *allocate(uint64_t size, uint64_t align) override;
    /// Deallocates the buddy. If p is null, then it's a nop.
    void deallocate(void *p) override;

  private:
    /// Returns the size of any buddy that resides at the given `level`
    inline uint64_t _buddy_size_at_level(uint64_t level) const;

    /// Just a helper
    uint64_t _last_level() const { return _num_levels - 1; }
    /// Returns pointer into buffer at the given offset
    void *_mem_at(uint64_t off) const { return (char *)_mem + off; }
    /// Returns the offset of the buddy
    uint64_t _off_at(void *p, int level) const { return ((char *)p - (char *)_mem) >> level; }
    /// Casts p to BuddyHead*. In debug mode, checks if p is indeed pointing
    /// to a buddy head.
    inline _internal::BuddyHead *_head_at(void *p);
    /// Returns index of the buddy i.e the offset in units of `min_buddy_size`
    /// chunks
    inline uint64_t _buddy_index(_internal::BuddyHead *p) const;
    /// Returns the number of smallest-buddies contained at this `level`
    inline uint64_t _buddies_contained(uint64_t level) const;

    /// Breaks the top free buddy residing in the given `level` into two,
    /// level must be < _last_level() (ensured by the assertion in
    /// `allocate`). Updates the `free_list` and returns pointer to the first
    /// of the two resulting headers.
    _internal::BuddyHead *_break_free(uint64_t level);
    /// Pushes a free buddy into the given `level`.
    void _push_free(_internal::BuddyHead *h, uint64_t level);
    // A check to ensure we did the correct math :)
    void _check_buddy_index(_internal::BuddyHead *p) const;
    /// Prints which level each buddy is allocated in (Only used for debugging)
    void dbg_print_levels(uint64_t start, uint64_t end) const;

}; // class BuddyAllocator

} // namespace foundation
