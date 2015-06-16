#pragma once

#include "collection_types.h"
#include "array.h"
#include "memory.h"
#include "murmur_hash.h"

#include <stdint.h>
#include <stdio.h>

namespace pod_hash {

namespace fo = foundation;

/// Type of hashing function
template <typename K> using HashFunc = uint64_t (*)(const K *key_p);

/// Type of equal operator function - operator== can't be used as it doesn't
/// work on non-class or non-enumerator types.
template <typename K> using EqualFunc = bool (*)(const K &k1, const K &k2);

/// A hash that works on POD types, that is, the hashes as well as the values
/// are copied using memcpy. Destructor for the keys or the values are not
/// called when the data structure is deleted.
template <typename K, typename V> struct Hash {
    struct Entry {
        K key;
        V value;
        uint32_t next;
    };

    fo::Array<uint32_t> _hashes;
    fo::Array<Entry> _entries;
    HashFunc<K> _hash_func;
    EqualFunc<K> _equal_func;

    Hash(fo::Allocator &hash_alloc, fo::Allocator &entry_alloc,
         HashFunc<K> hash_func, EqualFunc<K> equal_func)
        : _hashes(hash_alloc), _entries(entry_alloc), _hash_func(hash_func),
          _equal_func(equal_func) {}

    Hash(Hash &&other)
        : _hashes(std::move(other._hashes)),
          _entries(std::move(other._entries)), _hash_func(other._hash_func) {}
};

namespace _internal {

const uint32_t END_OF_LIST = 0xffffffffu;

struct FindResult {
    uint32_t hash_i;
    uint32_t entry_i;
    uint32_t entry_prev;
};

// Forward declaration
template <typename K, typename V> void rehash(Hash<K, V> &h, uint32_t new_size);

template <typename K, typename V> void grow(Hash<K, V> &h);

template <typename K, typename V>
void insert(Hash<K, V> &h, const K &key, const V &value);

template <typename K, typename V>
FindResult find(const Hash<K, V> &h, const K &key) {
    FindResult fr = {END_OF_LIST, END_OF_LIST, END_OF_LIST};

    if (fo::array::size(h._hashes) == 0) {
        return fr;
    }

    fr.hash_i = h._hash_func(&key) % fo::array::size(h._hashes);
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

template <typename K, typename V>
uint32_t add_entry(Hash<K, V> &h, const K &key) {
    typename Hash<K, V>::Entry e;
    e.key = key;
    e.next = END_OF_LIST;
    uint32_t ei = fo::array::size(h._entries);
    fo::array::push_back(h._entries, e);
    return ei;
}

/// Searches for the given key and if not found adds a new
/// entry for the key. Returns the entry index.
template <typename K, typename V>
uint32_t find_or_make(Hash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        return fr.entry_i;
    }

    uint32_t ei = add_entry(h, key);
    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = ei;
    } else {
        h._entries[fr.entry_prev].next = ei;
    }
    return ei;
}

/// Makes a new entry and appends it to the appropriate chain
template <typename K, typename V> uint32_t make(Hash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    const uint32_t ei = add_entry(h, key);

    if (fr.entry_prev == END_OF_LIST) {
        h._hashes[fr.hash_i] = ei;
    } else {
        h._entries[fr.entry_prev].next = ei;
    }
    h._entries[ei].next = fr.entry_i;
    return ei;
}

/// Allocates a new array for the hashes and recomputes the hashes of
/// the already allocated values
template <typename K, typename V>
void rehash(Hash<K, V> &h, uint32_t new_size) {
    Hash<K, V> nh(*h._hashes._allocator, *h._entries._allocator, h._hash_func,
                  h._equal_func); // a new hash table
    fo::array::resize(nh._hashes, new_size);
    fo::array::reserve(nh._entries, fo::array::size(h._entries));
    for (uint32_t i = 0; i < new_size; ++i) {
        nh._hashes[i] = END_OF_LIST; // Empty out the hashes
    }
    for (uint32_t i = 0; i < fo::array::size(h._entries); ++i) {
        const typename Hash<K, V>::Entry &e = h._entries[i];
        insert(nh, e.key, e.value);
    }

    Hash<K, V> empty(*h._hashes._allocator, *h._entries._allocator,
                     h._hash_func, h._equal_func);
    h.~Hash<K, V>();
    memcpy(&h, &nh, sizeof(Hash<K, V>));
    memcpy(&nh, &empty, sizeof(Hash<K, V>));
}

template <typename K, typename V> void grow(Hash<K, V> &h) {
    uint32_t new_size = fo::array::size(h._entries) * 2 + 10;
    rehash(h, new_size);
}

/// Returns true if the number of entries is more than 70% of the number of
/// hashes. Note that if the number of entries is less than that of hashes
/// then surely the hash table is not exhausted. So this function detects that
/// too.
template <typename K, typename V> bool full(const Hash<K, V> &h) {
    const float max_load_factor = 0.7;
    return fo::array::size(h._entries) >=
           fo::array::size(h._hashes) * max_load_factor;
}

/// Inserts an entry by simply appending to the chain, so if no chain
/// already exists for the given key, it's same as creating a new entry.
/// Otherwise, it just adds a new entry to the chain, and does not overwrite
/// any entry having the same key
template <typename K, typename V>
void insert(Hash<K, V> &h, const K &key, const V &value) {
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
template <typename K, typename V>
void erase(Hash<K, V> &h, const FindResult &fr) {
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

template <typename K, typename V>
void find_and_erase(Hash<K, V> &h, const K &key) {
    const FindResult fr = find(h, key);
    if (fr.entry_i != END_OF_LIST) {
        erase(h, fr);
    }
}

} // namespace _internal

/// Sets the key to the value
template <typename K, typename V>
void set(Hash<K, V> &h, const K &key, const V &value) {
    if (fo::array::size(h._hashes) == 0) {
        _internal::grow(h);
    }
    uint32_t ei = _internal::find_or_make(h, key);
    h._entries[ei].value = value;
    if (_internal::full(h)) {
        _internal::grow(h);
    }
}

/// Checks if the any entry with given key is in the table
template <typename K, typename V> bool has(Hash<K, V> &h, const K &key) {
    const _internal::FindResult fr = _internal::find(h, key);
    return fr.entry_i != _internal::END_OF_LIST;
}

/// Returns the value associated with the given key if the key exists, otherwise
/// creates a new entry and sets its value to deffault and returns it.
template <typename K, typename V>
const V &set_default(Hash<K, V> &h, const K &key, const V &deffault) {
    _internal::FindResult fr = _internal::find(h, key);
    if (fr.entry_i == _internal::END_OF_LIST) {
        const uint32_t ei = _internal::make(h, key);
        h._entries[ei].value = deffault;
        return h._entries[ei].value;
    }
    return h._entries[fr.entry_i].value;
}

/// Returns the value associated with the given key if the key exists,
/// otherwise returns deffault. Does not add any entry to the table.
template <typename K, typename V>
const V &get_default(Hash<K, V> &h, const K &key, const V &deffault) {
    _internal::FindResult fr = _internal::find(h, key);
    if (fr.entry_i == _internal::END_OF_LIST) {
        return deffault;
    }
    return h._entries[fr.entry_i].value;
}

/// Returns a copy of the key given - useful when the given key and the
/// existing key are equal by operator== but point to different objects.
/// For example:
///     struct K { char* name };
///     K k1 = {"Hey"};
///     K k2; k2 = malloc(4);
///     memcpy(k2.name, k1.name, 4);
/// Now operator== on K might just look at the whole cstring name and return
/// true if they are the same value. But the two strings point to different
/// objects(memory).
template <typename K, typename V>
K get_key(Hash<K, V> &h, const K &key, const K &deffault) {
    _internal::FindResult fr = _internal::find(h, key);
    if (fr.entry_i == _internal::END_OF_LIST) {
        return deffault;
    }
    return h._entries[fr.entry_i].key;
}

/// Removes the entry with the given key if it exists.
template <typename K, typename V> void remove(Hash<K, V> &h, const K &key) {
    _internal::find_and_erase(h, key);
}

/// Returns a constant-entry pointer to the start if the buffer of entries
template <typename K, typename V>
const typename Hash<K, V>::Entry *cbegin(const Hash<K, V> &h) {
    return fo::array::begin(h._entries);
}

/// Returns a constant-entry pointer to one cell past the end of the buffer of
/// entries
template <typename K, typename V>
const typename Hash<K, V>::Entry *cend(const Hash<K, V> &h) {
    return fo::array::end(h._entries);
}

template <typename K, typename V>
uint32_t max_chain_length(const Hash<K, V> &h) {
    uint32_t max_length = 0;

    for (uint32_t i = 0; i < fo::array::size(h._entries); ++i) {
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

} // namespace pod_hash
