#include <stdio.h>
#include <iostream>

#include "memory.h"
#include "scanner.cpp"
#include "json.h"

namespace ss = foundation::string_stream;
namespace mg = foundation::memory_globals;
namespace fo = foundation;

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

#include "array.h"

// A printing visitor for json values
template <typename StreamTy>
class PrintVisitor : public json::VisitorIF {
private:
    StreamTy& _stream;
public:
    PrintVisitor(StreamTy& stream) : _stream(stream) {}

    void visit(json::Object& ob) override;
    void visit(json::Array& arr) override;
    void visit(json::Number& num) override;
    void visit(json::String& str) override;
};

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Number& num) {
    _stream << num.get_number();
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::String& str) {
    _stream << str.get_cstr();
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Array& arr) {
    _stream << "[";
    for (auto it = arr.cbegin(); it != arr.cend(); it++) {
        (*it)->visit(*this);
        if (it == arr.cend() - 1) continue;
        _stream << ", ";
    }
    _stream << "]";
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Object& ob) {
    _stream << "{";
    for (auto it = ob.cbegin(); it != ob.cend(); it++) {
        _stream << it->key;
        _stream << ": ";
        it->value->visit(*this);
        if (it == ob.cend() - 1) continue;
        _stream << ", ";
    }
    _stream << "}";
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

    PrintVisitor<decltype(std::cout)> pv(std::cout);

    {
        ss::Buffer text(std::move(read_file(f)));
        if (f != stdin) {
            fclose(f);
        }
        scanner::Scanner s(std::move(text));
        json::Object *ob = json::Parser(std::move(s)).parse();
        json::Object ob1(std::move(*ob));
        pv.visit(ob1);
        MAKE_DELETE(OBJECT_ALLOCATOR, Object, ob);
        std::cout << std::endl;
    }

    {
        // Building a json value 
        // {"name": "Yennefer", "designation": "Sorceress",
        // "attrs": [90, 90, 80]}
        json::Object ob(true);

        using namespace ss;

        Buffer name(OBJECT_ALLOCATOR);
        name << "name";

        Buffer designation(OBJECT_ALLOCATOR);
        designation << "designation";

        Buffer attrs(OBJECT_ALLOCATOR);
        attrs << "attrs";

        ob.add_key_value(c_str(std::move(name)), MAKE_NEW(OBJECT_ALLOCATOR, json::String, "Yennefer"));
        ob.add_key_value(c_str(std::move(designation)), MAKE_NEW(OBJECT_ALLOCATOR, json::String, "Sorceress"));
        auto arr = MAKE_NEW(OBJECT_ALLOCATOR, json::Array);
        fo::array::push_back(arr->get_array(), static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 90)));
        fo::array::push_back(arr->get_array(), static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 90)));
        fo::array::push_back(arr->get_array(), static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 80)));
        ob.add_key_value(c_str(std::move(attrs)), arr);
        pv.visit(ob);
        std::cout << std::endl;
    }
    mg::shutdown();
}
