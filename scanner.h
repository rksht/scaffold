#pragma once

#include "memory.h"
#include "array.h"
#include "string_stream.h"

namespace scanner {

using namespace foundation;
using namespace memory_globals;
using namespace string_stream;

const int INT = -1;
const int FLOAT = -2;
const int STRING = -3;
const int IDENT = -4;
const int SPACE = -5;
const int COMMENT = -6;
const int EOFS = -7;
const int INVALID = -8;

enum Modes {
    SCAN_INTS = 1 << -INT,
    SCAN_FLOATS = 1 << -FLOAT,
    SCAN_STRINGS = 1 << -STRING,
    SCAN_IDENTS = 1 << -IDENT,
    SCAN_SPACES = 1 << -SPACE,
    SCAN_COMMENTS = 1 << -COMMENT,
};

const int DEFAULT_MODE = SCAN_INTS | SCAN_FLOATS | SCAN_STRINGS | SCAN_IDENTS;

/// Contains the current state of the scanner
struct Scanner {
    Buffer _text;
    int mode;
    int line;
    int col;
    int offset;
    int token_start;
    int current_tok;
    long current_int;
    double current_float;

    Scanner(Buffer &&text, int mode = DEFAULT_MODE);

    Scanner(Scanner &&sc);
};

/// Returns the next token id
int next(Scanner &s);
/// Returns a description of the given token id
const char *desc(int token);
/// Fills the buffer with the current token text
void token_text(Scanner &s, Buffer &b);
/// Overload of token_text that returns a null terminated c-string
char *token_text(Scanner &s, Allocator &a);
/// A function that takes takes a buffer and a raw string token text
/// and stores the in-memory representation of the string in the buffer.
/// Call this on the token for the STRING type
void string_token(Buffer &b, Buffer &raw);

} // namespace scanner
