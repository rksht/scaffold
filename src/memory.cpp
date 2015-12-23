#include "memory.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <new>

#include <stdio.h>

namespace {
using namespace foundation;

// Header stored at the beginning of a memory allocation to indicate the
// size of the allocated data.
struct Header {
    uint32_t size;
};

// If we need to align the memory allocation we pad the header with this
// value after storing the size. That way we can
const uint32_t HEADER_PAD_VALUE = 0xffffffffu;

// Given a pointer to the header, returns a pointer to the data that follows it.
inline void *data_pointer(Header *header, uint32_t align) {
    void *p = header + 1;
    return memory::align_forward(p, align);
}

// Given a pointer to the data, returns a pointer to the header before it.
inline Header *header(void *data) {
    uint32_t *p = (uint32_t *)data;
    while (p[-1] == HEADER_PAD_VALUE)
        --p;
    return (Header *)p - 1;
}

// Stores the size in the header and pads with HEADER_PAD_VALUE up to the
// data pointer.
inline void fill(Header *header, void *data, uint32_t size) {
    header->size = size;
    uint32_t *p = (uint32_t *)(header + 1);
    while (p < data)
        *p++ = HEADER_PAD_VALUE;
}

/// An allocator that uses the default system malloc(). Allocations are
/// padded so that we can store the size of each allocation and align them
/// to the desired alignment.
///
/// (Note: An OS-specific allocator that can do alignment and tracks size
/// does need this padding and can thus be more efficient than the
/// MallocAllocator.)
class MallocAllocator : public Allocator {
    uint32_t _total_allocated;

    // Returns the size to allocate from malloc() for a given size and align.
    static inline uint32_t size_with_padding(uint32_t size, uint32_t align) {
        return size + align + sizeof(Header);
    }

  public:
    MallocAllocator() : _total_allocated(0) {}

    ~MallocAllocator() {
        // Check that we don't have any memory leaks when allocator is
        // destroyed.
        assert(_total_allocated == 0);
    }

    virtual void *allocate(uint32_t size, uint32_t align) {
        const uint32_t ts = size_with_padding(size, align);
        Header *h = (Header *)malloc(ts);
        void *p = data_pointer(h, align);
        fill(h, p, ts);
        _total_allocated += ts;
        assert((uintptr_t)p % align == 0);

        // printf("ALLOCATE: %p\n", p);

        return p;
    }

    virtual void deallocate(void *p) {
        if (!p)
            return;

        // printf("DEALLOCATE: %p\n", p);

        Header *h = header(p);
        _total_allocated -= h->size;
        free(h);
    }

    virtual uint32_t allocated_size(void *p) { return header(p)->size; }

    virtual uint32_t total_allocated() { return _total_allocated; }
};

/// An allocator used to allocate temporary "scratch" memory. The allocator
/// uses a fixed size ring buffer to services the requests.
///
/// Memory is always always allocated linearly. An allocation pointer is
/// advanced through the buffer as memory is allocated and wraps around at
/// the end of the buffer. Similarly, a free pointer is advanced as memory
/// is freed.
///
/// It is important that the scratch allocator is only used for short-lived
/// memory allocations. A long lived allocator will lock the "free" pointer
/// and prevent the "allocate" pointer from proceeding past it, which means
/// the ring buffer can't be used.
///
/// If the ring buffer is exhausted, the scratch allocator will use its backing
/// allocator to allocate memory instead.
class ScratchAllocator : public Allocator {
    Allocator &_backing;

    // Start and end of the ring buffer.
    char *_begin, *_end;

    // Pointers to where to allocate memory and where to free memory.
    char *_allocate, *_free;

  public:
    /// Creates a ScratchAllocator. The allocator will use the backing
    /// allocator to create the ring buffer and to service any requests
    /// that don't fit in the ring buffer.
    ///
    /// size specifies the size of the ring buffer.
    ScratchAllocator(Allocator &backing, uint32_t size) : _backing(backing) {
        _begin = (char *)_backing.allocate(size);
        _end = _begin + size;
        _allocate = _begin;
        _free = _begin;
    }

    ~ScratchAllocator() {
        assert(_free == _allocate);
        _backing.deallocate(_begin);
    }

    bool in_use(void *p) {
        if (_free == _allocate)
            return false;
        if (_allocate > _free)
            return p >= _free && p < _allocate;
        return p >= _free || p < _allocate;
    }

    virtual void *allocate(uint32_t size, uint32_t align) {
        assert(align % 4 == 0);
        size = ((size + 3) / 4) * 4;

        char *p = _allocate;
        Header *h = (Header *)p;
        char *data = (char *)data_pointer(h, align);
        p = data + size;

        // Reached the end of the buffer, wrap around to the beginning.
        if (p > _end) {
            h->size = (_end - (char *)h) | 0x80000000u;

            p = _begin;
            h = (Header *)p;
            data = (char *)data_pointer(h, align);
            p = data + size;
        }

        // If the buffer is exhausted use the backing allocator instead.
        if (in_use(p))
            return _backing.allocate(size, align);

        fill(h, data, p - (char *)h);
        _allocate = p;
        return data;
    }

    virtual void deallocate(void *p) {
        if (!p)
            return;

        if (p < _begin || p >= _end) {
            _backing.deallocate(p);
            return;
        }

        // Mark this slot as free
        Header *h = header(p);
        assert((h->size & 0x80000000u) == 0);
        h->size = h->size | 0x80000000u;

        // Advance the free pointer past all free slots.
        while (_free != _allocate) {
            Header *h = (Header *)_free;
            if ((h->size & 0x80000000u) == 0)
                break;

            _free += h->size & 0x7fffffffu;
            if (_free == _end)
                _free = _begin;
        }
    }

    virtual uint32_t allocated_size(void *p) {
        Header *h = header(p);
        return h->size - ((char *)p - (char *)h);
    }

    virtual uint32_t total_allocated() { return _end - _begin; }
};

/// An ArenaAllocator allocates a buffer and bumps a pointer on each allocation.
/// This allocator is to be used standalone and not via the other foundation
/// data structures.
class ArenaAllocator : public Allocator {
  private:
    struct _Header {
        uint32_t size;
    };

  private:
    Allocator &_backing;
    void *_mem;
    _Header *_top_header;
    _Header *_next_header;
    uint32_t _total_allocated;
    uint32_t _wasted;

  private:
    // Returns a size that is a multiple of alignof(_Header)
    uint32_t _aligned_size_with_padding(uint32_t size) {
        uint32_t total = size + sizeof(_Header);
        int mod = total % alignof(_Header);
        if (mod) {
            total += alignof(_Header) - mod;
        }
        return total;
    }

  public:
    ArenaAllocator(Allocator &backing, uint32_t size) : _backing(backing) {
        uint32_t adjusted_size = _aligned_size_with_padding(size);
        _mem = _backing.allocate(adjusted_size, alignof(_Header));
        memset(_mem, 0, adjusted_size);
        _top_header = (_Header *)_mem;
        _top_header->size = 0;
        _next_header = _top_header;
        _total_allocated = 0;
    }

    void *allocate(uint32_t size, uint32_t align) override {
        void *after_header, *aligned_start;
        uint32_t pad_size;

        if (size % alignof(_Header) != 0) {
            printf("Size requested is not a multiple of the alignment of "
                   "header\n");
            int mod = size % alignof(_Header);
            if (mod) {
                size += alignof(_Header) - mod;
            }
        }

        assert(align % alignof(_Header) == 0);

        after_header = (char *)(_next_header + 1);
        aligned_start = (char *)memory::align_forward(after_header, align);
        if ((char *)aligned_start + size >
            (char *)_mem + _backing.allocated_size(_mem)) {
            assert(false && "Backing allocator ran out of memory");
            return (void *)0;
        }

        pad_size = (uint32_t)((char *)aligned_start - (char *)after_header);

        // wasted
        _wasted += sizeof(_Header) + pad_size;

        for (uint32_t *p = (uint32_t *)after_header;
             p != (uint32_t *)aligned_start; ++p) {
            p[0] = HEADER_PAD_VALUE;
        }

        _next_header->size = size;
        _total_allocated += _next_header->size + pad_size;
        _top_header = _next_header;
        _next_header = (_Header *)((char *)_top_header + sizeof(_Header) +
                                   pad_size + _top_header->size);

        return (void *)aligned_start;
    }

    /// Do not use deallocate for this allocator. Use the destructor.
    /// To free all memory
    void deallocate(void * /*unused*/) override {
        assert(false && "You must not call this");
    }

    uint32_t allocated_size(void *p) override {
        uint32_t *pad = (uint32_t *)p;
        while (pad[-1] == HEADER_PAD_VALUE) {
            --pad;
        }
        --pad;
        return ((Header *)pad)->size;
    }

    uint32_t total_allocated() override { return _total_allocated; }

    /// Destructor
    ~ArenaAllocator() {
        if (_mem)
            _backing.deallocate(_mem);
        _mem = 0;
        _top_header = (_Header *)0;
    }
};

struct MemoryGlobals {
    static const int ALLOCATOR_MEMORY = sizeof(MallocAllocator) +
                                        sizeof(ScratchAllocator) +
                                        sizeof(ArenaAllocator);

    char buffer[ALLOCATOR_MEMORY];

    MallocAllocator *default_allocator;
    ScratchAllocator *default_scratch_allocator;
    ArenaAllocator *default_arena_allocator;

    constexpr MemoryGlobals()
        : buffer{0}, default_allocator(0), default_scratch_allocator(0),
          default_arena_allocator(0) {}
};

MemoryGlobals _memory_globals;
} // anon namespace

namespace foundation {
namespace memory_globals {
void init(uint32_t temporary_memory) {
    // Create some default allocators
    char *p = _memory_globals.buffer;
    _memory_globals.default_allocator = new (p) MallocAllocator();

    p += sizeof(MallocAllocator);
    _memory_globals.default_scratch_allocator = new (p)
        ScratchAllocator(*_memory_globals.default_allocator, temporary_memory);

    p += sizeof(ScratchAllocator);
    _memory_globals.default_arena_allocator = new (p)
        ArenaAllocator(*_memory_globals.default_allocator, 4 * 1024 * 1024);
}

Allocator &default_allocator() { return *_memory_globals.default_allocator; }

Allocator &default_scratch_allocator() {
    return *_memory_globals.default_scratch_allocator;
}

Allocator &default_arena_allocator() {
    return *_memory_globals.default_arena_allocator;
}

void shutdown() {
    _memory_globals.default_arena_allocator->~ArenaAllocator();
    _memory_globals.default_scratch_allocator->~ScratchAllocator();
    _memory_globals.default_allocator->~MallocAllocator();
    _memory_globals = MemoryGlobals();
}
} //namespace memory_globals
} //namespace foundation

