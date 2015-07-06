#pragma once

#include "memory.h"
#include "scanner.h"
#include "pod_hash.h"
#include "array.h"
#include "murmur_hash.h"
#include "pod_hash_usuals.h"
#include <string.h>
#include <memory>
#include <utility>
#include <stdio.h>

// This is a very simple json loader. But the getters for the objects
// return non-const references to the data structures used, so it can be used to
// make up a json value using the usual array, hash, string_stream, etc.
// functions.

namespace json {

namespace fo = foundation;
namespace mg = fo::memory_globals;
namespace ss = fo::string_stream;

// The allocator
#define OBJECT_ALLOCATOR (mg::default_allocator())

// The json values
class Object;
class Number;
class String;
class Array;

// Visitor interface class
class VisitorIF {
  public:
    virtual void visit(Object &ob) = 0;
    virtual void visit(Number &n) = 0;
    virtual void visit(String &s) = 0;
    virtual void visit(Array &a) = 0;
};

// A json value
class Value {
  public:
    virtual ~Value() {}

    virtual void visit(VisitorIF &v) = 0;
};

class Parser;

class Object : public Value {
  public:
    using map_type = pod_hash::Hash<char *, Value *>;

  private:
    map_type _map;
    bool _keys_owned; // Are the keys of the map owned by the Object

    friend class Parser;

  public:
    Object(bool keys_owned)
        : _map(OBJECT_ALLOCATOR, OBJECT_ALLOCATOR, pod_hash::usual_hash<char*>,
               pod_hash::usual_equal<char*>),
          _keys_owned(keys_owned) {}

    Object(Object &&other)
        : _map(std::move(other._map)), _keys_owned(other._keys_owned) {}

    ~Object() {
        // Moved or not.
        if (_map._hashes._data == nullptr)
            return;

        // Delete all the keys and call destructor on the values
        for (map_type::Entry const *ep = pod_hash::cbegin(_map);
             ep != pod_hash::cend(_map); ep++) {
            // Just a char pointer, no destructor for it
            if (_keys_owned)
                OBJECT_ALLOCATOR.deallocate(ep->key);
            MAKE_DELETE(OBJECT_ALLOCATOR, Value, ep->value);
        }
    }

  public:
    // Returns the map
    map_type &get_map() { return _map; }
    // For iterating the map directly
    const map_type::Entry *cbegin() const { return pod_hash::cbegin(_map); }
    const map_type::Entry *cend() const { return pod_hash::cend(_map); }

    // Returns false if key already exists, otherwise adds the given value and
    // returns true.
    bool add_key_value(char *key, Value *value) {
        if (pod_hash::has(_map, key)) {
            return false;
        }
        pod_hash::set(_map, key, value);
        return true;
    }

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

    fo::Array<Value *> &get_array() { return _arr; }

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

    String(const char *cstr) : _buf(OBJECT_ALLOCATOR) {
        using namespace ss;
        _buf << cstr;
    }

    ss::Buffer &get_buffer() { return _buf; }

    const char *get_cstr() { return ss::c_str(_buf); }

    void visit(VisitorIF &v) { v.visit(*this); }
};

class Number : public Value {
  private:
    double _num;

  public:
    Number(double num) : _num(num) {}

    double &get_number() { return _num; }

    void visit(VisitorIF &v) { v.visit(*this); }
};

class Parser {
  private:
    scanner::Scanner _sc;

  public:
    Parser(scanner::Scanner &&sc) : _sc(std::move(sc)) {}

  private:
    Value *_number();
    Value *_string();
    char *_key_string();
    Value *_array();
    std::pair<char *, Value *> _key_val_pair();
    Value *_object();
    Value *_value();

    Value *_error(const char *fmt, ...);

  public:
    Object *parse();
};
} // ns json
