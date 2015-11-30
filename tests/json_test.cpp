#include <stdio.h>
#include <iostream>
#include <sstream>

#include "memory.h"
#include "scanner.cpp"
#include "json.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

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
template <typename StreamTy> class PrintVisitor : public json::VisitorIF {
  private:
    StreamTy &_stream;

  public:
    PrintVisitor(StreamTy &stream) : _stream(stream) {}

    void visit(json::Object &ob) override;
    void visit(json::Array &arr) override;
    void visit(json::Number &num) override;
    void visit(json::String &str) override;
};

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Number &num) {
    _stream << num.get();
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::String &str) {
    _stream << "\"" << ss::c_str(str.get()) << "\"";
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Array &arr) {
    _stream << "[";
    for (auto it = fo::array::begin(arr.get()); it != fo::array::end(arr.get()); it++) {
        (*it)->visit(*this);
        if (it == fo::array::end(arr.get()) - 1)
            continue;
        _stream << ",";
    }
    _stream << "]";
}

template <typename StreamTy>
void PrintVisitor<StreamTy>::visit(json::Object &ob) {
    _stream << "{";
    for (auto it = pod_hash::cbegin(ob.get()); it != pod_hash::cend(ob.get()); it++) {
        _stream << "\"" << it->key << "\"";
        _stream << ":";
        it->value->visit(*this);
        if (it == pod_hash::cend(ob.get()) - 1)
            continue;
        _stream << ",";
    }
    _stream << "}";
}

TEST_CASE("Loaded json correctly", "[json loading]") {
    mg::init();

    std::ostringstream out;
    PrintVisitor<std::ostringstream> pv(out);
    {
        using namespace ss;
        Buffer text(mg::default_allocator());
        text << "{\"name\": \"Yennefer\", \"designation\": "
                "\"Sorceress\",\"attrs\": [90, 90, 80]}";
        json::Parser p(std::move(scanner::Scanner(std::move(text))));
        json::Object &ob = *p.parse();
        pv.visit(ob);
        REQUIRE(out.str() == "{\"name\":\"Yennefer\",\"designation\":"
                             "\"Sorceress\",\"attrs\":[90,90,80]}");
        MAKE_DELETE(OBJECT_ALLOCATOR, Object, &ob);
    }

    mg::shutdown();
}

/*
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

    std::ostringstream out;
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

        ob.add_key_value(c_str(std::move(name)), MAKE_NEW(OBJECT_ALLOCATOR,
json::String, "Yennefer"));
        ob.add_key_value(c_str(std::move(designation)),
MAKE_NEW(OBJECT_ALLOCATOR, json::String, "Sorceress"));
        auto arr = MAKE_NEW(OBJECT_ALLOCATOR, json::Array);
        fo::array::push_back(arr->get_array(),
static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 90)));
        fo::array::push_back(arr->get_array(),
static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 90)));
        fo::array::push_back(arr->get_array(),
static_cast<json::Value*>(MAKE_NEW(OBJECT_ALLOCATOR, json::Number, 80)));
        ob.add_key_value(c_str(std::move(attrs)), arr);
        pv.visit(ob);
        std::cout << std::endl;
    }
    mg::shutdown();
}
*/
