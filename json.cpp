#include "json.h"

namespace json {

// cstring hash
unsigned long cstr_hash(char *const *s) {
    unsigned l = strlen(*s);
    return (unsigned long)fo::murmur_hash_64(*s, l, 0xDEADBEEF);
}

// cstring equal
bool cstr_equal(char *const &s1, char *const &s2) {
    return strcmp(s1, s2) == 0;
}

Value *Parser::error(const char *fmt, ...) {
    ss::Buffer text(mg::default_allocator());
    scanner::token_text(_sc, text);
    fprintf(stderr, "Parse error: %s (at line[%d]:%d:%d)  ", ss::c_str(text),
            _sc.line, _sc.col - fo::array::size(text), _sc.col);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    return nullptr;
}

Object *Parser::parse() {
    int t = scanner::next(_sc);
    if (t != '{') {
        return static_cast<Object *>(error("%s", "Expected '{' for object"));
    }
    return static_cast<Object *>(object());
}

Value *Parser::number() {
    double n = _sc.current_tok == scanner::INT ? (double)_sc.current_int
                                               : _sc.current_float;
    scanner::next(_sc);
    return MAKE_NEW(OBJECT_ALLOCATOR, Number, n);
}

Value *Parser::string() {
    ss::Buffer str(OBJECT_ALLOCATOR);
    ss::Buffer src(OBJECT_ALLOCATOR);
    scanner::token_text(_sc, src);
    scanner::string_token(str, src);
    String *string = MAKE_NEW(OBJECT_ALLOCATOR, String, std::move(str));
    scanner::next(_sc);
    return string;
}

char *Parser::key_string() {
    ss::Buffer str(OBJECT_ALLOCATOR);
    ss::Buffer src(OBJECT_ALLOCATOR);
    scanner::token_text(_sc, src);
    scanner::string_token(str, src);
    scanner::next(_sc);
    return ss::c_str(std::move(str));
}

Value *Parser::array() {
    Array *arr = MAKE_NEW(OBJECT_ALLOCATOR, Array);
    if (scanner::next(_sc) == ']') {
        scanner::next(_sc);
        return arr;
    }
    while (true) {
        Value *v = value();
        if (!v) {
            return nullptr;
        }
        fo::array::push_back(arr->_arr, v);
        if (_sc.current_tok == ',') {
            scanner::next(_sc);
            continue;
        } else if (_sc.current_tok == ']') {
            scanner::next(_sc);
            break;
        } else {
            return error("%s", "Expected ',' or ']' while parsing array");
        }
    }
    return arr;
}

std::pair<char *, Value *> Parser::key_val_pair() {
    char *key = key_string();
    if (_sc.current_tok != ':') {
        error("%s", "Expected :");
        return std::pair<char *, Value *>(nullptr, nullptr);
    }
    scanner::next(_sc);
    Value *val = value();
    return std::pair<char *, Value *>(key, val);
}

Value *Parser::object() {
    int token = scanner::next(_sc);
    if (token == '}') {
        scanner::next(_sc);
        return MAKE_NEW(OBJECT_ALLOCATOR, Object);
    }
    if (token != scanner::STRING) {
        return error("%s", "Expected a string while parsing json key");
    }
    Object *ob = MAKE_NEW(OBJECT_ALLOCATOR, Object);
    while (true) {
        std::pair<char *, Value *> key_val = key_val_pair();
        if (key_val.first == nullptr) {
            return nullptr;
        }
        ob->_add_key_value(key_val.first, key_val.second);
        if (_sc.current_tok == '}') {
            scanner::next(_sc);
            break;
        }
        if (_sc.current_tok == ',') {
            scanner::next(_sc);
            continue;
        }
        // Error
        ob->~Object();
        return error("%s", "Expected ',' or '}' at end of key value pair");
    }
    return ob;
}

Value *Parser::value() {
    switch (_sc.current_tok) {
    case scanner::INT:
    case scanner::FLOAT:
        return number();
    case scanner::STRING:
        return string();
    case '[':
        return array();
    case '{':
        return object();
    case scanner::EOFS:
        return error("%s", "Unexpected end of string");
    }
    return error("%s", "Unexpected token");
}

} // ns json