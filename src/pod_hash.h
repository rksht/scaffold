#pragma once

/// This file contains an implementation of a 'PodHash' which is essentially the
/// Hash in collection_types.h, but can use any POD data type as key. It's
/// a good idea to use Hash for integer keys and this one for larger objects.

#include "array.h"
#include "collection_types.h"
#include "memory.h"
#include "murmur_hash.h"

#include <iterator>
#include <stdint.h>
#include <stdio.h>
#include <type_traits>

/// namespace foundation contains a chain-based hash table implementation for
/// POD key- value pairs
namespace foundation {
template <typename K, typename V> struct _Entry {
    K key;
    V value;
    uint32_t next;
};
/// PodHash<K, V>
template <typename K, typename V>
struct PodHash : std::iterator<std::random_access_iterator_tag, _Entry<K, V>> {
    static_assert(std::is_trivially_copyable<K>::value,
                  "Key type must be trivially copyable");
    static_assert(std::is_trivially_copyable<V>::value,
                  "Value type must be trivially copyabe");

    using Entry = _Entry<K, V>;
    using iterator = typename foundation::Array<Entry>::iterator;
    using const_iterator = typename foundation::Array<Entry>::const_iterator;

    /// Type of hashing function
    using HashFunc = uint64_t (*)(K const &key);

    /// Type of equal operator function - operator== can't be used as it doesn't
    /// work on non-class or non-enumerator types.
    using EqualFunc = bool (*)(K const &k1, K const &k2);

    /// Array mapping a hash to an entry index
    foundation::Array<uint32_t> _hashes;

    /// Array of entries
    foundation::Array<Entry> _entries;

    /// The hashing function to use
    HashFunc _hash_func;

    /// The equal function to use
    EqualFunc _equal_func;

    /// Constructor
    PodHash(foundation::Allocator &hash_alloc,
            foundation::Allocator &entry_alloc, HashFunc hash_func,
            EqualFunc equal_func)
        : _hashes(hash_alloc), _entries(entry_alloc), _hash_func(hash_func),
          _equal_func(equal_func) {}

    /// Move constructor
    PodHash(PodHash &&other)
        : _hashes(std::move(other._hashes)),
          _entries(std::move(other._entries)), _hash_func(other._hash_func) {}

    PodHash(const PodHash &other) = delete;
};

/// Sets the given key's value
template <typename K, typename V>
void set(PodHash<K, V> &h, const K &key, const V &value);

/// Returns true if an entry with the given key is present - does not modify
/// the data structure
template <typename K, typename V>
bool has(const PodHash<K, V> &h, const K &key);

/// Sets the given key's value to the given default value if an entry is not
/// present with the given key. Returns reference to the value associated with
/// the key.
template <typename K, typename V>
V &set_default(PodHash<K, V> &h, const K &key, const V &deffault);

/// Returns constant reference to the key in the entry with the given key -
/// does not modify the data structure
template <typename K, typename V>
const K &get_key(const PodHash<K, V> &h, K const &key, K const &deffault);

/// Removes the entry with the given key
template <typename K, typename V> void remove(PodHash<K, V> &h, const K &key);

/// Returns pointer to the first entry - does not modify the data structure
template <typename K, typename V>
typename PodHash<K, V>::const_iterator cbegin(const PodHash<K, V> &h);

/// Returns pointer to the end of the entry array - does not modify the data
/// structure
template <typename K, typename V>
typename PodHash<K, V>::const_iterator cend(const PodHash<K, V> &h);

namespace _internal {

const uint32_t END_OF_LIST = 0xffffffffu;

struct FindResult {
    uint32_t hash_i;
    uint32_t entry_i;
    uint32_t entry_prev;
};

// Forward declaration
template <typename K, typename V>
void rehash(PodHash<K, V> &h, uint32_t new_size);

// Forward declaration
template <typename K, typename V> void grow(PodHash<K, V> &h);

// Forward declaration
template <typename K, typename V>
void insert(PodHash<K, V> &h, const K &key, const V &value);

template <typename K, typename V>
FindResult find(const PodHash<K, V> &h, const K &key) {
    FindResult fr = {END_OF_LIST, END_OF_LIST, END_OF_LIST};

    if (foundation::array::size(h._hashes) == 0) {
        return fr;
    }

    fr.hash_i = h._hash_func(key) % foundation::array::size(h._hashes);
    fr.entry_i = h._hashes[fr.hash_i];
    while (fr.entry_i != END_OF_LIST) {
        if (h._equal_func(h._entries[fr.entry_i].key, key)) {
            return fr;
        }
        fr.entry_prev = fr.entry_i;
        fr.entry_i = h._entries[fr.entry_prev].next;
    }
    return fr;
}

// Pushes a new entry initialized with the given key
template <typename K, typename V>
uint32_t push_entry(PodHash<K, V> &h, const K &key) {
    typename PodHash<K, V>::Entry e;
    e.key = key;
    e.next = END_OF_LIST;
    uint32_t ei = foundation::array::size(h._entries);
    foundation::array::push_back(h._entries, e);
    return ei;
}

// Searches for the given key and if not found adds a new entry for the key.
// Returns the entry index.
template <typename K, typename V>
uint32_t find_or_make(PodHash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        return fr.entry_i;
    }

    uint32_t ei = push_entry(h, key);
    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = ei;
    } else {
        h._entries[fr.entry_prev].next = ei;
    }
    return ei;
}

// Makes a new entry and appends it to the appropriate chain
template <typename K, typename V>
uint32_t make(PodHash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    const uint32_t ei = push_entry(h, key);

    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = ei;
    } else {
        h._entries[fr.entry_prev].next = ei;
    }
    h._entries[ei].next = fr.entry_i;
    return ei;
}

// Allocates a new array for the hashes and recomputes the hashes of the
// already allocated values
template <typename K, typename V>
void rehash(PodHash<K, V> &h, uint32_t new_size) {
    // create a new hash table
    PodHash<K, V> nh(*h._hashes._allocator, *h._entries._allocator,
                     h._hash_func, h._equal_func);

    // Don't need the previous hashes.
    foundation::array::clear(h._hashes);
    foundation::array::resize(nh._hashes, new_size);
    foundation::array::reserve(nh._entries,
                               foundation::array::size(h._entries));
    // Empty out the hashes first
    for (uint32_t i = 0; i < new_size; ++i) {
        nh._hashes[i] = END_OF_LIST;
    }
    // Insert one by one
    for (uint32_t i = 0; i < foundation::array::size(h._entries); ++i) {
        const typename PodHash<K, V>::Entry &e = h._entries[i];
        insert(nh, e.key, e.value);
    }

    PodHash<K, V> empty(*h._hashes._allocator, *h._entries._allocator,
                        h._hash_func, h._equal_func);
    h.~PodHash<K, V>();
    memcpy(&h, &nh, sizeof(PodHash<K, V>));
    memcpy(&nh, &empty, sizeof(PodHash<K, V>));
}

template <typename K, typename V> void grow(PodHash<K, V> &h) {
    uint32_t new_size = foundation::array::size(h._entries) * 2 + 10;
    rehash(h, new_size);
}

// Returns true if the number of entries is more than 70% of the number of
// hashes. Note that if the number of entries is less than that of hashes then
// surely the hash table is not exhausted. So this function detects that too.
template <typename K, typename V> bool full(const PodHash<K, V> &h) {
    const float max_load_factor = 0.7;
    return foundation::array::size(h._entries) >=
           foundation::array::size(h._hashes) * max_load_factor;
}

// Inserts an entry by simply appending to the chain, so if no chain already
// exists for the given key, it's same as creating a new entry. Otherwise, it
// just adds a new entry to the chain, and does not overwrite any entry having
// the same key
template <typename K, typename V>
void insert(PodHash<K, V> &h, const K &key, const V &value) {
    if (foundation::array::size(h._hashes) == 0) {
        grow(h);
    }

    const uint32_t ei = make(h, key);
    h._entries[ei].value = value;
    if (full(h)) {
        grow(h);
    }
}

// Erases the entry found
template <typename K, typename V>
void erase(PodHash<K, V> &h, const FindResult &fr) {
    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = h._entries[fr.entry_i].next;
    } else {
        h._entries[fr.entry_prev].next = h._entries[fr.entry_i].next;
    }

    if (fr.entry_i == foundation::array::size(h._entries) - 1) {
        foundation::array::pop_back(h._entries);
        return;
    }

    h._entries[fr.entry_i] =
        h._entries[foundation::array::size(h._entries) - 1];
    foundation::array::pop_back(h._entries);
    FindResult last = find(h, h._entries[fr.entry_i].key);

    if (last.entry_prev == END_OF_LIST) {
        h._hashes[last.hash_i] = fr.entry_i;
    } else {
        h._entries[last.entry_prev].next = fr.entry_i;
    }
}
// Finds entry with the given key and removes it
template <typename K, typename V>
void find_and_erase(PodHash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        erase(h, fr);
    }
}

} // namespace _internal

/// Sets the key to the value
template <typename K, typename V>
void set(PodHash<K, V> &h, const K &key, const V &value) {
    if (foundation::array::size(h._hashes) == 0) {
        _internal::grow(h);
    }
    uint32_t ei = _internal::find_or_make(h, key);
    h._entries[ei].value = value;
    if (_internal::full(h)) {
        _internal::grow(h);
    }
}

/// Checks if the any entry with given key is in the table
template <typename K, typename V> bool has(PodHash<K, V> &h, const K &key) {
    const _internal::FindResult fr = _internal::find(h, key);
    return fr.entry_i != _internal::END_OF_LIST;
}

/// Returns the value associated with the given key if the key exists, otherwise
/// creates a new entry and sets its value to deffault and returns it.
template <typename K, typename V>
V &set_default(PodHash<K, V> &h, const K &key, const V &deffault) {
    _internal::FindResult fr = _internal::find(h, key);
    if (fr.entry_i == _internal::END_OF_LIST) {
        const uint32_t ei = _internal::make(h, key);
        h._entries[ei].value = deffault;
        return h._entries[ei].value;
    }
    return h._entries[fr.entry_i].value;
}

/// Returns a const reference to the key. Use when using the hash as a set.
template <typename K, typename V>
const K &get_key(const PodHash<K, V> &h, K const &key, K const &deffault) {
    _internal::FindResult fr = _internal::find(h, key);
    if (fr.entry_i == _internal::END_OF_LIST) {
        return deffault;
    }
    return h._entries[fr.entry_i].key;
}

/// Removes the entry with the given key if it exists.
template <typename K, typename V> void remove(PodHash<K, V> &h, const K &key) {
    _internal::find_and_erase(h, key);
}

/// Returns a constant-entry pointer to the start if the buffer of entries
template <typename K, typename V>
typename PodHash<K, V>::const_iterator cbegin(const PodHash<K, V> &h) {
    return h._entries.begin();
}

/// Returns a constant-entry pointer to one cell past the end of the buffer of
/// entries
template <typename K, typename V>
typename PodHash<K, V>::const_iterator cend(const PodHash<K, V> &h) {
    return h._entries.end();
}

/// Returns an entry pointer to the start to the start of  the buffer of entries
template <typename K, typename V>
typename PodHash<K, V>::iterator begin(PodHash<K, V> &h) {
    return h._entries.begin();
}

/// Returns a constant-entry pointer to one cell past the end of the buffer of
/// entries
template <typename K, typename V>
typename PodHash<K, V>::iterator end(PodHash<K, V> &h) {
    return h._entries.end();
}

/// Finds the maximum chain length in the hash table. For debugging and stuff.
/// :)
template <typename K, typename V>
uint32_t max_chain_length(const PodHash<K, V> &h) {
    uint32_t max_length = 0;

    for (uint32_t i = 0; i < foundation::array::size(h._entries); ++i) {
        if (h._hashes[i] == _internal::END_OF_LIST) {
            continue;
        }

        uint32_t entry_i = h._hashes[i];
        uint32_t length = 1;

        while (h._entries[entry_i].next != _internal::END_OF_LIST) {
            entry_i = h._entries[entry_i].next;
            ++length;
        }
        if (max_length < length) {
            max_length = length;
        }
    }
    return max_length;
}

} // namespace foundation
