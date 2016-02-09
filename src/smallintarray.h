#pragma once

#include "const_log.h"
#include "string.h"
#include "debug.h"
#include "string_stream.h"
#include <array>
#include <algorithm>
#include <stdio.h>
#include <assert.h>

namespace foundation {

/// A data type that holds `num_int` packed unsigned integers which are
/// representable with `bits_per_int` bits. Uses 32 bit unsigned to store the
/// "small" integers, so `bits_per_int` must be <= 32
template <unsigned bits_per_int, unsigned num_ints> struct SmallIntArray {
  private:
    static_assert(bits_per_int <= 32,
                  "SmallIntArray uses 32 bit ints to store the small ints");

    constexpr static unsigned _ints_per_word =
        sizeof(uint32_t) * 8 / bits_per_int;

    constexpr static unsigned _num_words = ceil_div(num_ints, _ints_per_word);

    /// Array to store the stuff
    std::array<uint32_t, _num_words> _words;

    constexpr static uint32_t _front_mask() { return (1 << bits_per_int) - 1; }

    uint32_t _word(unsigned idx) const {
        const unsigned word_idx = idx / _ints_per_word;
        return _words[word_idx];
    }

    uint32_t &_word(unsigned idx) {
        const unsigned word_idx = idx / _ints_per_word;
        return _words[word_idx];
    }

    constexpr static uint32_t _mask(unsigned offset) {
        return _front_mask() << (offset * bits_per_int);
    }

  public:
    class const_iterator {
      private:
        const uint32_t *_word;
        uint32_t _offset;

      public:
        const_iterator() = delete;

        const_iterator(const const_iterator &other)
            : _word(other._word), _offset(other._offset) {}

        const_iterator(const uint32_t *word, uint32_t offset)
            : _word(word), _offset(offset) {}

        const_iterator &operator++() {
            if (_offset + 1 == _ints_per_word) {
                ++_word;
                _offset = 0;
            } else {
                _offset += 1;
            }
            return *this;
        }

        bool operator==(const const_iterator &other) {
            return _word == other._word && _offset == other._offset;
        }

        bool operator!=(const const_iterator &other) {
            return !(*this == other);
        }

        uint32_t operator*() {
            return (*_word & _mask(_offset)) >> (_offset * bits_per_int);
        }
    };

  public:
    /// Ctor - sets all to 0
    SmallIntArray() {
        memset(_words.data(), 0, sizeof(uint32_t) * _words.size());
    }

    /// Returns the `idx`-th integer
    uint32_t get(unsigned idx) const {
        const unsigned offset = idx % _ints_per_word;
        return (_word(idx) & _mask(offset)) >> (offset * bits_per_int);
    }

    /// Sets the `idx`-th integer
    void set(unsigned idx, uint32_t the_int) {
        const unsigned offset = idx % _ints_per_word;
        uint32_t word = _word(idx) & ~(_mask(offset));
        _word(idx) = word | (the_int << (offset * bits_per_int));
    }

    /// Sets the given range of indices to a given integer
    void set_range(unsigned begin_idx, unsigned end_idx, uint32_t the_int) {
        if (begin_idx >= end_idx) {
            return;
        }

        // The word at which begin_idx is located in
        const unsigned begin_word_pos = begin_idx / _ints_per_word;
        // The offset in that word where `begin_idx` is located in
        const unsigned begin_word_offset = begin_idx % _ints_per_word;
        // ditto for `end_idx`
        const unsigned end_word_pos = end_idx / _ints_per_word;
        /*const unsigned end_word_offset = end_idx % _ints_per_word;*/

        // If begin and end are in the same word, or two consecutive words set
        // them one by one
        if ((int64_t)end_word_pos - (int64_t)begin_word_pos <= 1) {
            for (unsigned idx = begin_idx; idx < end_idx; ++idx) {
                set(idx, the_int);
            }
            return;
        }

        // Otherwise set the begin word and end words first...
        for (unsigned idx = begin_idx,
                      e = begin_idx + (_ints_per_word - begin_word_offset);
             idx < e; ++idx) {
            set(idx, the_int);
        }

        for (unsigned idx = end_word_pos * _ints_per_word; idx < end_idx;
             ++idx) {
            set(idx, the_int);
        }

        // Calculate the word once and set all the middle words
        uint32_t n = 0;
        for (unsigned offset = 0; offset < _ints_per_word; ++offset) {
            n = (n & ~(_mask(offset))) | (the_int << (offset * bits_per_int));
        }

        for (unsigned word_pos = begin_word_pos + 1; word_pos < end_word_pos;
             ++word_pos) {
            _words[word_pos] = n;
        }
    }

    /// A forward const iterator
    const_iterator begin() const { return const_iterator(_words.data(), 0); }

    const_iterator end() const {
        return const_iterator(_words.data() + _num_words, 0);
    }

    void print() const {
        int n = 0;
        using namespace string_stream;

        Buffer b(memory_globals::default_allocator());

        for (auto i = this->begin(), e = this->end(); i != e; ++i) {
            b << n << " = " << *i << "\t";
            tab(b, 8);
            ++n;
            if (array::size(b) >= 80) {
                ::printf("%s\n", c_str(b));
                array::clear(b);
            }
        }
        if (array::size(b) != 0) {
            ::printf("%s\n", c_str(b));
        }
    }
};
}