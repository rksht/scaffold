/// A lexer for common formats
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "scanner.h"

namespace scanner {

Scanner::Scanner(Array<char>&& text, int mode)
    : _text(std::move(text)), mode(mode),
    line(1), col(1), offset(0), token_start(-1), current_tok(INVALID) {}

Scanner::Scanner(Scanner&& sc)
    : _text(std::move(sc._text)), mode(sc.mode),
    line(sc.line), col(sc.col), offset(sc.offset),
    token_start(sc.token_start), current_tok(sc.current_tok) {}


#define SET_TOK_AND_RET(s, tok) do { (s).current_tok = tok; return tok; } while (0)

int next(Scanner& s)
{
    const char* p = array::begin(s._text);
    const char* e = array::end(s._text);

    p += s.offset;
    if (p == e) {
        SET_TOK_AND_RET(s, EOFS);
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
            SET_TOK_AND_RET(s, EOFS);
        }
    }
    if (*p == '"' && (s.mode & SCAN_STRINGS)) {
        s.token_start = p - array::begin(s._text);
        do {
            if (*p == '\\') {
                ++p;
                ++s.offset;
                ++s.col;
                if (p == e) { break; }
            } else if (*p == '\n') {
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
        SET_TOK_AND_RET(s, STRING);
    }
    if (*p >= '0' && *p <= '9' && (s.mode & (SCAN_INTS | SCAN_FLOATS))) {
        const char* endp;
        int ret = INT;
        s.token_start = p - array::begin(s._text);
        s.current_int = strtol(p, (char**)&endp, 0);
        if (endp != e && *endp == '.' && (s.mode & SCAN_FLOATS)) {
            s.current_float = strtod(p, (char**)&endp);
            ret = FLOAT;
        }
        s.offset = endp - array::begin(s._text);
        s.col += endp - p;
        SET_TOK_AND_RET(s, ret);
    }
    if (((*p >= 'A' &&  *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p == '_'))
            && (s.mode & SCAN_IDENTS)) {
        s.token_start = p - array::begin(s._text);
        do {
            ++p;
            ++s.offset;
            ++s.col;
        } while (p != e && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')
                    || (*p == '_') || (*p >= '0' && *p <= '9')));
        SET_TOK_AND_RET(s, IDENT);
    }
    if (*p == '\n') {
        ++s.line;
        s.col = 0;
    }
    s.token_start = p - array::begin(s._text);
    ++s.offset;
    ++s.col;
    if (*p == ' ' || *p == '\t' || *p == '\n') {
        SET_TOK_AND_RET(s, SPACE);
    }
    SET_TOK_AND_RET(s, *p);
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

void token_text(Scanner& s, Buffer& b)
{
    for (int i = s.token_start; i < s.offset; ++i) {
        b << s._text[i];
    }
}

char* token_text(Scanner& s, Allocator& a)
{
    uint32_t length = s.offset - s.token_start;
    char* buf = (char*) a.allocate(length + 1, 4); // Aligning to 4 bytes in case scratch allocator is used
    memcpy(buf, c_str(s._text) + s.token_start, length);
    buf[length] = 0;
    return buf;
}

void string_token(Buffer& b, Buffer& raw)
{
    for (const char* i = c_str(raw) + 1; *i; ++i) {
        if (i[0] == '\\') {
            char c = i[1];
            switch (c) {
            case 'n':
                b << '\n'; break;
            case 't':
                b << '\t'; break;
            case 'r':
                b << '\r'; break;
            case '"':
                b << '"'; break;
            case '\\':
                b << '\\'; break;
            default:
                b << '\\';
            }
            ++i;
        } else if (i[0] == '"') {
            break;
        } else {
            b << i[0];
        }
    }
}

} // namespace scanner
