#pragma once

#include "array.h"
#include "collection_types.h"
#include "memory.h"

#include <algorithm> // std::swap
#include <functional>
#include <iterator>
#include <memory> // std::move
#include <stdint.h>
#include <stdio.h>
#include <type_traits>

namespace fo {

namespace pod_hash_internal {

template <typename K, typename V> struct Entry {
    K key;
    mutable V value;
    uint32_t next;
};

} // namespace pod_hash_internal

/// 'PodHash' is similar hash table like the one in collection_types.h, but can
/// use any 'trivially-copyassignable data type as key.
template <typename K, typename V> struct PodHash {

    static_assert(std::is_trivially_copy_assignable<K>::value, "Key type must be trivially copy assignable");
    static_assert(std::is_trivially_copy_assignable<V>::value,
                  "Value type must be trivially copy assignable");

    using Entry = pod_hash_internal::Entry<K, V>;

    /// Both are const_iterator to the underlying array because we don't allow
    /// changing keys.
    using iterator = typename fo::Array<Entry>::const_iterator;
    using const_iterator = typename fo::Array<Entry>::const_iterator;

    /// Type of hashing function
    using HashFn = std::function<uint64_t(K const &key)>;

    /// Type of equal operator function - operator== can't be used as it doesn't
    /// work on non-class or non-enumerator types.
    using EqualFn = std::function<bool(K const &k1, K const &k2)>;

    /// Array mapping a hash to an entry index
    fo::Array<uint32_t> _hashes;

    /// Array of entries
    fo::Array<Entry> _entries;

    /// The hash function to use
    HashFn _hashfn;

    /// The equal function to use
    EqualFn _equalfn;

    /// Constructor
    PodHash(fo::Allocator &hash_alloc, fo::Allocator &entry_alloc, HashFn hash_func, EqualFn equal_func)
        : _hashes(hash_alloc)
        , _entries(entry_alloc)
        , _hashfn(hash_func)
        , _equalfn(equal_func) {}

    /// Returns reference to the value associated with given key. If no value is
    /// currently associated, constructs a value using value-initialization and
    /// returns reference to that.
    V &operator[](const K &key);
};

// -- Iterators on PodHash

template <typename K, typename V> auto cbegin(const PodHash<K, V> &h) { return h._entries.begin(); }
template <typename K, typename V> auto cend(const PodHash<K, V> &h) { return h._entries.end(); }
template <typename K, typename V> auto begin(const PodHash<K, V> &h) { return h._entries.begin(); }
template <typename K, typename V> auto end(const PodHash<K, V> &h) { return h._entries.end(); }
template <typename K, typename V> auto begin(PodHash<K, V> &h) { return h._entries.begin(); }
template <typename K, typename V> auto end(PodHash<K, V> &h) { return h._entries.end(); }

// -- Functions to operate on PodHash

/// Reserve space for `size` keys. Does not reserve space for the entries
/// beforehand
template <typename K, typename V> void reserve(PodHash<K, V> &h, uint32_t size);

/// Sets the given key's value (Can trigger a rehash if `key` doesn't already
/// exist)
template <typename K, typename V> void set(PodHash<K, V> &h, const K &key, const V &value);

/// Returns true if an entry with the given key is present.
template <typename K, typename V> bool has(const PodHash<K, V> &h, const K &key);

/// Returns an iterator to the entry containing given `key`, if present.
/// Otherwise returns an iterator pointing to the end.
template <typename K, typename V> typename PodHash<K, V>::iterator get(const PodHash<K, V> &h, const K &key);

/// Sets the given key's associated value to the given default value if no
/// entry is present with the given key. Returns reference to the value
/// associated with the key. (Can trigger a rehash if `key` doesn't already
/// exist)
template <typename K, typename V> V &set_default(PodHash<K, V> &h, const K &key, const V &deffault);

/// Returns constant reference to the key in the entry with the given key -
/// does not modify the data structure
template <typename K, typename V> const K &get_key(const PodHash<K, V> &h, K const &key, K const &deffault);

/// Removes the entry with the given key
template <typename K, typename V> void remove(PodHash<K, V> &h, const K &key);

} // namespace fo

namespace fo {
namespace pod_hash_internal {

const uint32_t END_OF_LIST = 0xffffffffu;

struct FindResult {
    uint32_t hash_i;
    uint32_t entry_i;
    uint32_t entry_prev;
};

template <typename K, typename V> uint32_t hash_slot(const PodHash<K, V> &h, const K &k) {
    return h._hashfn(k) % fo::array::size(h._hashes);
}

// Forward declaration
template <typename K, typename V> void rehash(PodHash<K, V> &h, uint32_t new_size);

// Forward declaration
template <typename K, typename V> void grow(PodHash<K, V> &h);

// Forward declaration
template <typename K, typename V> void insert(PodHash<K, V> &h, const K &key, const V &value);

template <typename K, typename V> FindResult find(const PodHash<K, V> &h, const K &key) {
    FindResult fr = {END_OF_LIST, END_OF_LIST, END_OF_LIST};

    if (fo::array::size(h._hashes) == 0) {
        return fr;
    }

    fr.hash_i = hash_slot(h, key);
    fr.entry_i = h._hashes[fr.hash_i];
    while (fr.entry_i != END_OF_LIST) {
        if (h._equalfn(h._entries[fr.entry_i].key, key)) {
            return fr;
        }
        fr.entry_prev = fr.entry_i;
        fr.entry_i = h._entries[fr.entry_prev].next;
    }
    return fr;
}

template <typename K, typename V, bool value_init> struct PushEntry;

template <typename K, typename V> struct PushEntry<K, V, true> {
    static uint32_t push_entry(PodHash<K, V> &h, const K &key) {
        typename PodHash<K, V>::Entry e{};
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = fo::array::size(h._entries);
        fo::array::push_back(h._entries, e);
        return ei;
    }
};

template <typename K, typename V> struct PushEntry<K, V, false> {
    static uint32_t push_entry(PodHash<K, V> &h, const K &key) {
        typename PodHash<K, V>::Entry e;
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = fo::array::size(h._entries);
        fo::array::push_back(h._entries, e);
        return ei;
    }
};

/// Searches for the given key and if not found adds a new entry for the key.
/// Returns the entry index.
template <typename K, typename V>
uint32_t find_or_make(PodHash<K, V> &h, const K &key, bool value_initialize) {
    FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        return fr.entry_i;
    }

    fr.hash_i = hash_slot(h, key);

    if (value_initialize) {
        fr.entry_i = PushEntry<K, V, true>::push_entry(h, key);
    } else {
        fr.entry_i = PushEntry<K, V, false>::push_entry(h, key);
    }

    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = fr.entry_i;
    } else {
        h._entries[fr.entry_prev].next = fr.entry_i;
    }
    return fr.entry_i;
}

/// Makes a new entry and appends it to the appropriate chain
template <typename K, typename V> uint32_t make(PodHash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    const uint32_t ei = PushEntry<K, V, false>::push_entry(h, key);

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
template <typename K, typename V> void rehash(PodHash<K, V> &h, uint32_t new_size) {
    // create a new hash table
    PodHash<K, V> new_hash(*h._hashes._allocator, *h._entries._allocator, h._hashfn, h._equalfn);

    // Don't need the previous hashes.
    fo::array::free(h._hashes);
    fo::array::resize(new_hash._hashes, new_size);
    fo::array::reserve(new_hash._entries, fo::array::size(h._entries));

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

template <typename K, typename V> void grow(PodHash<K, V> &h) {
    uint32_t new_size = fo::array::size(h._entries) * 2 + 10;
    rehash(h, new_size);
}

/// Returns true if the number of entries is more than 70% of the number of
/// hashes. Note that if the number of entries is less than that of hashes
/// then surely the hash table is not exhausted. So this function detects that
/// too.
template <typename K, typename V> bool full(const PodHash<K, V> &h) {
    const float max_load_factor = 0.7;
    return fo::array::size(h._entries) >= fo::array::size(h._hashes) * max_load_factor;
}

/// Inserts an entry by simply appending to the chain, so if no chain already
/// exists for the given key, it's same as creating a new entry. Otherwise, it
/// just adds a new entry to the chain, and does not overwrite any entry having
/// the same key
template <typename K, typename V> void insert(PodHash<K, V> &h, const K &key, const V &value) {
    if (fo::array::size(h._hashes) == 0) {
        grow(h);
    }

    const uint32_t ei = make(h, key);
    h._entries[ei].value = value;
    if (full(h)) {
        grow(h);
    }
}

/// Erases the entry found
template <typename K, typename V> void erase(PodHash<K, V> &h, const FindResult &fr) {
    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = h._entries[fr.entry_i].next;
    } else {
        h._entries[fr.entry_prev].next = h._entries[fr.entry_i].next;
    }

    if (fr.entry_i == fo::array::size(h._entries) - 1) {
        fo::array::pop_back(h._entries);
        return;
    }

    h._entries[fr.entry_i] = h._entries[fo::array::size(h._entries) - 1];
    fo::array::pop_back(h._entries);
    FindResult last = find(h, h._entries[fr.entry_i].key);

    if (last.entry_prev == END_OF_LIST) {
        h._hashes[last.hash_i] = fr.entry_i;
    } else {
        h._entries[last.entry_prev].next = fr.entry_i;
    }
}

/// Finds entry with the given key and removes it
template <typename K, typename V> void find_and_erase(PodHash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        erase(h, fr);
    }
}

} // namespace pod_hash_internal
} // namespace fo

namespace fo {

template <typename K, typename V> void reserve(PodHash<K, V> &h, uint32_t size) {
    pod_hash_internal::rehash(h, size);
}

template <typename K, typename V> void set(PodHash<K, V> &h, const K &key, const V &value) {
    if (fo::array::size(h._hashes) == 0) {
        pod_hash_internal::grow(h);
    }
    const uint32_t ei = pod_hash_internal::find_or_make(h, key, false);
    h._entries[ei].value = value;
    if (pod_hash_internal::full(h)) {
        pod_hash_internal::grow(h);
    }
}

template <typename K, typename V> bool has(PodHash<K, V> &h, const K &key) {
    const pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    return fr.entry_i != pod_hash_internal::END_OF_LIST;
}

template <typename K, typename V> typename PodHash<K, V>::iterator get(const PodHash<K, V> &h, const K &key) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        return h._entries.end();
    }
    return h._entries.begin() + fr.entry_i;
}

template <typename K, typename V> V &PodHash<K, V>::operator[](const K &key) {
    if (fo::array::size(_hashes) == 0) {
        pod_hash_internal::grow(*this);
    }
    auto ei = pod_hash_internal::find_or_make(*this, key, true);
    return (this->_entries.begin() + ei)->value;
}

template <typename K, typename V> V &set_default(PodHash<K, V> &h, const K &key, const V &deffault) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        if (fo::array::size(h._hashes) == 0) {
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
template <typename K, typename V> const K &get_key(const PodHash<K, V> &h, K const &key, K const &deffault) {
    pod_hash_internal::FindResult fr = pod_hash_internal::find(h, key);
    if (fr.entry_i == pod_hash_internal::END_OF_LIST) {
        return deffault;
    }
    return h._entries[fr.entry_i].key;
}

/// Removes the entry with the given key if it exists.
template <typename K, typename V> void remove(PodHash<K, V> &h, const K &key) {
    pod_hash_internal::find_and_erase(h, key);
}

/// Finds the maximum chain length in the hash table.
template <typename K, typename V> uint32_t max_chain_length(const PodHash<K, V> &h) {
    uint32_t max_length = 0;

    for (uint32_t i = 0; i < fo::array::size(h._entries); ++i) {
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
