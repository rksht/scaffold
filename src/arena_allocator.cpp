#include <scaffold/arena_allocator.h>
#include <scaffold/const_log.h>

namespace fo {

namespace arena_internal {

// This header is only used for supporting `allocated_size` method and debugging purposes. Contains the size
// of the allocated region and a
struct AllocationHeader {
    AddrUint user_info;
    AddrUint size;

    static constexpr AddrUint PADDING = std::numeric_limits<AddrUint>::max();
};

static_assert(sizeof(AllocationHeader) == 2 * sizeof(AddrUint), "");

}; // namespace arena_internal

using namespace arena_internal;

static inline AddrUint size_with_padding(AddrUint size, AddrUint align) {
    return sizeof(AllocationHeader) + align + size;
}

static inline void *data_pointer(AllocationHeader *header, AddrUint align) {
    AddrUint *p = reinterpret_cast<AddrUint *>(header + 1);
    return memory::align_forward(p, align);
}

static inline void fill_with_padding(void *data, AllocationHeader *header) {
    AddrUint *p = reinterpret_cast<AddrUint *>(header + 1);

    while (p < data) {
        *p = AllocationHeader::PADDING;
        ++p;
    }
}

ArenaAllocator::ArenaAllocator(Allocator &backing, AddrUint buffer_size)
    : _backing(&backing) {

    _buffer_size = buffer_size;

    // Size required to store a pointer header
    const AddrUint next_allocator_chunk_size =
        size_with_padding(sizeof(ArenaAllocator), alignof(ArenaAllocator));

    if (_buffer_size < next_allocator_chunk_size) {
        _buffer_size = next_allocator_chunk_size;

        log_warn(
            "Size of buffer managed by ArenaAllocator should be >= sizeof(ArenaAllocator) which is %zu bytes",
            sizeof(ArenaAllocator));
    }

    _mem = (u8 *)_backing->allocate_with_info(_buffer_size, alignof(AllocationHeader));

    AllocationHeader *h = reinterpret_cast<AllocationHeader *>(_mem);

    // Set the header
    {
        AllocationHeader next_allocator_header;
        next_allocator_header.size = sizeof(ArenaAllocator);
        next_allocator_header.user_info = 0;
        *h = next_allocator_header;
    }

    // Advance to the data pointer
    {
        void *data = data_pointer(h, alignof(ArenaAllocator));
        fill_with_padding(data, h);

        // Create the uninitialized child ArenaAllocator at the head.
        new (data) ArenaAllocator();
        _child = reinterpret_cast<ArenaAllocator *>(data);

        _top = ((u8 *)data + sizeof(ArenaAllocator)) - _mem;
    }
}

void *ArenaAllocator::allocate_with_info(AddrUint size, AddrUint align, u32 info) {
    if (size == 0) {
        return nullptr;
    }

    u8 *top_mem = _mem + _top;

    top_mem = (u8 *)memory::align_forward(top_mem, alignof(AllocationHeader));

    AddrUint total_chunk_size = size_with_padding(size, align);

    if (top_mem + total_chunk_size > end()) {
        set_full();
        return allocate_from_child(size, align, info);
    }

    AllocationHeader *h = reinterpret_cast<AllocationHeader *>(top_mem);

    {
        AllocationHeader header;
        header.size = size;
        header.user_info = info;
        *h = header;
    }

    {
        void *data = data_pointer(h, align);
        fill_with_padding(data, h);

        _top = ((u8 *)data + size) - _mem;

        return data;
    }
}

void ArenaAllocator::deallocate(void *) {}

uint64_t ArenaAllocator::total_allocated() { return _buffer_size + _child ? _child->total_allocated() : 0; }

uint64_t ArenaAllocator::allocated_size(void *p) {
    u8 *p8 = (u8 *)p;

    if (p8 < _mem || p8 >= end()) {
        if (_child->is_initialized()) {
            return _child->allocated_size(p);
        } else {
            log_assert(false, "Pointer %p not in range of ArenaAllocator", p);
        }
    }

    AddrUint *pad = reinterpret_cast<AddrUint *>(p8);
    pad -= 1;
    while (*pad == AllocationHeader::PADDING) {
        --pad;
    }

    AllocationHeader *h = reinterpret_cast<AllocationHeader *>(pad - 1);
    return h->size;
}

void *ArenaAllocator::allocate_from_child(AddrUint size, AddrUint align, u32 info) {
    // Create a child allocator if there isn't one.

    if (!_child->is_initialized()) {
        AddrUint child_buffer_size_needed =
            sizeof(AllocationHeader) + alignof(ArenaAllocator) + sizeof(ArenaAllocator) + align + size;

        // If the size of allocation is more than 4 times the initial buffer size, fail.
        AddrUint multiple = ceil_div(child_buffer_size_needed, _buffer_size);

        if (multiple > 4) {
            log_warn("Arena Allocator %s of buffer size = %lu bytes requested allocation of size %lu bytes",
                     name(),
                     (u64)_buffer_size,
                     (u64)size);
        }

        child_buffer_size_needed = clip_to_pow2(child_buffer_size_needed);

        log_info("ArenaAllocator of size %.2f KB  allocating a child buffer of size %.2f KB",
                 _buffer_size / 1024.0,
                 child_buffer_size_needed / 1024.0);

        // Delete the empty child allocator and create a new ArenaAllocator. This deletion could be skipped,
        // but it's undefined behavior to overwrite an object
        _child->~ArenaAllocator();
        new (_child) ArenaAllocator(*_backing, child_buffer_size_needed);
    }

    return _child->allocate_with_info(size, align, info);
}

ArenaAllocator::~ArenaAllocator() {
    if (_child) {
        _child->~ArenaAllocator();
        _child = nullptr;
    }

    if (_mem) {
        _backing->deallocate(_mem);
        _buffer_size = 0;
        _top = 0;
        _mem = nullptr;
        _full = false;
    }
}

void ArenaAllocator::get_chain_info(fo::Array<ArenaInfo> &a) {
    ArenaInfo info;
    info.buffer_size = _buffer_size;
    info.total_allocated = _top;

    push_back(a, info);

    if (_child) {
        _child->get_chain_info(a);
    }
}

} // namespace fo
