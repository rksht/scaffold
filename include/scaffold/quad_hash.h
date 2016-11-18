/// Implements a quadratic probing based hash table
#pragma once

#include <scaffold/array.h>
#include <scaffold/collection_types.h>
#include <scaffold/const_log.h>
#include <scaffold/debug.h>
#include <scaffold/memory.h>

#include <type_traits> // std::conditional

namespace foundation {
/// An open-addressed hash table demands two requirements of the type of the
/// keys it will hold. First, there must be a value that is used to denote a
/// 'nil' key, and second, there must be a value that denotes a 'deleted' key.
/// No valid key will ever have either of these two values. Therefore, for a
/// key type K, specialize these two templates so that get() returns an
/// appropriate value. For example, if you store pointers returned by an
/// allocator, they are usually aligned to 16 bytes, so that leaves 4 bits in
/// the tail always set to 0, so we can use lsb = 01 and 10 to denote the nil
/// and deleted keys.
template <typename K> struct QuadNil { static K get(); };
template <typename K> struct QuadDeleted { static K get(); };

/// This template simply collects the two above types for convenience.
template <typename K> struct QuadDefault {
    using QuadNilTy = QuadNil<K>;
    using QuadDeletedTy = QuadNil<K>;
};

/// Forward declaraing QuadHash
template <typename K, typename V, typename Params = QuadDefault<K>> struct QuadHash;
} // namespace foundation

namespace foundation {
namespace quad_hash {

/// Iterator for QuadHash
template <typename K, typename V, typename Params, bool is_const = true> struct Iterator {
  private:
    using nil_ty = typename Params::QuadNilTy;
    using deleted_ty = typename Params::QuadDeletedTy;

    // Key pointer should always be to const keys
    using key_ptr = const K *;
    // Value pointer will depend
    using value_ptr = std::conditional_t<is_const, const V *, V *>;
    using quad_hash_ty = std::conditional_t<is_const, QuadHash<K, V, Params>, const QuadHash<K, V, Params>>;

    key_ptr _cur_key;
    key_ptr _key_end;
    key_ptr _key_start;
    value_ptr _cur_value;
    value_ptr _value_end;
    value_ptr _value_start;

  private:
    // Sets _cur_key to point to the next valid key. If no such key exists, it
    // will point to the end of the key array.
    inline void _next_valid();
    // Sets _cur_key to point to the previous valid key. If no such key
    // exists, it will point to the end of the key array.
    inline void _prev_valid();

  public:
    Iterator(quad_hash_ty &q);
    inline bool operator==(const Iterator &other);
    inline bool operator!=(const Iterator &other);
    /// Increments until valid key is hit
    Iterator &operator++();
    inline Iterator operator++(int);
    /// Decrements until valid key is hit
    Iterator &operator--();
    inline Iterator operator--(int);
};
} // namespace quad_hash
} // namespace foundation

namespace foundation {
/// The open-addressed hash table
template <typename K, typename V, typename Params> struct QuadHash {

    static_assert(std::is_trivially_copy_assignable<K>::value, "Must be is_trivially_copy_assignable");
    static_assert(std::is_trivially_copy_assignable<V>::value, "Must be is_trivially_copy_assignable");
    static_assert(alignof(V) >= alignof(uint32_t), "Need this");

    /// Type of hash function
    using HashFn = std::function<uint32_t(const K &)>;
    using EqualFn = std::function<bool(const K &, const K &)>;

    HashFn _hash_fn;       // Hash function
    EqualFn _equal_fn;     // Equal comparison
    Array<K> _keys;        // Array containing keys
    Array<V> _values;      // Array containing corresponding values
    uint32_t _num_valid;   // Number of valid entries
    uint32_t _num_deleted; // Number of deleted entries

    /// Creates a hash table able to hold `initial_size` number of elements
    /// without rehashing.
    QuadHash(Allocator &allocator, uint32_t initial_size, HashFn hash_fn, EqualFn equal_fn);

    /// Iterators
    using iterator = quad_hash::Iterator<K, V, Params, false>;
    using const_iterator = quad_hash::Iterator<K, V, Params, true>;
    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
};
} // namespace foundation

namespace foundation {
namespace quad_hash {
/// Denotes that key is not found
constexpr uint32_t NOT_FOUND = 0xffffffffu;

/// Returns -1 if given `key` is not associated with any value. Otherwise
/// returns an integer i such that h->_values[i] contains the associated
/// value.
template <typename K, typename V, typename Params>
uint32_t find(const QuadHash<K, V, Params> &h, const K &key);

/// Associates the given value with the given key. May trigger a rehash if key
/// doesn't exist already. Returns the position of the value.
template <typename K, typename V, typename Params>
void set(QuadHash<K, V, Params> &h, const K &key, const V &value);

/// Removes the key if it exists
template <typename K, typename V, typename Params> void remove(QuadHash<K, V, Params> &h, const K &key);

} // namespace quad_hash
} // namespace foundation

//
// ------ Implementations ------
//

namespace foundation {
namespace quad_hash {

template <typename K, typename V, typename Params, bool is_const>
void Iterator<K, V, Params, is_const>::_next_valid() {
    while (_cur_key != _key_end) {
        if (*_cur_key == nil_ty::get() || *_cur_key == deleted_ty::get()) {
            ++_cur_key;
            ++_cur_value;
        } else {
            return;
        }
    }
}

template <typename K, typename V, typename Params, bool is_const>
void Iterator<K, V, Params, is_const>::_prev_valid() {
    while (_cur_key >= _key_start) {
        if (*_cur_key == nil_ty::get() || *_cur_key == deleted_ty::get()) {
            --_cur_key;
            --_cur_value;
        }
    }
    _cur_key = _key_end;
    _cur_value = nullptr;
}

template <typename K, typename V, typename Params, bool is_const>
Iterator<K, V, Params, is_const>::Iterator(quad_hash_ty &q)
    : _cur_key{array::begin(q._keys)}
    , _key_end{array::end(q._keys)}
    , _key_start{_cur_key}
    , _cur_value{array::begin(q._values)}
    , _value_end{array::end(q._values)}
    , _value_start{_cur_value} {

    // Maintain the invariant that the iterator always points to a valid key
    if (*_cur_key == nil_ty::get() || *_cur_key == deleted_ty::get()) {
        operator++();
    }
}

template <typename K, typename V, typename Params, bool is_const>
bool Iterator<K, V, Params, is_const>::operator==(const Iterator &other) {
    return _cur_key == other._cur_key;
}

template <typename K, typename V, typename Params, bool is_const>
bool Iterator<K, V, Params, is_const>::operator!=(const Iterator<K, V, Params, is_const> &other) {
    return !(*this == other);
}
template <typename K, typename V, typename Params, bool is_const>
Iterator<K, V, Params, is_const> &Iterator<K, V, Params, is_const>::operator++() {
    _next_valid();
    return *this;
}

template <typename K, typename V, typename Params, bool is_const>
Iterator<K, V, Params, is_const> Iterator<K, V, Params, is_const>::operator++(int) {
    Iterator saved{*this};
    operator++();
    return saved;
}

template <typename K, typename V, typename Params, bool is_const>
Iterator<K, V, Params, is_const> &Iterator<K, V, Params, is_const>::operator--() {
    _prev_valid();
    return *this;
}

template <typename K, typename V, typename Params, bool is_const>
Iterator<K, V, Params, is_const> Iterator<K, V, Params, is_const>::operator--(int) {
    Iterator saved{*this};
    operator--();
    return saved;
}

} // namespace quad_hash
} // namespace foundation

namespace foundation {
template <typename K, typename V, typename Params>
QuadHash<K, V, Params>::QuadHash(Allocator &allocator, uint32_t initial_size,
                                 typename QuadHash<K, V, Params>::HashFn hash_fn,
                                 typename QuadHash<K, V, Params>::EqualFn equal_fn)
    : _hash_fn(hash_fn)
    , _equal_fn(equal_fn)
    , _keys(allocator)
    , _values(allocator)
    , _num_valid(0)
    , _num_deleted(0) {
    array::resize(_keys, clip_to_power_of_2(initial_size));
    array::resize(_values, clip_to_power_of_2(initial_size));
    using nil_ty = typename Params::QuadNilTy;
    for (auto &k : _keys) {
        k = nil_ty::get();
    }
}
} // namespace foundation

namespace foundation {
namespace quad_hash_internal {

template <typename K, typename V, typename Params> void rehash_if_needed(QuadHash<K, V, Params> &h) {
    using nil_ty = typename Params::QuadNilTy;
    using deleted_ty = typename Params::QuadDeletedTy;

    static constexpr float max_load_factor = 0.5;
    const uint32_t array_size = array::size(h._keys);

    const float real_load_factor = (h._num_valid + h._num_deleted) / float(array_size);
    const float valid_load_factor = h._num_valid / float(array_size);

    if (real_load_factor < max_load_factor) {
        return;
    }

    // Check and see if we do not actually need to double the size, since only
    // valid entries will be entered
    uint32_t new_size = valid_load_factor >= max_load_factor ? array_size * 2 : array_size;

    QuadHash<K, V, Params> new_h{*h._keys._allocator, new_size, h._hash_fn, h._equal_fn};

    for (uint32_t i = 0; i < array_size; ++i) {
        const K &key = h._keys[i];
        if (!h._equal_fn(key, nil_ty::get()) && !h._equal_fn(key, deleted_ty::get())) {
            quad_hash::set(new_h, key, h._values[i]);
        }
    }
    std::swap(h, new_h);
}

} // namespace quad_hash_internal
} // namespace foundation

namespace foundation {
namespace quad_hash {

template <typename K, typename V, typename Params>
uint32_t find(const QuadHash<K, V, Params> &h, const K &key) {
    using nil_ty = typename Params::QuadNilTy;

    uint32_t idx = h._hash_fn(key);

    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        idx = (idx + i) % array::size(h._keys);
        if (h._equal_fn(h._keys[idx], key)) {
            return idx;
        }
        if (h._equal_fn(h._keys[idx], nil_ty::get())) {
            return NOT_FOUND;
        }
    }
    return NOT_FOUND;
}

template <typename K, typename V, typename Params>
void set(QuadHash<K, V, Params> &h, const K &key, const V &value) {
    using nil_ty = typename Params::QuadNilTy;

    quad_hash_internal::rehash_if_needed(h);

    uint64_t idx = h._hash_fn(key);

    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        idx = (idx + i) % array::size(h._keys);
        if (h._equal_fn(h._keys[idx], nil_ty::get()) || h._equal_fn(h._keys[idx], key)) {
            h._keys[idx] = key;
            h._values[idx] = value;
            ++h._num_valid;
            return;
        }
    }

    log_assert(0, "Impossible");
}

template <typename K, typename V, typename Params> void remove(QuadHash<K, V, Params> &h, const K &key) {
    using deleted_ty = typename Params::QuadDeletedTy;

    uint32_t idx = find(h, key);
    if (idx != NOT_FOUND) {
        h._keys[idx] = deleted_ty::get();
        ++h._num_deleted;
    }
}

} // namespace quad_hash
} // namespace foundation
