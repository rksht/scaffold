#pragma once

#include <scaffold/array.h>
#include <scaffold/collection_types.h>
#include <scaffold/debug.h>
#include <scaffold/memory.h>

#include <algorithm> // std::swap
#include <functional>
#include <iterator>
#include <memory> // std::move
#include <stdint.h>
#include <stdio.h>
#include <type_traits>

namespace fo {

// Passing this instead of a callable to `HashFnType` will simply use the value of the key as its hash.
template <typename K> struct IdentityHashTag { static_assert(std::is_integral<K>::value, ""); };

// Passing this instead of a callable "equal" comparison will simply make the table use operator== on the
// keys.
template <typename K> struct IdentityEqualTag {};

namespace pod_hash_internal {

template <typename K, typename V> struct Entry {
    K key;
    mutable V value;
    uint32_t next;
};

} // namespace pod_hash_internal

/// 'PodHash' is similar hash table like the one in collection_types.h, but can use any 'trivially-
/// copyassignable' data type as key.
template <typename K,
          typename V,
          typename HashFnType = IdentityHashTag<K>,
          typename EqualFnType = IdentityEqualTag<K>>
struct PodHash {
    using Entry = pod_hash_internal::Entry<K, V>;
    using HashFn = HashFnType;
    using EqualFn = EqualFnType;

    /// Both are const_iterator to the underlying array because we don't allow changing keys.
    using iterator = typename fo::Array<Entry>::const_iterator;
    using const_iterator = typename fo::Array<Entry>::const_iterator;

    fo::Array<uint32_t> _hashes; // Array mapping a hash to an entry index
    fo::Array<Entry> _entries;   // Array of entries
    HashFnType _hashfn;          // The hash function to use
    EqualFnType _equalfn;        // The equal function to use
    float _load_factor = 0.7f;

    /// Constructor
    PodHash(fo::Allocator &hash_alloc,
            fo::Allocator &entry_alloc,
            HashFnType hash_func,
            EqualFnType equal_func)
        : _hashes(hash_alloc)
        , _entries(entry_alloc)
        , _hashfn(std::move(hash_func))
        , _equalfn(std::move(equal_func)) {
        // Check that the provided function types are ok. This is to catch the error early. This check is
        // performed only in c++17 mode
#if IS_CPP_17
        using returned_hash_type = decltype(_hashfn(std::declval<K>()));
        static_assert(std::is_constructible<uint64_t, returned_hash_type>::value,
                      "Incompatible hash function");

        using equal_result = decltype(_equalfn(std::declval<K>(), std::declval<K>()));
        static_assert(std::is_constructible<bool, equal_result>::value, "Incompatible equals function");
#endif
    }

    PodHash(const PodHash &) = default;
    PodHash(PodHash &&) = default;

    PodHash &operator=(const PodHash &) = default;
    PodHash &operator=(PodHash &&) = default;

    /// Returns reference to the value associated with given key. If no value is currently associated,
    /// constructs a value using value-initialization and returns reference to that.
    V &operator[](const K &key);
};

#define TypeList typename K, typename V, typename HashFnType, typename EqualFnType
#define PodHashSig PodHash<K, V, HashFnType, EqualFnType>

// A 'make_' function for convenience. Takes just one allocator used to allocate both the buckets and entries.
// You can call like this - fo::make_pod_hash<u32, const char *>(alloc) - for example.
template <typename K,
          typename V,
          typename HashFnType = IdentityHashTag<K>,
          typename EqualFnType = IdentityEqualTag<K>>
PodHashSig make_pod_hash(fo::Allocator &alloc = fo::memory_globals::default_allocator(),
                         HashFnType hash_func = IdentityHashTag<K>(),
                         EqualFnType equal_func = IdentityEqualTag<K>()) {
    return PodHashSig(alloc, alloc, std::move(hash_func), std::move(equal_func));
}

// -- Iterators on PodHash
template <TypeList> auto cbegin(const PodHashSig &h) { return h._entries.begin(); }
template <TypeList> auto cend(const PodHashSig &h) { return h._entries.end(); }
template <TypeList> auto begin(const PodHashSig &h) { return h._entries.begin(); }
template <TypeList> auto end(const PodHashSig &h) { return h._entries.end(); }
template <TypeList> auto begin(PodHashSig &h) { return h._entries.begin(); }
template <TypeList> auto end(PodHashSig &h) { return h._entries.end(); }

// -- Functions to operate on PodHash

/// Reserve space for `size` keys. Does not reserve space for the entries
/// beforehand
template <TypeList> void reserve(PodHashSig &h, uint32_t size);

/// Sets the given key's value (Can trigger a rehash if `key` doesn't already
/// exist)
template <TypeList> void set(PodHashSig &h, const K &key, const V &value);

/// Returns true if an entry with the given key is present.
template <TypeList> bool has(const PodHashSig &h, const K &key);

/// Returns an iterator to the entry containing given `key`, if present.
/// Otherwise returns an iterator pointing to the end.
template <TypeList> typename PodHashSig::iterator get(const PodHashSig &h, const K &key);

/// Sets the given key's associated value to the given default value if no
/// entry is present with the given key. Returns reference to the value
/// associated with the key. (Can trigger a rehash if `key` doesn't already
/// exist)
template <TypeList> V &set_default(PodHashSig &h, const K &key, const V &deffault);

/// Returns constant reference to the key in the entry with the given key -
/// does not modify the data structure
template <TypeList> const K &get_key(const PodHashSig &h, K const &key, K const &deffault);

/// Removes the entry with the given key
template <TypeList> void remove(PodHashSig &h, const K &key);

/// Set the load factor
template <TypeList> void set_load_factor(PodHashSig &h, float new_load_factor);

} // namespace fo

namespace fo {
namespace pod_hash_internal {

const uint32_t END_OF_LIST = 0xffffffffu;

struct FindResult {
    uint32_t hash_i;
    uint32_t entry_i;
    uint32_t entry_prev;
};

template <TypeList> struct KeyHashSlot {
    static REALLY_INLINE uint64_t hash_slot(const PodHashSig &h, const K &k) {
        return h._hashfn(k) % fo::size(h._hashes);
    }
};

template <typename K, typename V, typename EqualFnType>
struct KeyHashSlot<K, V, IdentityHashTag<K>, EqualFnType> {
    static REALLY_INLINE uint64_t hash_slot(const PodHash<K, V, IdentityHashTag<K>, EqualFnType> &h,
                                            const K &k) {
        return k % fo::size(h._hashes);
    }
};

template <TypeList> uint32_t hash_slot(const PodHashSig &h, const K &k) {
    return KeyHashSlot<K, V, HashFnType, EqualFnType>::hash_slot(h, k);
}

// Forward declaration
template <TypeList> void rehash(PodHashSig &h, uint32_t new_size);

// Forward declaration
template <TypeList> void grow(PodHashSig &h);

// Forward declaration
template <TypeList> void insert(PodHashSig &h, const K &key, const V &value);

template <TypeList> struct KeyEqualCaller {
    static bool key_equal(const PodHashSig &h, const K &k1, const K &k2) { return h._equalfn(k1, k2); }
};

template <typename K, typename V, typename HashFnType>
struct KeyEqualCaller<K, V, HashFnType, IdentityEqualTag<K>> {
    static bool key_equal(const PodHash<K, V, HashFnType, IdentityEqualTag<K>> &h, const K &k1, const K &k2) {
        return k1 == k2;
    }
};

template <TypeList> REALLY_INLINE auto key_equal(const PodHashSig &h, const K &key1, const K &key2) {
    return KeyEqualCaller<K, V, HashFnType, EqualFnType>::key_equal(h, key1, key2);
};

template <TypeList> FindResult find(const PodHashSig &h, const K &key) {
    FindResult fr = { END_OF_LIST, END_OF_LIST, END_OF_LIST };

    if (fo::size(h._hashes) == 0) {
        return fr;
    }

    fr.hash_i = hash_slot(h, key);
    fr.entry_i = h._hashes[fr.hash_i];
    while (fr.entry_i != END_OF_LIST) {
        if (key_equal(h, h._entries[fr.entry_i].key, key)) {
            return fr;
        }
        fr.entry_prev = fr.entry_i;
        fr.entry_i = h._entries[fr.entry_prev].next;
    }
    return fr;
}

template <typename K, typename V, typename HashFnType, typename EqualFnType, bool value_init>
struct PushEntry;

template <TypeList> struct PushEntry<K, V, HashFnType, EqualFnType, true> {
    static uint32_t push_entry(PodHashSig &h, const K &key) {
        typename PodHashSig::Entry e{};
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = fo::size(h._entries);
        fo::push_back(h._entries, e);
        return ei;
    }
};

template <TypeList> struct PushEntry<K, V, HashFnType, EqualFnType, false> {
    static uint32_t push_entry(PodHashSig &h, const K &key) {
        typename PodHashSig::Entry e;
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = fo::size(h._entries);
        fo::push_back(h._entries, e);
        return ei;
    }
};

/// Searches for the given key and if not found adds a new entry for the key.
/// Returns the entry index.
template <TypeList> uint32_t find_or_make(PodHashSig &h, const K &key, bool value_initialize) {
    FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        return fr.entry_i;
    }

    fr.hash_i = hash_slot(h, key);

    if (value_initialize) {
        fr.entry_i = PushEntry<K, V, HashFnType, EqualFnType, true>::push_entry(h, key);
    } else {
        fr.entry_i = PushEntry<K, V, HashFnType, EqualFnType, false>::push_entry(h, key);
    }

    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = fr.entry_i;
    } else {
        h._entries[fr.entry_prev].next = fr.entry_i;
    }
    return fr.entry_i;
}

/// Makes a new entry and appends it to the appropriate chain
template <TypeList> uint32_t make(PodHashSig &h, const K &key) {
    const FindResult fr = find(h, key);
    const uint32_t ei = PushEntry<K, V, HashFnType, EqualFnType, false>::push_entry(h, key);

    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = ei;
    } else {
        h._entries[fr.entry_prev].next = ei;
    }
    h._entries[ei].next = fr.entry_i;
    return ei;
}

/// Allocates a new array for the hashes and recomputes the hashes of the
/// already allocated values
template <TypeList> void rehash(PodHashSig &h, uint32_t new_size) {
    // create a new hash table
    PodHashSig new_hash(*h._hashes._allocator, *h._entries._allocator, h._hashfn, h._equalfn);

    // Don't need the previous hashes.
    fo::free(h._hashes);
    fo::resize(new_hash._hashes, new_size);
    fo::reserve(new_hash._entries, fo::size(h._entries));

    // Empty out hashes
    for (uint32_t &entry_i : new_hash._hashes) {
        entry_i = END_OF_LIST;
    }

    // Insert one by one
    for (const auto &entry : h._entries) {
        insert(new_hash, entry.key, entry.value);
    }

    std::swap(h, new_hash);
}

template <TypeList> void grow(PodHashSig &h) {
    uint32_t new_size = fo::size(h._entries) * 2 + 10;
    rehash(h, new_size);
}

/// Returns true if the number of entries is more than 70% of the number of hashes. Note that if the number of
/// entries is less than that of hashes then surely the hash table is not exhausted. So this function detects
/// that too.
template <TypeList> bool full(const PodHashSig &h) {
    const float max_load_factor = h._load_factor;
    return fo::size(h._entries) >= fo::size(h._hashes) * max_load_factor;
}

/// Inserts an entry by simply appending to the chain, so if no chain already exists for the given key, it's
/// same as creating a new entry. Otherwise, it just adds a new entry to the chain, and does not overwrite any
/// entry having the same key
template <TypeList> void insert(PodHashSig &h, const K &key, const V &value) {
    if (fo::size(h._hashes) == 0) {
        grow(h);
    }

    const uint32_t ei = make(h, key);
    h._entries[ei].value = value;
    if (full(h)) {
        grow(h);
    }
}

/// Erases the entry found
template <TypeList> void erase(PodHashSig &h, const FindResult &fr) {
    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = h._entries[fr.entry_i].next;
    } else {
        h._entries[fr.entry_prev].next = h._entries[fr.entry_i].next;
    }

    if (fr.entry_i == fo::size(h._entries) - 1) {
        fo::pop_back(h._entries);
        return;
    }

    h._entries[fr.entry_i] = h._entries[fo::size(h._entries) - 1];
    fo::pop_back(h._entries);
    FindResult last = find(h, h._entries[fr.entry_i].key);

    if (last.entry_prev == END_OF_LIST) {
        h._hashes[last.hash_i] = fr.entry_i;
    } else {
        h._entries[last.entry_prev].next = fr.entry_i;
    }
}

/// Finds entry with the given key and removes it
template <TypeList> void find_and_erase(PodHashSig &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        erase(h, fr);
    }
}

} // namespace pod_hash_internal
} // namespace fo

namespace fo {

template <TypeList> void reserve(PodHashSig &h, uint32_t size) { pod_hash_internal::rehash(h, size); }

template <TypeList> void set(PodHashSig &h, const K &key, const V &value) {
    if (fo::size(h._hashes) == 0) {
        pod_hash_internal::grow(h);
    }
    const uint32_t ei = pod_hash_internal::find_or_make(h, key, false);
    h._entries[ei].value = value;
    if (pod_hash_internal::full(h)) {
        pod_hash_internal::grow(h);
    }
}

template <TypeList> bool has(PodHashSig &h, const K &key) {
    const pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    return fr.entry_i != pod_hash_internal::END_OF_LIST;
}

template <TypeList> typename PodHashSig::iterator get(const PodHashSig &h, const K &key) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        return h._entries.end();
    }
    return h._entries.begin() + fr.entry_i;
}

template <TypeList> V &PodHashSig::operator[](const K &key) {
    if (fo::size(_hashes) == 0) {
        pod_hash_internal::grow(*this);
    }
    auto ei = pod_hash_internal::find_or_make(*this, key, true);
    return (this->_entries.begin() + ei)->value;
}

template <TypeList> V &set_default(PodHashSig &h, const K &key, const V &deffault) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        if (fo::size(h._hashes) == 0) {
            pod_hash_internal::grow(h);
        }
        const uint32_t ei = pod_hash_internal::make(h, key);
        h._entries[ei].value = deffault;
        if (pod_hash_internal::full(h)) {
            pod_hash_internal::grow(h);
        }
        return h._entries[ei].value;
    }
    return h._entries[fr.entry_i].value;
}

/// Returns a const reference to the key. Use when using the hash as a set.
template <TypeList> const K &get_key(const PodHashSig &h, K const &key, K const &deffault) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        return deffault;
    }
    return h._entries[fr.entry_i].key;
}

/// Removes the entry with the given key if it exists.
template <TypeList> void remove(PodHashSig &h, const K &key) { pod_hash_internal::find_and_erase(h, key); }

template <TypeList> void remove(PodHashSig &h, float new_load_factor) {
    log_assert(new_load_factor <= 1.0f, "Must have load factor < 1.0f for decent performance");
    h._load_factor = new_load_factor;
}

/// Finds the maximum chain length in the hash table.
template <TypeList> uint32_t max_chain_length(const PodHashSig &h) {
    uint32_t max_length = 0;

    for (uint32_t i = 0; i < fo::size(h._entries); ++i) {
        if (h._hashes[i] == pod_hash_internal::END_OF_LIST) {
            continue;
        }

        uint32_t entry_i = h._hashes[i];
        uint32_t length = 1;

        while (h._entries[entry_i].next != pod_hash_internal::END_OF_LIST) {
            entry_i = h._entries[entry_i].next;
            ++length;
        }
        if (max_length < length) {
            max_length = length;
        }
    }
    return max_length;
}

} // namespace fo

#undef TypeList
#undef PodHashSig
