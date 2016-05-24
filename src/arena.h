#pragma once

#include "memory.h"
#include <cassert>

namespace foundation {

/// An ArenaAllocator allocates a buffer and bumps a pointer on each allocation.
class ArenaAllocator : public Allocator {
  private:
    struct _Header {
        uint32_t size;
    };

  private:
    static constexpr uint32_t HEADER_PAD_VALUE = 0xffffffffu;

  private:
    Allocator *_backing;
    void *_mem;
    _Header *_top_header;
    _Header *_next_header;
    uint32_t _total_allocated;
    uint32_t _wasted;

  private:
    // Returns a size that is a multiple of alignof(_Header)
    uint32_t _aligned_size_with_padding(uint32_t size);

  public:
    ArenaAllocator(Allocator &backing, uint32_t size);

    ~ArenaAllocator();

    void *allocate(uint32_t size, uint32_t align) override;

    /// Do not use it explicitly
    void deallocate(void *p) override;

    uint32_t allocated_size(void *p) override;

    uint32_t total_allocated() override { return _total_allocated; }
};
} // namespace foundation
