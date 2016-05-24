#include "arena.h"
#include "debug.h"
#include <string.h>

namespace foundation {
uint32_t ArenaAllocator::_aligned_size_with_padding(uint32_t size) {
    uint32_t total = size + sizeof(_Header);
    int mod = total % alignof(_Header);
    if (mod) {
        total += alignof(_Header) - mod;
    }
    return total;
}

ArenaAllocator::ArenaAllocator(Allocator &backing, uint32_t size)
    : _backing(&backing) {
    uint32_t adjusted_size = _aligned_size_with_padding(size);
    _mem = _backing->allocate(adjusted_size, alignof(_Header));
    memset(_mem, 0, adjusted_size);
    _top_header = (_Header *)_mem;
    _top_header->size = 0;
    _next_header = _top_header;
    _total_allocated = 0;
}

void *ArenaAllocator::allocate(uint32_t size, uint32_t align) {
    char *after_header, *aligned_start;
    uint32_t pad_size;

    if (size % alignof(_Header) != 0) {
        log_err("ArenaAllocator - size not multiple of header\n");
        int mod = size % alignof(_Header);
        if (mod) {
            size += alignof(_Header) - mod;
        }
    }

    assert(align % alignof(_Header) == 0);

    after_header = (char *)(_next_header + 1);
    aligned_start = (char *)memory::align_forward(after_header, align);

    // Would go past last byte?
    if (aligned_start + size > (char *)_mem + _backing->allocated_size(_mem)) {
        log_err("ArenaAllocator(%s) exhausted, using fallback allocator",
                name());
    }

    pad_size = (uint32_t)(aligned_start - after_header);

    // wasted
    _wasted += sizeof(_Header) + pad_size;

    for (uint32_t *p = (uint32_t *)after_header; p != (uint32_t *)aligned_start;
         ++p) {
        p[0] = HEADER_PAD_VALUE;
    }

    _next_header->size = size;
    _total_allocated += _next_header->size + pad_size;
    _top_header = _next_header;
    _next_header = (_Header *)((char *)_top_header + sizeof(_Header) +
                               pad_size + _top_header->size);

    return (void *)aligned_start;
}

/// Do not use it explicitly
void ArenaAllocator::deallocate(void *p) {
    (void)p;
    assert(false && "You must not call this");
}

uint32_t ArenaAllocator::allocated_size(void *p) {
    uint32_t *pad = (uint32_t *)p;
    while (pad[-1] == HEADER_PAD_VALUE) {
        --pad;
    }
    --pad;
    return ((_Header *)pad)->size;
}

/// Destructor
ArenaAllocator::~ArenaAllocator() {
    if (_mem)
        _backing->deallocate(_mem);
    _mem = 0;
    _top_header = (_Header *)0;
}
}
