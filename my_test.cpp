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

ss::Buffer read_file(FILE* f, Allocator& a)
{
    using namespace ss;
    Buffer text = Buffer(a);
    char c = fgetc(f);
    while (c != EOF) {
        text << c;
        c = fgetc(f);
    }
    return text;
}

// ~1 kb file
char* read_file_cstring(FILE* f, Allocator& a)
{
    using namespace ss;
    char* buf = (char*) a.allocate(1025, 4);
    int i = 0;

    char c = fgetc(f);
    while (c != EOF) {
        buf[i++] = c;
        c = fgetc(f);
    }
    buf[i] = 0;
    return buf;
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

    if (argc != 2) {
        fprintf(stderr, "Expected a file name\n");
        return 1;
    }
    
    mg::init();
    {
        FILE* f = fopen(argv[1], "r");
        /*
        Buffer text = std::move(read_file(f), mg::default_scratch_allocator());
        fclose(f);
        printf("%s\n", c_str(text));
        */
        foundation::TempAllocator2048 ta;
        char* text = read_file_cstring(f, ta);
        fclose(f);
        printf("%s\n", text);
        ta.deallocate(text);
    }
    mg::shutdown();
    return 0;
}
