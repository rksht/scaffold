#include <scaffold/memory.h>
#include <scaffold/types.h>

namespace fo {

template <typename T> struct Vector {
    T *_data;
    u32 _size;
    u32 _capacity;
    fo::Allocator *_allocator;

    Vector(u32 initial_count = 0, fo::Allocator &a = fo::memory_globals::default_allocator());

    Vector(u32 initial_count,
           const T &fill_element,
           fo::Allocator &a = fo::memory_globals::default_allocator());

    Vector(const Vector &);
    Vector(Vector &&);

    ~Vector();

    Vector &operator=(const Vector &);
    Vector &operator=(Vector &&);

    inline T &operator[](i32 n) { return _data[n]; }
    inline const T &operator[](i32 n) const { return _data[n]; }

    T *begin() { return _data; }
    T *end() { return _data + _size; }
    const T *begin() const { return _data; }
    const T *end() const { return _data + _size; }

    const T *cbegin() { return _data; }
    const T *cend() { return _data; }

    const T *cbegin() const { return _data; }
    const T *cend() const { return _data; }
};

} // namespace fo
