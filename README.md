## A set of C++ "things" thrown into a cauldron.

Almost always in progress. Meant to be easily modifiable and understandable.

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

are mostly the same as in `foundation` library. `Array<T>` is made to support
move semantics.

The `pod_hash.h` file contains a generalized implementation of `Hash<T>`,
where the key can be any POD type. Hash functions for different types are
defined (and should be defined) in `pod_hash_usuals.h`

The memory allocators and global variables are in the files `memory.h`,
`memory.cpp`, `arena_allocator.h`, `arena_allocator.cpp`, `buddy_allocator.h`,
`temp_allocator.h`, `temp_allocator.cpp`.

File `rbt.h` contains a red-black tree.

File `scanner.h` and `scanner.cpp` contain a 'usual' lexer (for ASCII only).

And some other stuff.