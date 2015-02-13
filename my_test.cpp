/// Markov chain - a tutorial for using the Bitsquid Foundation library
#include "murmur_hash.h"
#include "hash.h"
#include "string_stream.h"
#include "memory.h"
#include "temp_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <memory>
#include <assert.h>

namespace mg = foundation::memory_globals;
namespace ss = foundation::string_stream;
using foundation::Allocator;

ss::Buffer read_file(FILE* f)
{
    using namespace ss;
    Allocator& alloc = mg::default_allocator();
    Buffer text = Buffer(alloc);
    char c = fgetc(f);
    while (c != EOF) {
        text << c;
        c = fgetc(f);
    }
    return text;
}


struct Student {
    long id;
    long mana;

    Student (long id, long mana)
        : id(id), mana(mana) {}
};


int main(int argc, char** argv)
{
    using namespace ss;
    mg::init();

    if (argc != 2) {
        fprintf(stderr, "Expected a file name\n");
        return 1;
    }
    {
        FILE* f = fopen(argv[1], "r");
        Buffer text = std::move(read_file(f));
        fclose(f);
        printf("%s\n", c_str(text));
    }
    mg::shutdown();
    return 0;
}
