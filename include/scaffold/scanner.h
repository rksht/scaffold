#pragma once

#include "array.h"
#include "memory.h"
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
const int ESCAPE = -8;
const int INVALID = -9;

enum Modes {
    SCAN_INTS = 1 << -INT,
    SCAN_FLOATS = 1 << -FLOAT,
    SCAN_STRINGS = 1 << -STRING,
    SCAN_IDENTS = 1 << -IDENT,
    SCAN_SPACES = 1 << -SPACE,
    SCAN_COMMENTS = 1 << -COMMENT,
    SCAN_ESCAPES = 1 << -ESCAPE, // This will inteprete escape codes all
                                 // throughout the string
};

/// This default mode is like scanning most simple languages
const int DEFAULT_MODE = SCAN_INTS | SCAN_FLOATS | SCAN_STRINGS | SCAN_IDENTS;

/// This mode treats the whole text as if being inside a string literal. It
/// scans characters one by one and also interpretes escape codes.
const int WHOLESTRING_MODE = SCAN_SPACES | SCAN_ESCAPES;

/// Contains the current state of the scanner
struct Scanner {
    mutable Buffer _text;
    int mode;
    int line;
    int col;
    int offset;
    int token_start;
    int current_tok;
    long current_int;
    double current_float;

    Scanner(Buffer text, int mode = DEFAULT_MODE);

    Scanner(Scanner &&sc);
};

/// Returns the next token id
int next(Scanner &s);
/// Returns a description of the given token id
const char *desc(int token);
/// Fills the buffer with the current token text
void token_text(const Scanner &s, Buffer &b);
/// Overload of token_text that returns a null terminated c-string
char *token_text(const Scanner &s, Allocator &a);

/// Pushes into `b` all the characters from `raw` while decoding the escape
/// codes in `raw` (e.g - The sequence of characters '\' and 'n' in `raw` will
/// be pushed as a single newline character into `b`.
void string_token(Buffer &b, Buffer &raw);

} // namespace scanner
