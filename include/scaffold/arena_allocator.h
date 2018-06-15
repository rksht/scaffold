#pragma once

#include <scaffold/memory.h>

namespace fo {

/// An ArenaAllocator allocates a buffer and bumps a pointer on each allocation.
/// It does not support deallocating allocations individually. Instead it will
/// deallocate all the memory it owns when it's destroyed.
class SCAFFOLD_API ArenaAllocator : public Allocator {
  private:
    struct _Header {
        uint64_t size;
    };

  private:
    static constexpr uint64_t HEADER_PAD_VALUE = ~uint64_t(0);

  private:
    Allocator *_backing;
    void *_mem;
    _Header *_top_header;
    _Header *_next_header;
    uint64_t _total_allocated;
    uint64_t _wasted;

  private:
    // Returns a size that is a multiple of alignof(_Header)
    uint64_t _aligned_size_with_padding(uint64_t size);

  public:
    ArenaAllocator(Allocator &backing, uint64_t size);

    ~ArenaAllocator();

    void *allocate(uint64_t size, uint64_t align) override;

    /// Do not use it explicitly
    void deallocate(void *p) override;

    uint64_t allocated_size(void *p) override;

    uint64_t total_allocated() override { return _total_allocated; }
};
} // namespace fo
