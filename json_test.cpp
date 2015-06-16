#include <stdio.h>

#include "memory.h"
#include "scanner.cpp"
#include "json.h"

namespace ss = foundation::string_stream;
namespace mg = foundation::memory_globals;

ss::Buffer read_file(FILE *f) {
    using namespace ss;
    foundation::Allocator &alloc = mg::default_allocator();
    Buffer text = Buffer(alloc);
    char c = fgetc(f);
    while (c != EOF) {
        text << c;
        c = fgetc(f);
    }
    return text;
}

int main(int argc, char **argv) {
    mg::init();

    FILE *f = stdin;

    if (argc > 1) {
        f = fopen(argv[1], "r");
        if (!f) {
            fprintf(stderr, "Failed to open file %s\n", argv[1]);
            return 1;
        }
    }

    {
        ss::Buffer text(std::move(read_file(f)));
        if (f != stdin) {
            fclose(f);
        }
        scanner::Scanner s(std::move(text));
        json::Object *ob = json::Parser(std::move(s)).parse();
        json::Object ob1(std::move(*ob));
        MAKE_DELETE(OBJECT_ALLOCATOR, Object, ob);
    }

    mg::shutdown();
}
