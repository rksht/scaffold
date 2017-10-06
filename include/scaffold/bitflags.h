// A type-safe bit flag.
#pragma once

#include <stdint.h>

namespace fo {

template <typename BitType, typename MaskType = uint32_t> struct BitFlags {
    // No warranty if you explcitly access this. Haven't made this private to
    // keep it a trivially-copyable type
    MaskType _mask;

    BitFlags()
        : _mask(0) {}

    // From enum
    BitFlags(BitType bit)
        : _mask(static_cast<MaskType>(bit)) {}

    // Copy
    BitFlags(BitFlags const &rhs)
        : _mask(rhs._mask) {}

    BitFlags &operator=(BitFlags const &rhs) {
        _mask = rhs._mask;
        return *this;
    }

    BitFlags &operator|=(BitFlags const &rhs) {
        _mask |= rhs._mask;
        return *this;
    }

    BitFlags &operator&=(BitFlags const &rhs) {
        _mask &= rhs._mask;
        return *this;
    }

    BitFlags &operator^=(BitFlags const &rhs) {
        _mask ^= rhs._mask;
        return *this;
    }

    BitFlags operator|(BitFlags const &rhs) const {
        BitFlags result(*this);
        result |= rhs;
        return result;
    }

    BitFlags operator&(BitFlags const &rhs) const {
        BitFlags result(*this);
        result &= rhs;
        return result;
    }

    BitFlags operator^(BitFlags const &rhs) const {
        BitFlags result(*this);
        result ^= rhs;
        return result;
    }

    bool operator!() const { return !_mask; }

    BitFlags operator~() const {
        BitFlags result(*this);
        result._mask ^= FlagTraits<BitType>::allBitFlags;
        return result;
    }

    bool operator==(BitFlags const &rhs) const { return _mask == rhs._mask; }

    bool operator!=(BitFlags const &rhs) const { return _mask != rhs._mask; }

    explicit operator bool() const { return !!_mask; }

    explicit operator MaskType() const { return _mask; }
};

} // namespace fo
