#include <stdio.h>

#include "memory.h"
#include "scanner.cpp"

namespace ss = foundation::string_stream;
namespace mg = foundation::memory_globals;

ss::Buffer read_file(FILE* f)
{
    using namespace ss;
    foundation::Allocator& alloc = mg::default_allocator();
    Buffer text = Buffer(alloc);
    char c = fgetc(f);
    while (c != EOF) {
        text << c;
        c = fgetc(f);
    }
    return text;
}


int main()
{
    mg::init();

    {
        ss::Buffer text(std::move(read_file(stdin)));
        scanner::Scanner s(std::move(text));

        int token = scanner::next(s);

        while (token != scanner::EOFS) {
            ss::Buffer token_text = scanner::token_text(s);
            printf("%s %d:%d - %s\n", scanner::desc(token), s.line, s.col,
                    ss::c_str(token_text));
            token = scanner::next(s);
        }
    }
    mg::shutdown();
}
