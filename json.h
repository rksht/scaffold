#include "memory.h"
#include "scanner.h"
#include "pod_hash.h"
#include "array.h"
#include "murmur_hash.h"
#include <string.h>
#include <memory>
#include <utility>
#include <stdio.h>

namespace json {

namespace fo = foundation;
namespace mg = fo::memory_globals;
namespace ss = fo::string_stream;

// The allocator
#define OBJECT_ALLOCATOR (mg::default_allocator())

class Value;
class Object;
class Number;
class String;
class Array;

class VisitorIF {
  public:
    virtual void visit(Object &ob) = 0;
    virtual void visit(Number &n) = 0;
    virtual void visit(String &s) = 0;
    virtual void visit(Array &a) = 0;
};

class Value {
  public:
    virtual ~Value() {}

    virtual void visit(VisitorIF &v) = 0;
};

class Parser;

// cstring hash
unsigned long cstr_hash(char *const *s);
// cstring equal
bool cstr_equal(char *const &s1, char *const &s2);

class Object : public Value {
  private:
    pod_hash::Hash<char *, Value *> _map;

    friend class Parser;

  public:
    using map_type = pod_hash::Hash<char *, Value *>;

    Object()
        : _map(OBJECT_ALLOCATOR, OBJECT_ALLOCATOR, cstr_hash, cstr_equal) {}

    Object(Object &&other) : _map(std::move(other._map)) {}

    ~Object() {
        // Moved or not.
        if (_map._hashes._data == nullptr)
            return;

        // Delete all the keys and call destructor on the values
        for (map_type::Entry const *ep = pod_hash::cbegin(_map);
             ep != pod_hash::cend(_map); ep++) {
            OBJECT_ALLOCATOR.deallocate(
                ep->key); // Just a char pointer, no destructor for it
            MAKE_DELETE(OBJECT_ALLOCATOR, Value, ep->value);
        }
    }

  private:
    // Returns false if key already exists, otherwise returns false.
    bool _add_key_value(char *key, Value *value) {
        if (pod_hash::has(_map, key)) {
            return false;
        }
        pod_hash::set(_map, key, value);
        return true;
    }

  public:
    const map_type::Entry *cbegin() { return pod_hash::cbegin(_map); }

    const map_type::Entry *cend() { return pod_hash::cend(_map); }

    void visit(VisitorIF &v) override { v.visit(*this); }
};

class Array : public Value {
  private:
    fo::Array<Value *> _arr;

    friend class Parser;

  public:
    Array() : _arr(OBJECT_ALLOCATOR) {}

    ~Array() {
        for (Value **vp = fo::array::begin(_arr); vp != fo::array::end(_arr);
             vp++) {
            MAKE_DELETE(OBJECT_ALLOCATOR, Value, *vp);
        }
    }

    Value *const *cbegin() const { return fo::array::begin(_arr); }

    Value *const *cend() const { return fo::array::end(_arr); }

    void visit(VisitorIF &v) override { v.visit(*this); }
};

class String : public Value {
  private:
    ss::Buffer _buf;
    friend class Parser;

  public:
    String(ss::Buffer &&buf) : _buf(std::move(buf)) {}

    ss::Buffer &get_buffer() { return _buf; }

    void visit(VisitorIF &v) { v.visit(*this); }
};

class Number : public Value {
  private:
    double _num;

  public:
    Number(double num) : _num(num) {}

    void visit(VisitorIF &v) { v.visit(*this); }
};

class Parser {
  private:
    scanner::Scanner _sc;

  public:
    Parser(scanner::Scanner &&sc) : _sc(std::move(sc)) {}

  private:
    Value *number();
    Value *string();
    char *key_string();
    Value *array();
    std::pair<char *, Value *> key_val_pair();
    Value *object();
    Value *value();

    Value *error(const char *fmt, ...);

  public:
    Object *parse();
};
} // ns json