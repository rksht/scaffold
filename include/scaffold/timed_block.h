#pragma once

#include <scaffold/debug.h>

#include <limits>
#include <stdio.h>
#include <type_traits>

#ifndef TIMED_BLOCK_CAPACITY
#define TIMED_BLOCK_CAPACITY 128
#endif

#ifndef TIMED_BLOCK_COUNT_TO_KEEP
#define TIMED_BLOCK_COUNT_TO_KEEP 5
#endif

static_assert((TIMED_BLOCK_CAPACITY & (TIMED_BLOCK_CAPACITY - 1)) == 0, "Must be power of 2");

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define DEFER(C) auto TOKENPASTE2(deferred_statement_, __LINE__) = make_deferred(C)

#define TIMED_BLOCK                                                                                          \
    auto TOKENPASTE2(timed_block_variable, __LINE__) = timedblock::get_table().add_on_entry(                 \
        __FILE__, __func__, __PRETTY_FUNCTION__, __LINE__, timedblock::get_timestamp())

namespace timedblock {

__attribute__((always_inline)) inline uint64_t get_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_nsec;
}

struct Record {
    // Key portion.

    // Pointer to the __func__ variable. This will be unique for each function therefore.
    const char *func_pointer;

    // File name
    const char *filename;

    // Line number
    u32 line;

    // "Value" portion

    uint64_t entry_timestamp;
    uint64_t times_entered;
    uint64_t max_time_spent[TIMED_BLOCK_COUNT_TO_KEEP + 1]; // +1 makes the insertion sort easier
    uint64_t total_time_spent;

    // Function name is for showing the result in an informative way only. Not used for comparison.
    const char *function_name;
};

inline bool is_nil_record(const Record &rec) { return rec.func_pointer == 0; }

struct RecordScope {
    Record *_rec;
    uint16_t _rec_prev;

    RecordScope(Record &rec)
        : _rec(&rec) {}

    RecordScope(const RecordScope &o) = delete;

    RecordScope(RecordScope &&o)
        : _rec(o._rec)
        , _rec_prev(o._rec_prev) {
        o._rec = nullptr;
    }

    RecordScope &operator=(const RecordScope &) = delete;

    ~RecordScope() {
        if (!_rec) {
            return;
        }

        auto exit_timestamp = get_timestamp();

        auto time_spent =
            exit_timestamp > _rec->entry_timestamp ? (exit_timestamp - _rec->entry_timestamp) : 0;

        auto &arr = _rec->max_time_spent;

        arr[TIMED_BLOCK_COUNT_TO_KEEP] = time_spent;

        int i = TIMED_BLOCK_COUNT_TO_KEEP;

        while (i > 0 && arr[i] >= arr[i - 1]) {
            std::swap(arr[i], arr[i - 1]);
            --i;
        }
        _rec->total_time_spent += time_spent;
    }
};

struct RecordTable {
    Record _records[TIMED_BLOCK_CAPACITY];

    RecordTable() { reset(); }

    void reset() { memset(_records, 0, sizeof(_records)); }

    RecordScope add_on_entry(const char *filename,
                             const char *func_pointer,
                             const char *function_name,
                             u32 line,
                             uint64_t timestamp) {
        u64 hash = ((u64)filename + (u64)func_pointer) & ((u64)func_pointer + (line & 7));
        u64 index = hash & (TIMED_BLOCK_CAPACITY - 1);

        u32 count = 0;

        while (count < TIMED_BLOCK_CAPACITY) {
            auto &rec = _records[index];

            const bool nil = is_nil_record(rec);

            if (!nil && (rec.func_pointer != func_pointer || rec.line != line)) {
                index = (index + 1) & (TIMED_BLOCK_CAPACITY - 1);
                ++count;

                continue;
            }

            // Empty slot or found previously added
            if (nil) {
                rec.filename = filename;
                rec.func_pointer = func_pointer;
                rec.line = line;
                rec.function_name = function_name;
                rec.times_entered = 1;
                rec.entry_timestamp = timestamp;
                rec.total_time_spent = 0;
                std::fill(rec.max_time_spent, rec.max_time_spent + TIMED_BLOCK_COUNT_TO_KEEP, 0);
            } else {
                // Set new timestamp of entry
                rec.entry_timestamp = timestamp;
                ++rec.times_entered;
            }

            return RecordScope(rec);
        }

        log_assert(false, "Failed to add/find function and block %s:%u", func_pointer, line);
    }
};

RecordTable &get_table();

inline void reset() { get_table().reset(); }

void print_record_table(FILE *f);

} // namespace timedblock
