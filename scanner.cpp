/// A lexer for common formats
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "scanner.h"

namespace scanner {

Scanner::Scanner(Array<char>&& text, int mode, Allocator& token_text_allocator)
    : _text(std::move(text)), _token_text(token_text_allocator),  mode(mode),
    line(1), col(1), offset(0), _token_start(-1) {}

double _current_float;
int _current_int;

int next(Scanner& s)
{
    const char* p = array::begin(s._text);
    const char* e = array::end(s._text);

    p += s.offset;
    if (p == e) {
        return EOFS;
    }
    if (!(s.mode & SCAN_SPACES)) {
        while (p != e && (*p == ' ' || *p == '\t' || *p == '\n')) {
            if (*p == '\n') {
                ++s.line;
                s.col = 0;
            }
            ++p;
            ++s.offset;
            ++s.col;
        }
        if (p == e) {
            return EOFS;
        }
    }
    if (*p == '"' && (s.mode & SCAN_STRINGS)) {
        s._token_start = p - array::begin(s._text);
        do {
            if (*p == '\n') {
                ++s.line;
                s.col = 0;
            }
            ++p;
            ++s.offset;
            ++s.col;
        } while (p != e && *p != '"');
        if (p == e) {
            return INVALID + STRING;
        }
        ++s.col;
        ++s.offset;
        return STRING;
    }
    if (*p >= '0' && *p <= '9' && (s.mode & (SCAN_INTS | SCAN_FLOATS))) {
        const char* endp;
        int ret = INT;
        s._token_start = p - array::begin(s._text);
        _current_int = (int) strtol(p, (char**)&endp, 0);
        if (endp != e && *endp == '.' && (s.mode & SCAN_FLOATS)) {
            _current_float = strtod(p, (char**)&endp);
            ret = FLOAT;
        }
        s.offset = endp - array::begin(s._text);
        s.col += endp - p;
        return ret;
    }
    if (((*p >= 'A' &&  *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p == '_'))
            && (s.mode & SCAN_IDENTS)) {
        s._token_start = p - array::begin(s._text);
        do {
            ++p;
            ++s.offset;
            ++s.col;
        } while (p != e && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')
                    || (*p == '_') || (*p >= '0' && *p <= '9')));
        return IDENT;
    }
    if (*p == '\n') {
        ++s.line;
        s.col = 0;
    }
    s._token_start = p - array::begin(s._text);
    ++s.offset;
    ++s.col;
    if (*p == ' ' || *p == '\t' || *p == '\n') {
        return SPACE;
    }
    return p[-1];
}

char char_token[] = "INVALID";

const char* desc(int token)
{
    if (token <= INVALID) {
        sprintf(char_token, "%s", "INVALID");
        return char_token;
    }

    switch (token) {
    case INT:
        return "INT";
    case FLOAT:
        return "FLOAT";
    case STRING:
        return "STRING";
    case IDENT:
        return "IDENT";
    case SPACE:
        return "SPACE";
    case COMMENT:
        return "COMMENT";
    default:
        snprintf(char_token, 1, "%c", (char)token);
        return char_token;
    }
}

Buffer token_text(Scanner& s)
{
    array::clear(s._token_text);
    for (int i = s._token_start; i < s.offset; ++i) {
        s._token_text << s._text[i];
    }
    return s._token_text;
}

float current_float() {
    return _current_float;
}

int current_int() {
    return _current_int;
}

} // namespace scanner
