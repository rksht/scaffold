#pragma once

#include <limits>
#include <scaffold/array.h>
#include <scaffold/memory.h>
#include <scaffold/debug.h>

namespace fo {

struct ArenaInfo {
    AddrUint buffer_size;
    AddrUint total_allocated;
};

// Implementation of an ArenaAllocator that only allocates from a buffer, and does not deallocate. Allocations
// must be aligned to at least 16 bytes.
/// An ArenaAllocator allocates a buffer and bumps a pointer on each allocation. It does not support
/// deallocating allocations individually. Instead it will deallocate all the memory it owns when it's
/// destroyed.
class SCAFFOLD_API ArenaAllocator : public Allocator {
  private:
    Allocator *_backing = nullptr;
    u8 *_mem = nullptr;

    // In case the current buffer is exhausted, we can allow the allocator to
    ArenaAllocator *_child = nullptr;

    AddrUint _buffer_size = 0;
    AddrUint _top = 0;
    bool _full = false;

  private:
    // Constructs an unitialized allocator stored at the head of the buffer.
    ArenaAllocator() = default;

    void *allocate_from_child(AddrUint size, AddrUint align, u32 info);

    u8 *end() const { return _mem + _buffer_size; }

    void set_full() {
        if (!_full) {
            log_info("ArenaAllocator - %s full. Allocating child", name());
        }
        _full = true;
    }

    bool is_initialized() const { return _mem != nullptr; }

  public:
    ArenaAllocator(Allocator &backing, AddrUint buffer_size);

    ~ArenaAllocator();

    void *allocate(AddrUint size, AddrUint align = DEFAULT_ALIGN) override {
        return this->allocate_with_info(size, align, 0);
    }

    void *allocate_with_info(AddrUint size, AddrUint align = DEFAULT_ALIGN, u32 info = 0) override;

    void deallocate(void *p) override;

    uint64_t allocated_size(void *p) override;

    uint64_t total_allocated() override;

    void get_chain_info(fo::Array<ArenaInfo> &a);
};

} // namespace fo