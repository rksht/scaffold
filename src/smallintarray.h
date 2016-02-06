#pragma once

#include "const_log.h"
#include "string.h"
#include <array>
#include <stdio.h>

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
        printf("Initialized small ints with %u bits\n", bits_per_int);
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

    /// A forward const iterator
    const_iterator begin() const { return const_iterator(_words.data(), 0); }

    const_iterator end() const {
        return const_iterator(_words.data() + _num_words, 0);
    }

    void print() const {
        int n = 0;
        for (auto i = this->begin(), e = this->end(); i != e; ++i) {
            printf("%d = %u\n", n++, *i);
        }
    }
};
