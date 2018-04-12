#include <scaffold/memory.h>

namespace fo {

class OneTimeAllocator : public Allocator {
  public:
    OneTimeAllocator();
    virtual ~OneTimeAllocator();

    /// Allocates memory using mmap. Will fail if called more than once.
    virtual void *allocate(uint64_t size, uint64_t align = Allocator::DEFAULT_ALIGN) override;

    /// Deallocates memory. Will fail if called while the chunk is not allocated.
    virtual void deallocate(void *p) override;

    /// Returns the total amount of memory allocated (not just the total number of currently allocated nodes).
    virtual uint64_t total_allocated() override;

    /// Always returns `_node_size`. Doesn't check if `p` is valid.
    virtual uint64_t allocated_size(void *p) override;

  private:
    uint64_t _total_allocated;
    void *_mem;
};

} // namespace fo
