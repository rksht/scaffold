## A set of C++ "things" thrown into a cauldron.

Almost always in progress. Meant to be easily extensible.

* Foundation library by Bitsquid plays a big part which resides in the top
  level.

* A scanner for tokenizing input.

* Some more allocators.

* Some more data structures.

## Notes

The containers in `collection_types.h`

- `Array<T>`
- `Queue<T>`
- `Hash<T>`

are mostly the same as in `foundation` library. `Array<T>` is made to work
with `rvalue` references

The `pod_hash.h` file contains a generalized implementation of `Hash<T>`,
where the key can be any POD type.

The memory allocator and global variables are in the files `memory.h`,
`memory.cpp`, `arena_allocator.h`, `arena_allocator.cpp`, `buddy_allocator.h`.
