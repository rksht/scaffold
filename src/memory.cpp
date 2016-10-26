#include <scaffold/debug.h>
#include <scaffold/memory.h>

// For posix_memalign
#ifdef MALLOC_ALLOC_DONT_TRACK_SIZE
#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 200112L)
#define _POSIX_C_SOURCE 200112L
#endif
#endif

#include <assert.h>
#include <iostream>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace foundation {
void Allocator::set_name(const char *name, uint64_t len) {
    assert(len < ALLOCATOR_NAME_SIZE && "Allocator name too large");
    memcpy(_name, name, len);
}
}

namespace {

using namespace foundation;

// Header stored at the beginning of a memory allocation to indicate the
// size of the allocated data.
struct Header {
    uint64_t size;
};

// If we need to align the memory allocation we pad the header with this
// value after storing the size. That way we can
const uint64_t HEADER_PAD_VALUE = ~uint64_t(0);

// Given a pointer to the header, returns a pointer to the data that follows it.
inline void *data_pointer(Header *header, uint64_t align) {
    void *p = header + 1;
    return memory::align_forward(p, align);
}

// Given a pointer to the data, returns a pointer to the header before it.
inline Header *header(void *data) {
    uint64_t *p = (uint64_t *)data;
    while (p[-1] == HEADER_PAD_VALUE)
        --p;
    return (Header *)p - 1;
}

// Stores the size in the header and pads with HEADER_PAD_VALUE up to the
// data pointer.
inline void fill(Header *header, void *data, uint64_t size) {
    header->size = size;
    uint64_t *p = (uint64_t *)(header + 1);
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
    uint64_t _total_allocated;

    // Returns the size to allocate from malloc() for a given size and align.
    static inline uint64_t size_with_padding(uint64_t size, uint64_t align) {
        return size + align + sizeof(Header);
    }

  public:
    MallocAllocator() : _total_allocated(0) {
#ifdef MALLOC_ALLOC_DONT_TRACK_SIZE
        _total_allocated = SIZE_NOT_TRACKED;
#endif
    }

    ~MallocAllocator() {
// Check that we don't have any memory leaks when allocator is
// destroyed.
#ifndef MALLOC_ALLOC_DONT_TRACK_SIZE
        assert(_total_allocated == 0);
#endif
    }

#ifdef MALLOC_ALLOC_DONT_TRACK_SIZE
    virtual void *allocate(uint64_t size, uint64_t align) override {
        int ret;
        void *p = nullptr;

        // The minimum alignment required for posix_memalign
        if (align % alignof(void *) != 0) {
            align = alignof(void *);
        }

        if ((ret = posix_memalign(&p, align, size)) != 0) {
            log_err(
                "MallocAllocator failed to allocate - error = %s, align = %lu",
                ret == EINVAL ? "EINVAL" : "ENOMEM", align);
            abort();
        }
        return p;
    }

    virtual void deallocate(void *p) override { free(p); }

    virtual uint64_t allocated_size(void *p) {
        (void)p;
        return SIZE_NOT_TRACKED;
    }
#else
    virtual void *allocate(uint64_t size, uint64_t align) override {
        const uint64_t ts = size_with_padding(size, align);
        Header *h = (Header *)malloc(ts);
        void *p = data_pointer(h, align);
        fill(h, p, ts);
        _total_allocated += ts;
        assert((uintptr_t)p % align == 0);
        return p;
    }

    virtual void deallocate(void *p) override {
        if (!p)
            return;

        Header *h = header(p);
        _total_allocated -= h->size;
        free(h);
    }

    virtual uint64_t allocated_size(void *p) override {
        return header(p)->size;
    }
#endif

    virtual uint64_t total_allocated() override { return _total_allocated; }
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
    ScratchAllocator(Allocator &backing, uint64_t size) : _backing(backing) {
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

    virtual void *allocate(uint64_t size, uint64_t align) {
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

    virtual uint64_t allocated_size(void *p) {
        Header *h = header(p);
        return h->size - ((char *)p - (char *)h);
    }

    virtual uint64_t total_allocated() { return _end - _begin; }
};

/// This struct should contain all allocators required by the
/// application/library/deathray etc. Put any extra allocators required inside
/// this struct.
struct MemoryGlobals {
    alignas(MallocAllocator) char _default_allocator[sizeof(MallocAllocator)];
    alignas(ScratchAllocator) char _default_scratch_allocator[sizeof(
        ScratchAllocator)];

    // Don't like to have strict aliasing related warnings. I can spare a few
    // bytes to point to the above two chunks
    MallocAllocator *default_allocator;
    ScratchAllocator *default_scratch_allocator;
};

MemoryGlobals _memory_globals;
} // anon namespace

namespace foundation {

namespace memory_globals {

/// Allocator names should be defined here statically...
static const char default_allocator_name[] = "default_alloc";
static const char default_scratch_allocator_name[] = "default_scratch_alloc";
// static const char default_arena_allocator_name[] = "Default arena allocator";

/// ... And add the initialization code here ...
void init(uint64_t scratch_buffer_size) {
#ifdef MALLOC_ALLOC_DONT_TRACK_SIZE
    log_info("MallocAllocator is NOT tracking size");
#else
    log_info("MallocAllocator is tracking size");
#endif

#ifndef BUDDY_ALLOC_LEVEL_LOGGING
    log_info("BuddyAllocator will not log level state");
#else
    log_info("BuddyAllocator WILL log level state");
#endif
    _memory_globals.default_allocator =
        new (_memory_globals._default_allocator) MallocAllocator{};
    _memory_globals.default_scratch_allocator =
        new (_memory_globals._default_scratch_allocator) ScratchAllocator{
            *(MallocAllocator *)_memory_globals.default_allocator,
            scratch_buffer_size};

    default_allocator().set_name(default_allocator_name,
                                 sizeof(default_allocator_name));
    default_scratch_allocator().set_name(
        default_scratch_allocator_name, sizeof(default_scratch_allocator_name));
}

Allocator &default_allocator() { return *_memory_globals.default_allocator; }

Allocator &default_scratch_allocator() {
    return *_memory_globals.default_scratch_allocator;
}

/// ... And add deallocation code here.
void shutdown() {
    _memory_globals.default_scratch_allocator->~ScratchAllocator();
    // MallocAllocator must be last as its used as the backing allocator for
    // others
    _memory_globals.default_allocator->~MallocAllocator();
    _memory_globals = MemoryGlobals();
}
} // namespace memory_globals
} // namespace foundation
