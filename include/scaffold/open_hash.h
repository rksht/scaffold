/// Implements a quadratic probing based hash table for storing POD(-ish) objects
#pragma once

#include <scaffold/array.h>
#include <scaffold/collection_types.h>
#include <scaffold/const_log.h>
#include <scaffold/debug.h>
#include <scaffold/memory.h>

#include <assert.h>
#include <functional>
#include <type_traits>

namespace fo {
/// An open-addressed hash table demands two requirements of the type of the keys it will hold. First, there
/// must be a value that is used to denote a 'nil' key, and second, there must be a value that denotes a
/// 'deleted' key. No valid key will ever have either of these two values. Therefore, for a key type K,
/// specialize these two templates so that get() returns an appropriate value. For example, if you store
/// pointers returned by an allocator, they are usually aligned to 16 bytes, so that leaves 4 bits in the tail
/// always set to 0, so we can use lsb = 01 and 10 to denote the nil and deleted keys.
template <typename K> struct OpenNil { static K get(); };
template <typename K> struct OpenDeleted { static K get(); };

/// This template simply collects the two above types for convenience.
template <typename K> struct QuadDefault {
    using OpenNilTy = OpenNil<K>;
    using OpenDeletedTy = OpenNil<K>;
};
} // namespace fo

namespace fo {

template <typename K, typename V, typename Params = QuadDefault<K>> struct OpenHash {
    static_assert(std::is_trivially_destructible<K>::value, "");
    static_assert(std::is_trivially_destructible<V>::value, "");
    static_assert(std::is_default_constructible<K>::value, "");
    static_assert(std::is_default_constructible<V>::value, "");
    static_assert(std::is_trivially_copy_assignable<K>::value, "");
    static_assert(std::is_trivially_copy_assignable<V>::value, "");
    static_assert(alignof(V) >= alignof(uint32_t), "Need this");

    using KeyType = K;
    using ValueType = V;
    using ParamsType = Params;

    /// Type of hash function
    using HashFn = std::function<uint32_t(const K &)>;
    using EqualFn = std::function<bool(const K &, const K &)>;

    uint32_t _num_valid;     // Number of valid entries
    uint32_t _num_deleted;   // Number of deleted entries
    uint32_t _num_slots;     // Number of slots (valid, deleted, nil)
    HashFn _hash_fn;         // Hash function
    EqualFn _equal_fn;       // Equal comparison
    uint8_t *_buffer;        // Pointer to the buffer where we keep the keys and value arrays
    uint32_t _keys_offset;   // Offset of keys array
    uint32_t _values_offset; // Offset of values array
    Allocator *_allocator;   // Buffer allocator

    /// Creates a hash table able to hold `initial_size` number of elements
    /// without rehashing.
    OpenHash(Allocator &allocator, uint32_t initial_size, HashFn hash_fn, EqualFn equal_fn);
    /// Copy-constructs
    OpenHash(const OpenHash &other);
    /// Move-constructs. The other table becomes invalid for all operations.
    OpenHash(OpenHash &&other);
    /// Copy assigns
    OpenHash &operator=(const OpenHash &other);
    /// Move assigns. The other table becomes invalid for all operations.
    OpenHash &operator=(OpenHash &&other);
    ~OpenHash();
};
} // namespace fo

namespace fo {
namespace open_hash {
/// Denotes that key is not found
constexpr uint32_t NOT_FOUND = 0xffffffffu;

/// Returns NOT_FOUND if given `key` is not associated with any value. Otherwise returns an integer i such
/// that calling `value` will return a reference to the value.
template <typename K, typename V, typename Params>
uint32_t find(const OpenHash<K, V, Params> &h, const K &key);

/// Returns the value at the given index
template <typename K, typename V, typename Params> V &value(OpenHash<K, V, Params> &h, uint32_t index);

/// Returns the value at the given index (const reference)
template <typename K, typename V, typename Params>
const V &value(const OpenHash<K, V, Params> &h, uint32_t index);

/// Returns reference to value associated with the given key. If given key doesn't exist, inserts it and
/// default constructs a value first.
template <typename K, typename V, typename Params>
const V &value_default(const OpenHash<K, V, Params> &h, const K &key);

/// Same as above. Returns non-const reference
template <typename K, typename V, typename Params> V &value_default(OpenHash<K, V, Params> &h, const K &key);

/// Returns the value associated with the given key. Key must exist.
template <typename K, typename V, typename Params>
const V &must_value(const OpenHash<K, V, Params> &h, const K &key);

/// Returns the value associated with the given key (const reference). Key must exist.
template <typename K, typename V, typename Params>
const V &must_value(const OpenHash<K, V, Params> &h, const K &key);

/// Associates the given value with the given key. May trigger a rehash if key doesn't exist already. Returns
/// the position of the value.
template <typename K, typename V, typename Params>
void set(OpenHash<K, V, Params> &h, const K &key, const V &value);

/// Inserts the given key but does not take any value to associate with the key. Returns the index into the
/// values array. (You must create some value there yourself!)
template <typename K, typename V, typename Params>
uint32_t insert_key(OpenHash<K, V, Params> &h, const K &key);

/// Removes the key if it exists
template <typename K, typename V, typename Params> void remove(OpenHash<K, V, Params> &h, const K &key);

/// Flaky iterator support. A little too convoluted for my likes.
template <typename K, typename V, typename Params, bool is_const> struct Iterator {
    using KeyType = K;
    using ValueType = typename std::conditional<is_const, V, const V>::type;
    using ParamsType = Params;
    using OpenHashType =
        typename std::conditional<is_const, const OpenHash<K, V, Params>, OpenHash<K, V, Params>>::type;

    OpenHashType *_h;
    uint32_t _slot; // Either points to `end` or a valid slot. Never points to a deleted or nil slot.

    Iterator() = delete;

    Iterator(OpenHashType &h, uint32_t slot)
        : _h(&h)
        , _slot(slot) {

        _slot = std::min(_h->_num_slots, _slot);

        auto keys = _keys_array();
        auto values = _values_array();

        // Must start with a valid slot if it exists
        while (_slot != _h->_num_slots) {
            if (ParamsType::OpenDeletedTy::get() == keys[_slot] ||
                ParamsType::OpenNilTy::get() == keys[_slot]) {
                ++_slot;
            } else {
                break;
            }
        }
    }

    Iterator(const Iterator &other)
        : _h(other._h)
        , _slot(other._slot) {}

    KeyType *_keys_array() const { return reinterpret_cast<KeyType *>(_h->_buffer + _h->_keys_offset); }

    ValueType *_values_array() const {
        return reinterpret_cast<ValueType *>(_h->_buffer + _h->_values_offset);
    }

    Iterator &operator++() {
        auto keys = _keys_array();
        auto values = _values_array();

        while (true) {
            ++_slot;

            // Reached end
            if (_slot == _h->_num_slots) {
                break;
            }

            // A deleted or nil slot
            if (_slot == _h->_num_slots || ParamsType::OpenDeletedTy::get() == keys[_slot] ||
                ParamsType::OpenNilTy::get() == keys[_slot]) {
                continue;
            }

            // Valid slot means this is it
            break;
        }
        return *this;
    }

    // Actually should be called "dereferenceable", but that's a long name
    struct Derefable {
        KeyType &key;
        ValueType &value;

        Derefable(const Derefable &) = default;
        Derefable(Derefable &&) = default;

        Derefable operator=(const Derefable &) = delete;
        Derefable operator=(Derefable &&) = delete;
    };

    Derefable operator*() const { return Derefable{_keys_array()[_slot], _values_array()[_slot]}; }
};

} // namespace open_hash

// Iterator returns

template <typename K, typename V, typename Params>
open_hash::Iterator<K, V, Params, true> begin(const OpenHash<K, V, Params> &h) {
    return open_hash::Iterator<K, V, Params, true>(h, 0);
}

template <typename K, typename V, typename Params>
open_hash::Iterator<K, V, Params, true> end(const OpenHash<K, V, Params> &h) {
    return open_hash::Iterator<K, V, Params, true>(h, h._num_slots);
}

template <typename K, typename V, typename Params>
open_hash::Iterator<K, V, Params, false> begin(OpenHash<K, V, Params> &h) {
    return open_hash::Iterator<K, V, Params, false>(h, 0);
}

template <typename K, typename V, typename Params>
open_hash::Iterator<K, V, Params, false> end(OpenHash<K, V, Params> &h) {
    return open_hash::Iterator<K, V, Params, false>(h, h._num_slots);
}

// Comparisons. Not doing all of them. Will add if I ever need others.

template <typename K, typename V, typename Params, bool is_const>
bool operator<(const open_hash::Iterator<K, V, Params, is_const> &it_0,
               const open_hash::Iterator<K, V, Params, is_const> &it_1) {
    return it_0._slot < it_1._slot;
}

template <typename K, typename V, typename Params, bool is_const>
bool operator==(const open_hash::Iterator<K, V, Params, is_const> &it_0,
                const open_hash::Iterator<K, V, Params, is_const> &it_1) {
    return it_0._slot == it_1._slot;
}

template <typename K, typename V, typename Params, bool is_const>
bool operator!=(const open_hash::Iterator<K, V, Params, is_const> &it_0,
                const open_hash::Iterator<K, V, Params, is_const> &it_1) {
    return it_0._slot != it_1._slot;
}

template <typename K, typename V, typename Params, bool is_const>
bool operator>(const open_hash::Iterator<K, V, Params, is_const> &it_0,
               const open_hash::Iterator<K, V, Params, is_const> &it_1) {
    return it_1 < it_0;
}

} // namespace fo

// --- Implementations

namespace fo {
namespace open_hash {
namespace internal {

// Calculates key and value array offsets
template <typename K, typename V, typename Params>
uint32_t allocate_buffer(OpenHash<K, V, Params> *q, uint32_t num_slots);

// Destroys a hash table
template <typename K, typename V, typename Params> void destroy(OpenHash<K, V, Params> *q, bool moved_from);

// Rehashes
template <typename K, typename V, typename Params> void rehash_if_needed(OpenHash<K, V, Params> &h);
} // namespace internal
} // namespace open_hash
} // namespace fo

namespace fo {
template <typename K, typename V, typename Params>
OpenHash<K, V, Params>::OpenHash(Allocator &allocator, uint32_t initial_size,
                                 typename OpenHash<K, V, Params>::HashFn hash_fn,
                                 typename OpenHash<K, V, Params>::EqualFn equal_fn)
    : _num_valid(0)
    , _num_deleted(0)
    , _num_slots(0)
    , _hash_fn(hash_fn)
    , _equal_fn(equal_fn) {
    _allocator = &allocator;

    _num_slots = clip_to_power_of_2(initial_size);
    open_hash::internal::allocate_buffer(this, _num_slots);

    K *keys = (K *)(_buffer + _keys_offset);

    using nil_ty = typename Params::OpenNilTy;
    for (uint32_t i = 0; i < _num_slots; ++i) {
        keys[i] = nil_ty::get();
    }
}
template <typename K, typename V, typename Params>
OpenHash<K, V, Params>::OpenHash(const OpenHash &other)
    : _num_valid(other._num_valid)
    , _num_deleted(other._num_deleted)
    , _num_slots(other._num_slots)
    , _hash_fn(other._hash_fn)
    , _equal_fn(other._equal_fn) {
    _allocator = other._allocator;
    uint32_t buffer_size = open_hash::internal::allocate_buffer(this, _num_slots);
    memcpy(_buffer, other._buffer, buffer_size);
}
template <typename K, typename V, typename Params>
OpenHash<K, V, Params>::OpenHash(OpenHash<K, V, Params> &&other)
    : _num_valid(other._num_valid)
    , _num_deleted(other._num_deleted)
    , _num_slots(other._num_slots)
    , _hash_fn(other._hash_fn)
    , _equal_fn(other._equal_fn)
    , _buffer(other._buffer)
    , _keys_offset(other._keys_offset)
    , _values_offset(other._values_offset)
    , _allocator(other._allocator) {
    open_hash::internal::destroy(&other, true);
}

template <typename K, typename V, typename Params>
OpenHash<K, V, Params> &OpenHash<K, V, Params>::operator=(OpenHash &&other) {
    if (this != &other) {
        if (_allocator != nullptr) {
            _allocator->deallocate(_buffer);
        }
        _num_valid = other._num_valid;
        _num_deleted = other._num_deleted;
        _num_slots = other._num_slots;
        _hash_fn = other._hash_fn;
        _equal_fn = other._equal_fn;
        _buffer = other._buffer;
        _keys_offset = other._keys_offset;
        _values_offset = other._values_offset;
        _allocator = other._allocator;
        open_hash::internal::destroy(&other, true);
    }
    return *this;
}

template <typename K, typename V, typename Params>
OpenHash<K, V, Params> &OpenHash<K, V, Params>::operator=(const OpenHash &other) {
    if (this != &other) {
        if (_allocator != nullptr) {
            _allocator->deallocate(_buffer);
        }
        _num_valid = other._num_valid;
        _num_deleted = other._num_deleted;
        _num_slots = other._num_slots;
        _hash_fn = other._hash_fn;
        _equal_fn = other._equal_fn;
        _allocator = other._allocator;
        uint32_t buffer_size = open_hash::internal::allocate_buffer(this, _num_slots);
        memcpy(_buffer, other._buffer, buffer_size);
    }
    return *this;
}

template <typename K, typename V, typename Params> OpenHash<K, V, Params>::~OpenHash() {
    open_hash::internal::destroy(this, false);
}

} // namespace fo

namespace fo {
namespace open_hash {
namespace internal {

template <typename K, typename V, typename Params>
uint32_t allocate_buffer(OpenHash<K, V, Params> *q, uint32_t num_slots) {
    // Using the one with the stricter alignment. Larger one is kept first in the buffer.
    constexpr uint16_t buffer_align = alignof(K) > alignof(V) ? alignof(K) : alignof(V);
    if (buffer_align == alignof(K)) {
        q->_keys_offset = 0;
        q->_values_offset = sizeof(K) * num_slots;
    } else {
        q->_values_offset = 0;
        q->_keys_offset = sizeof(V) * num_slots;
    }
    uint32_t buffer_size = sizeof(K) * num_slots + sizeof(V) * num_slots;
    q->_buffer = (uint8_t *)q->_allocator->allocate(buffer_size, buffer_align);
    return buffer_size;
}

template <typename K, typename V, typename Params> void destroy(OpenHash<K, V, Params> *q, bool moved_from) {
    // allocator non null denotes valid object
    if (q->_allocator) {
        // Only deallocate if this was not moved from
        if (!moved_from) {
            q->_allocator->deallocate(q->_buffer);
        }
        q->_allocator = nullptr;
        q->_buffer = nullptr;
        q->_num_deleted = 0;
        q->_num_valid = 0;
        q->_num_slots = 0;
        q->_keys_offset = 0;
        q->_values_offset = 0;
    }
}

template <typename K, typename V, typename Params> void rehash_if_needed(OpenHash<K, V, Params> &h) {
    using nil_ty = typename Params::OpenNilTy;
    using deleted_ty = typename Params::OpenDeletedTy;

    static constexpr float max_load_factor = 0.5;
    // const uint32_t array_size = array::size(h._keys);

    const float real_load_factor = (h._num_valid + h._num_deleted) / float(h._num_slots);
    const float valid_load_factor = h._num_valid / float(h._num_slots);

    if (real_load_factor < max_load_factor) {
        return;
    }

    // Check and see if we do not actually need to double the size, since only valid entries will be entered
    uint32_t new_size = valid_load_factor >= max_load_factor ? h._num_slots * 2 : h._num_slots;

    OpenHash<K, V, Params> new_h{*h._allocator, new_size, h._hash_fn, h._equal_fn};

    K *keys = (K *)(h._buffer + h._keys_offset);
    V *values = (V *)(h._buffer + h._values_offset);

    for (uint32_t i = 0; i < h._num_slots; ++i) {
        const K &key = keys[i];
        if (!h._equal_fn(key, nil_ty::get()) && !h._equal_fn(key, deleted_ty::get())) {
            open_hash::set(new_h, key, values[i]);
        }
    }
    std::swap(h, new_h);
}

} // namespace internal
} // namespace open_hash
} // namespace fo

namespace fo {
namespace open_hash {

template <typename K, typename V, typename Params>
uint32_t find(const OpenHash<K, V, Params> &h, const K &key) {
    using nil_ty = typename Params::OpenNilTy;

    uint32_t idx = h._hash_fn(key);

    K *keys = (K *)(h._buffer + h._keys_offset);
    // V *values = (V *)(h._buffer + h._values_offset);

    for (uint32_t i = 0; i < h._num_slots; ++i) {
        idx = (idx + i) % h._num_slots;
        if (h._equal_fn(keys[idx], key)) {
            return idx;
        }
        if (h._equal_fn(keys[idx], nil_ty::get())) {
            return NOT_FOUND;
        }
    }
    return NOT_FOUND;
}

/// Returns the value at the given index
template <typename K, typename V, typename Params> V &value(OpenHash<K, V, Params> &h, uint32_t index) {
    assert(index != NOT_FOUND);
    V *values = (V *)(h._buffer + h._values_offset);
    return values[index];
}

template <typename K, typename V, typename Params> V &value(const OpenHash<K, V, Params> &h, uint32_t index) {
    assert(index != NOT_FOUND);
    const V *values = (const V *)(h._buffer + h._values_offset);
    return values[index];
}

template <typename K, typename V, typename Params> V &value_default(OpenHash<K, V, Params> &h, const K &key) {
    uint32_t index = find(h, key);

    V *values;

    if (index == NOT_FOUND) {
        index = insert_key(h, key);
        values = (V *)(h._buffer + h._values_offset);
        new (&values[index]) V; // Default ctor
    } else {
        values = (V *)(h._buffer + h._values_offset);
    }

    return values[index];
}

template <typename K, typename V, typename Params>
const V &value_default(const OpenHash<K, V, Params> &h, const K &key) {
    return value_default(const_cast<OpenHash<K, V, Params> &>(h), key);
}

template <typename K, typename V, typename Params> V &must_value(OpenHash<K, V, Params> &h, const K &key) {
    uint32_t i = find(h, key);
    assert(i != NOT_FOUND);
    return value(h, i);
}

template <typename K, typename V, typename Params>
const V &must_value(const OpenHash<K, V, Params> &h, const K &key) {
    uint32_t i = find(h, key);
    assert(i != NOT_FOUND);
    return value(h, i);
}

template <typename K, typename V, typename Params>
void set(OpenHash<K, V, Params> &h, const K &key, const V &value) {
    using nil_ty = typename Params::OpenNilTy;

    internal::rehash_if_needed(h);

    uint64_t idx = h._hash_fn(key);

    K *keys = (K *)(h._buffer + h._keys_offset);
    V *values = (V *)(h._buffer + h._values_offset);

    for (uint32_t i = 0; i < h._num_slots; ++i) {
        idx = (idx + i) % h._num_slots;
        if (h._equal_fn(keys[idx], nil_ty::get()) || h._equal_fn(keys[idx], key)) {
            keys[idx] = key;
            values[idx] = value;
            ++h._num_valid;
            return;
        }
    }

    log_assert(0, "Impossible");
}

template <typename K, typename V, typename Params>
uint32_t insert_key(OpenHash<K, V, Params> &h, const K &key) {
    // Implementation is essentially the same as `set`. Just returning the
    // index where we inserted.
    using nil_ty = typename Params::OpenNilTy;

    internal::rehash_if_needed(h);

    uint64_t idx = h._hash_fn(key);

    K *keys = (K *)(h._buffer + h._keys_offset);
    // V *values = (V *)(h._buffer + h._values_offset);

    for (uint32_t i = 0; i < h._num_slots; ++i) {
        idx = (idx + i) % h._num_slots;
        if (h._equal_fn(keys[idx], nil_ty::get()) || h._equal_fn(keys[idx], key)) {
            keys[idx] = key;
            ++h._num_valid;
            return idx;
        }
    }

    log_assert(0, "Impossible");
    return NOT_FOUND;
}

template <typename K, typename V, typename Params> void remove(OpenHash<K, V, Params> &h, const K &key) {
    using deleted_ty = typename Params::OpenDeletedTy;

    K *keys = (K *)(h._buffer + h._keys_offset);

    uint32_t idx = find(h, key);
    if (idx != NOT_FOUND) {
        keys[idx] = deleted_ty::get();
        ++h._num_deleted;
    }
}

} // namespace open_hash
} // namespace fo
