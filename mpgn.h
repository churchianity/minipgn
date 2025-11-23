/*

Quick Start
--------------------------------------------------------------------------------

Do this:
    #define MINIPGN_IMPLEMENTATION
    #include "mpgn.h"
before you include this file in *one* C or C++ file to create the implementation.

    char *content_of_pgn_file = ...; // load your pgn data with your own code
                                     // this buffer must be null-terminated
    unsigned long long line   = 1;
    unsigned long long column = 1;
    MPGN_Node node;

    while (mpgn_parse(&content_of_pgn_file, &node, &line, &column)) {
        // do something with the parsed node.
        // you can append it to an array, etc.
        // it will be a valid node, as-in, not of type 'MPGN_Node_Error'.
        switch (node.type) {
            // (...)
        }
    }

    if (node.type == MPGN_Node_Error) {
        // there was a parsing failure of some kind!
        // more info is on `node.error.message`
    }


API
--------------------------------------------------------------------------------

returns 0 when there is no more data to parse, or we encountered an error during parsing, and 1 otherwise.
information about what happened, what was parsed, etc. is on the 'node' parameter (as well as line/column)
the data pointer pointed to by 'pgn' will be advanced as we move through the data stream.

    int mpgn_parse(char **pgn, MPGN_Node *node, unsigned long long *line, unsigned long long *column);

mpgn uses a 'slice' or 'string view' type. you might want to compare
one of these to a null-terminated string/your personal string type:

    int mpgn_slice_eq (const MPGN_Slice s, const char *str);
    int mpgn_slice_eqn(const MPGN_Slice s, const char *str, long long int str_count);


Configuration
--------------------------------------------------------------------------------

by default, <stdio.h> is included to do error message formatting and output.
Omit it by doing this:
    #define MINIPGN_OMIT_STDIO
with this defined, you will not produce debug, error, or crash output. The
program will silently fail. MPGN_Node_Error.message will always be empty.

If you want to still have useful debug error messages, but don't want to
pull in <stdio.h>, I would recommend integrating 'stb_sprintf' into your
project: https://github.com/nothings/stb/blob/master/stb_sprintf.h

And then replacing the single occurence in this file of 'snprintf' with the
stb equivalent 'stsbsp_snprintf', which has an identical API and behavior.

There is also a single occurence of 'printf' you will have to remove/replace.

---

You can omit <assert.h> by defining MPGN_ASSERT:
    #define MPGN_ASSERT(test, msg) my_assert_func(!!(test), msg)

---

By default we are not thread-safe as error messages are stored in a single
global buffer. You can:
    #define MINIPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER

Which will put the error buffer on the 'MPGN_Error' structure instead. This
bloats the overall size of the structure by ~6x (maybe this matters if you
are building very large arrays of these nodes), but you will get thread-safety.


PGN Format Known Caveats
--------------------------------------------------------------------------------

Caveats, exceptions, and notes on the 'spec' of PGN and how it relates to this
parser are below with annotations for the relevant section(s) of the spec. The
spec document(s) themselves are linked (multiple times for redundancy) just below:

https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm
https://ia902908.us.archive.org/26/items/pgn-standard-1994-03-12/PGN_standard_1994-03-12.txt
https://github.com/fsmosca/PGN-Standard/blob/master/PGN-Standard.txt


3: Formats: import and export
    This parser makes no distinctions between these two formats.

3.2.2: Archival Storage and the newline character
    This parser works with both newline and carriage return + newline line endings.

4.1: Character codes
    This parser doesn't validate that the data within strings, or comments, or 
    the 'escape mechanism' conform to ISO 8859/1 or any other specific encoding.
    @TODO we could catch some low-hanging fruit.

4.2: Tab characters
    This parser accepts horizontal and vertical tab characters inside comments
    and the 'escape mechanism' and forbids them otherwise.

4.3: Line lengths
    This parser does not enforce line lengths of any kind anywhere.

7. Tokens
    Backslashes are preserved in comments and tags. So, an escaped quote, which 
    in the source data will look like:
        \"

    will be exactly the same in the output comment/tag node, meaning, we will 
    not replace the escape sequence with a literal double-quote.

    This parser doesn't support "<" or ">" tokens.

8.1: Tag pair section
    "The same tag name should not appear more than once in a tag pair section".
    We don't validate this.

    "Some tag values may be composed of a sequence of items.  For example, a
     consultation game may have more than one player for a given side.  When this
     occurs, the single character ":" (colon) appears between adjacent items.
     Because of this use as an internal separator in strings, the colon should not
     otherwise appear in a string."
    We don't validate this - colons can appear anywhere in strings, any number of times.

8.1.1: Seven Tag Roster
    "For export format,
     the STR tag pairs appear before any other tag pairs. (The STR tag pairs must
     also appear in order; this order is described below). Also for export format,
     any additional tag pairs appear in ASCII order by tag name."
    We don't enforce the order of tags under any circumstance, or that they must exist.

    "a single empty line follows the last tag pair."
    We don't validate this.

8.2: Movetext section
    "Because illegal moves are not real chess moves, they are not permitted in PGN
     movetext."
    We don't validate that moves are legal chess moves.

8.2.1: Movetext line justification
    "In PGN export format, tokens in the movetext are placed left justified on
     successive text lines each of which has less than 80 printing characters. As
     many tokens as possible are placed on a line with the remainder appearing on
     successive lines. A single space character appears between any two adjacent
     symbol tokens on the same line in the movetext. As with the tag pair section,
     a single empty line follows the last line of data to conclude the movetext
     section.

     Neither the first or the last character on an export format PGN line is a
     space. (This may change in the case of commentary; this area is currently
     under development.)"
    We don't validate any of this.


8.2.3.2: Piece identification
    "The letter code for a pawn is not used for SAN moves in PGN export format
     movetext. However, some PGN import software disambiguation code may allow for
     the appearance of pawn letter codes."
    We don't allow the 'P' character in the movetext section.


9.7.1: Tag: SetUp
    "This tag takes an integer that denotes the "set-up" status of the game.  A
    value of "0" indicates that the game has started from the usual initial array.
    A value of "1" indicates that the game started from a set-up position; this
    position is given in the "FEN" tag pair.  This tag must appear for a game
    starting with a set-up position.  If it appears with a tag value of "1", a FEN
    tag pair must also appear."

*/
#ifndef INCLUDE_GUARD_MINIPGN_H
#define INCLUDE_GUARD_MINIPGN_H

typedef struct {
    char *p;
    long long int count;
} MPGN_Slice;

typedef struct {
#ifndef MINIPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER
    char *message;
#else
    char message[230];
#endif
    unsigned long long line;
    unsigned long long column;
} MPGN_Error;

typedef struct {
    MPGN_Slice name;  // not including the beginning '[', or any trailing whitespace.
    MPGN_Slice value; // not including the ending ']', or the delimiting double-quotes ('"')
} MPGN_Tag;

typedef enum {
    MPGN_Result_White   = 1,
    MPGN_Result_Black   = 2,
    MPGN_Result_Draw    = 3,
    MPGN_Result_Unknown = 4
} MPGN_Result_Type;

typedef struct {
    MPGN_Result_Type type;
    MPGN_Slice       text; // "0-1", "1-0", "1/2-1/2", or "*"
} MPGN_Result;

typedef enum {
    MPGN_Moveflag_Check            = 1 <<  0,
    MPGN_Moveflag_Mate             = 1 <<  1,
    MPGN_Moveflag_Castle           = 1 <<  2,
    MPGN_Moveflag_Queenside_Castle = 1 <<  3,
    MPGN_Moveflag_Null             = 1 <<  4, // note that the 'text' of a null move will either be "--", or "Z0"
    MPGN_Moveflag_Capture          = 1 <<  5, // if 'x' appears in the movetext
} MPGN_Moveflag;

// These are sometimes called 'PGN Symbols' and sometimes just 'Symbols'.
// There is also this acronymn CSM == Chess Symbol Mnemonic.
// Also sometimes called 'Nunn's Convention': https://en.wikipedia.org/wiki/Chess_annotation_symbols#Nunn's_convention
// They are just the strings "?", "?!", "??", "!", "!?", "!!" which occasionally appear at the end of a move.
typedef enum {
    MPGN_CSM_Goodmove    = 1,
    MPGN_CSM_Poormove    = 2,
    MPGN_CSM_Brilliant   = 3,
    MPGN_CSM_Blunder     = 4,
    MPGN_CSM_Interesting = 5,
    MPGN_CSM_Dubious     = 6
} MPGN_CSM;

typedef struct {
    MPGN_Slice text;               // the literal text of the move, including CSMs like '!!' or '?' as part of the move.
    MPGN_Moveflag flags;           // bit flags of various things you might want to know about this move
    MPGN_CSM csm;                  // if non-zero, the 'Chess Symbol Mnenomic' that follows the move. Like '!?', etc.
    char explicit_piece;           // non-zero when we have a move starting with a piece , like 'N', 'R', 'K' etc. note that pawns don't have this set in PGN.
    char source_file, source_rank; // if the move has a defined source, these will be non-zero. 'fe4' will set file to 'f', and 'Nh3g5' sets file to 'h' and rank to '3', for example.
    char target_file, target_rank; // same as above, but for the 'target' square. 'fe4' will set the target file/rank to be 'e' and '4', for example.
    char promotion;                // 0 if the move isn't a promotion, otherwise 'Q', 'R', 'N' or 'B'
} MPGN_Move;

typedef enum {
    MPGN_Node_Move      = 1, // "Nf3+", etc.
    MPGN_Node_Tag       = 2, // "[Site \"New York, NY USA\"]", etc.

    MPGN_Node_Comment   = 3, // "{This opening is called the Ruy Lopez.}", etc.
                             // Often people embed data in comments, like clock timers ex: "{[%clk 1:30:46]}"
                             // Note that we don't parse stuff inside comments, we'll just give you the text
                             // of the comment for you to do with what you like.

    MPGN_Node_Result    = 4, // "0-1", "1-0", "1/2-1/2", or "*"
    MPGN_Node_RAV_Begin = 5, // '(', indicates a RAV (recursive annotation variation). Moves parsed between this and a closing ')' 
                             // are a separate variation that is not necessarily played in the actual game. This structure is recursive,
                             // so you can see multiple nested '(' which open separate variations. If you need to parse RAVs, it is
                             // recommended to manage a stack to balance the parentheses and determine which moves go to which RAVs.
    MPGN_Node_RAV_End   = 6, // ')' The end of a Recursive Annotation Variation.
    MPGN_Node_NAG       = 7, // 'Numeric Annotation Glyph'. This is a string of the form '$XXX' where, 'X' are digits encoding a number >= 0 and <= 255 (with no leading zeroes

    MPGN_Node_Escape    = 8, // A single line of text that starts with a '%', including the percent sign. This is effectively just a comment.
                             // Please note that we've actually never encountered this in the wild. Commonly people will embed 'commands' inside
                             // of comments that contain the '%' sign, but these are not the same as what the PGN spec calls the 'escape mechanism'.

    MPGN_Node_Error     = 9  // indicates a parser error - not an actual parsed bit of PGN, unlike the others.
} MPGN_Node_Type;

typedef struct {
    MPGN_Node_Type type;
    union {
        MPGN_Move   move;       // a chess move, including null noves
        MPGN_Slice  comment;    // comments. can disambiguate ';' and '{' by checking first character
        MPGN_Tag    tag;        // key/value metadata in the header of a game
        MPGN_Result result;     // the 'result' or conclusion of a game
        char        *RAV_begin; // points to the '('
        char        *RAV_end;   // points to the ')'
        MPGN_Slice  nag;        // Numeric Annotation Glyph. The slice points to a string like '$45', including the dollar sign.
        MPGN_Slice  escape;     // 'Escape Mechanism'. This is effectively just a single-line comment

        MPGN_Error  error;      // not a piece of PGN syntax - this indicates a parsing error
    };
} MPGN_Node;

int mpgn_parse(char **pgn, MPGN_Node *node, unsigned long long *line, unsigned long long *column);

int mpgn_slice_eq (const MPGN_Slice s, const char *str);
int mpgn_slice_eqn(const MPGN_Slice s, const char *str, long long int str_count);

#endif // INCLUDE_GUARD_MINIPGN_H

#ifdef MINIPGN_IMPLEMENTATION

#ifndef MINIPGN_OMIT_STDIO
#include <stdio.h> // printf, snprintf
#endif // MINIPGN_OMIT_STDIO

#ifndef MINIPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER
static char mpgn_error_message[230];
#endif

#ifndef MINIPGN_OMIT_STDIO
#ifndef MINIPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER
#define mpgn_write_error_message(n, fmt, ...) \
    snprintf(mpgn_error_message, sizeof(mpgn_error_message), fmt, ##__VA_ARGS__); \
    n->error.message = mpgn_error_message;
#else
#define mpgn_write_error_message(n, fmt, ...) snprintf(n->error.message, sizeof(n->error.message), fmt, ##__VA_ARGS__)
#endif // MINIPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER
#else
#define mpgn_write_error_message(fmt, ...) do{}while(0)
#endif // MINIPGN_OMIT_STDIO

#define mpgn_report_error(node, line, column, fmt, ...) \
    { \
        node->type = MPGN_Node_Error; \
        node->error.line   = *line; \
        node->error.column = *column; \
        mpgn_write_error_message(node, fmt, ##__VA_ARGS__); \
        return 0; \
    }

// you can #define MPGN_ASSERT to avoid including 'assert.h'
#ifndef MPGN_ASSERT
#include <assert.h>
static void _mpgn_assert(int test, const char *msg) {
#ifndef MINIPGN_OMIT_STDIO
    if (!test) printf("%s\n", msg);
#endif
    assert(test);
}
#define MPGN_ASSERT(test, msg) _mpgn_assert(!!(test), msg)
#endif

int mpgn_slice_eqn(const MPGN_Slice s, const char *str, long long int str_count) {
    if (s.count != str_count) return 0;
    for (int i = 0; i < s.count; i++) if (s.p[i] != str[i]) return 0;
    return 1;
}

int mpgn_slice_eq(const MPGN_Slice s, const char *str) {
    int count = 0;
    const char *_str = str;
    while (*_str++) { count++; }
    return mpgn_slice_eqn(s, str, count);
}

static inline int mpgn_is_file(char c) {
    switch (c) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
            return 1;
    }
    return 0;
}

static inline int mpgn_is_rank(char c) {
    switch (c) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            return 1;
    }
    return 0;
}

static inline void mpgn_memset(char *p, int size, char v) {
    for (int i = 0; i < size; i++) p[i] = v;
}

static inline int mpgn_eat_square(const char *pgn, char *out_file, char *out_rank) {
    *out_file = 0;
    *out_rank = 0;
    char c = *pgn;
    if (!mpgn_is_file(c)) return 0;
    *out_file = c;
    c = *++pgn;
    if (!mpgn_is_rank(c)) return 0;
    *out_rank = c;
    return 1;
}

static inline char mpgn_advance_cursor(char **cursor, unsigned long long *line, unsigned long long *column, int count) {
    for (int i = 0; i < count; i++) {
        if (**cursor == '\n') {
            (*line)++;
            (*column) = 0;
        }
        (*cursor)++;
        (*column)++;
    }
    return **cursor;
}

static inline int mpgn_is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\r';
}

static inline char mpgn_skip_whitespace(char **cursor, unsigned long long *line, unsigned long long *column) {
    char c = **cursor;
    do {
        if (!mpgn_is_whitespace(c)) break;
    } while (c = mpgn_advance_cursor(cursor, line, column, 1));
    return c;
}

static inline int mpgn_is_alpha(int c) {
    return ((unsigned int)(c | 32) - 97) < 26U;
}

static inline int mpgn_is_digit(char c) {
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return 1;
    }
    return 0;
}

static inline char mpgn_eat_capture(char c, char **cursor, unsigned long long *line, unsigned long long *column, MPGN_Node *node) {
    if (c == 'x') {
        node->move.flags |= MPGN_Moveflag_Capture;
        c = mpgn_advance_cursor(cursor, line, column, 1);
    }
    return c;
}

static char mpgn_end_move(char **cursor, unsigned long long *line, unsigned long long *column, MPGN_Node *node) {
    char c = **cursor;
    // after the target square, there could be a '+' (check) or a '#' (mate)
    if (c == '#') {
        node->move.flags |= MPGN_Moveflag_Mate | MPGN_Moveflag_Check;
        c = mpgn_advance_cursor(cursor, line, column, 1);

    } else if (c == '+') {
        node->move.flags |= MPGN_Moveflag_Check;
        c = mpgn_advance_cursor(cursor, line, column, 1);
    }
    // parse CSM markers after the movetext. we still consider this part of the 'move', as-in it will be on the slice for the move.
    if (c == '?') {
        c = mpgn_advance_cursor(cursor, line, column, 1);
        if (c == '!') { // questionable/dubious move '?!'
            c = mpgn_advance_cursor(cursor, line, column, 1);
            node->move.csm |= MPGN_CSM_Dubious;
        } else if (c == '?') { // blunder/very poor move '??'
            c = mpgn_advance_cursor(cursor, line, column, 1);
            node->move.csm |= MPGN_CSM_Blunder;
        } else { // poor move/mistake '?'
            node->move.csm |= MPGN_CSM_Poormove;
        }
    } else if (c == '!') {
        c = mpgn_advance_cursor(cursor, line, column, 1);
        if (c == '!') { // brilliant move double exclam '!!' (ben finegold)
            c = mpgn_advance_cursor(cursor, line, column, 1);
            node->move.csm |= MPGN_CSM_Brilliant;
        } else if (c == '?') { // 'interesting' move '!?'
            c = mpgn_advance_cursor(cursor, line, column, 1);
            node->move.csm |= MPGN_CSM_Interesting;
        } else { // good move '!'
            node->move.csm |= MPGN_CSM_Goodmove;
        }
    }
    return c;
}

// returns 0 when there is no more data to parse, or we encountered an error during parsing, and 1 otherwise.
int mpgn_parse(char **pgn, MPGN_Node *node, unsigned long long *line, unsigned long long *column) {
    MPGN_ASSERT(pgn, "'pgn' is a required parameter!");
    MPGN_ASSERT(*pgn, "'pgn' points to a null pointer! It must point to a pointer which points to the beginning of some PGN data.");
    MPGN_ASSERT(node, "'node' is a required parameter!");
    MPGN_ASSERT(line, "'line' is a required parameter!");
    MPGN_ASSERT(column, "'column' is a required parameter!");

    mpgn_memset((char*)node, sizeof(MPGN_Node), 0);

    char *cursor = *pgn;
    char c = *cursor;
    while (c != '\0') {
        c = *cursor;

        switch (c) {
            case '\0':
                return 0;
            case (char)0xEF:
                if (cursor[1] == (char)0xBB && cursor[2] == (char)0xBF) {
                    mpgn_report_error(node, line, column, "Got BOM (Byte-Order-Mark) for UTF-8 file at beginning of stream - you probably want to skip these (increment pointer by 3) before beginning parsing.");
                }
            default:
                mpgn_report_error(node, line, column, "Unexpected character ('%c') in stream with codepoint (hex): 0x%hhx", c, c);

            case '\v':
                mpgn_report_error(node, line, column, "Got a vertical tab character ('\\v'), which is not permitted outside of comments");
            case '\t':
                mpgn_report_error(node, line, column, "Got a horizontal tab character ('\\t'), which is not permitted outside of comments");

            case '\n': {
                c = mpgn_advance_cursor(&cursor, line, column, 1);
            } continue;

            case '\r': // you can change this to be an error if you care about forbidding carriage returns.
                       // I recommend against it, because some databases in the wild use carriage returns.
            case ' ': {
                c = mpgn_advance_cursor(&cursor, line, column, 1);
            } continue;

            case '-': {
                if (cursor[1] == '-') { // null move. This isn't a real PGN standard thing, but people do it anyway.
                    node->type = MPGN_Node_Move;
                    node->move.text.p = cursor;
                    node->move.text.count = 2;
                    node->move.flags |= MPGN_Moveflag_Null;

                    c = mpgn_advance_cursor(&cursor, line, column, 2);
                    *pgn = cursor;
                    return 1;
                }
                mpgn_report_error(node, line, column, "We expected to see a 'null' move, as-in '--', but we only got a single dash, followed by a '%c'. Malformed data perhaps?", c);
            } continue;

            case 'Z': {
                if (cursor[1] != '0') mpgn_report_error(node, line, column, "Expected a null move in the 'Z0' notation, but we got: %c%c", c, cursor[1]);
                node->type = MPGN_Node_Move;
                node->move.text.p = cursor;
                node->move.text.count = 2;
                node->move.flags |= MPGN_Moveflag_Null;

                c = mpgn_advance_cursor(&cursor, line, column, 2);
                *pgn = cursor;
                return 1;
            } continue;

            case '$': {
                // parse 'NAG' or 'Numeric Annotation Glyph'
                node->type = MPGN_Node_NAG;
                node->nag.p = cursor;
                int digit_count = 0;
                do {
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    if (!c) mpgn_report_error(node, line, column, "Unexpected end of stream when parsing NAG!");
                    if (c < 48 || c > 57) {
                        if (digit_count == 0) mpgn_report_error(node, line, column, "Unexpected character after '$' (expecting a NAG): %c", c);
                        break;
                    }
                    digit_count++;
                    if (digit_count > 3) mpgn_report_error(node, line, column, "When parsing NAG, got more than 3 digits. NAGs must be a 1-3 digit number from 0-255!");
                } while (1);
                node->nag.count = cursor - node->nag.p;
                *pgn = cursor;
                return 1;
            } continue;

            case '(': {
                if (cursor[1] == '*') {
                    mpgn_report_error(node, line, column, "Got a continuation '(*' - this is unsupported.");
                }
                node->type = MPGN_Node_RAV_Begin;
                node->RAV_begin = cursor;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                *pgn = cursor;
                return 1;
            } continue;

            case ')': {
                node->type = MPGN_Node_RAV_End;
                node->RAV_end = cursor;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                *pgn = cursor;
                return 1;
            } continue;

            // brace-delimited comment:
            case '{': {
                node->type = MPGN_Node_Comment;
                node->comment.p = cursor;
                do {
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    if (!c) mpgn_report_error(node, line, column, "unexpected end of string when looking for closing brace in comment! Missing a terminating '}'?");
                    if (c == '}') break;
                } while (1);
                node->comment.count = cursor - node->comment.p + 1;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                *pgn = cursor;
                return 1;
            } continue;

            // 'escape mechanism'
            case '%': {
                if (*column != 1) mpgn_report_error(node, line, column, "There's an escape mechanism in this game but it's not on the first column of a line!");
                node->escape.p = cursor;
                do {
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    if (!c) mpgn_report_error(node, line, column, "unexpected end of string when looking for terminating newline in escape mechanism!");
                    if (c == '\n') break;
                } while (1);
                node->type = MPGN_Node_Escape;
                node->escape.count = cursor - node->escape.p;
                *pgn = cursor;
                return 1;
            } continue;

            // single-line comment:
            case ';': {
                node->type = MPGN_Node_Comment;
                node->comment.p = cursor;
                do {
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    if (!c) mpgn_report_error(node, line, column, "unexpected end of string when looking for end of line for comment!");
                    if (c == '\n') break;
                } while (1);
                node->comment.count = cursor - node->comment.p;
                *pgn = cursor;
                return 1;
            } continue;

            case '[': {
                node->type = MPGN_Node_Tag;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                c = mpgn_skip_whitespace(&cursor, line, column);
                // parse the tag name:
                do {
                    if (mpgn_is_alpha(c) || mpgn_is_digit(c)) {
                        if (!node->tag.name.p) node->tag.name.p = cursor;

                    } else if (c == '_') {
                        if (!node->tag.name.p) {
                            mpgn_report_error(node, line, column, "Found an underscore leading a tag name, which is not allowed. Tag names should start with a letter or a digit.");
                        }
                    } else if (mpgn_is_whitespace(c)) {
                        break;

                    } else if (!c) {
                        mpgn_report_error(node, line, column, "Unexpected end of string when parsing tag name!");

                    } else {
                        mpgn_report_error(node, line, column, "Unexpected character %c when parsing tag name!", c);
                    }
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                } while (1);
                node->tag.name.count = cursor - node->tag.name.p;

                c = mpgn_skip_whitespace(&cursor, line, column);
                if (c != '"') mpgn_report_error(node, line, column, "Expected a double-quote to begin a tag value, but got: %c", c);
                c = mpgn_advance_cursor(&cursor, line, column, 1);

                // parse the tag value:
                node->tag.value.p = cursor;
                int leading_backslashes = 0;
                do {
                    if (!c) mpgn_report_error(node, line, column, "unexpected end of string when looking for end of tag pair ']'!");
                    if (c == '\\') {
                        leading_backslashes++;
                    } else {
                        if (c == '"' && !(leading_backslashes & 1)) break; // allow escaped quotes to not end the string.
                        leading_backslashes = 0;
                    }
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                } while (1);
                node->tag.value.count = cursor - node->tag.value.p;
                c = mpgn_advance_cursor(&cursor, line, column, 1); // move past the terminating quote.
                c = mpgn_skip_whitespace(&cursor, line, column);
                if (c != ']') mpgn_report_error(node, line, column, "Expected closing ']' at end of tag pair, but got: %c", c);
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                *pgn = cursor;
                return 1;
            } continue;

            case '*': {
                // unknown game result
                node->type = MPGN_Node_Result;
                node->result.type = MPGN_Result_Unknown;
                node->result.text.p = cursor;
                node->result.text.count = 1;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                *pgn = cursor;
                return 1;
            } break;

            case '0':
                if (cursor[1] == '-' && cursor[2] == '1') {
                    // black win marker
                    node->type = MPGN_Node_Result;
                    node->result.type = MPGN_Result_Black;
                    node->result.text.p = cursor;
                    node->result.text.count = 3;
                    c = mpgn_advance_cursor(&cursor, line, column, 3);
                    *pgn = cursor;
                    return 1;
                }
            case 'O': {
                node->type = MPGN_Node_Move;
                node->move.text.p = cursor;
                // castling. apparently, it is not consistent whether systems use 0 or O for this.
                // taking 'O' as the marker, 'O-O' is king-side castle, and 'O-O-O' is queen-side.
                char marker = c;
                if (marker == '0') mpgn_report_error(node, line, column, "Expecting to parse a castling move that uses '0' instead of 'O'! We expect that 'O' is used");
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                node->move.flags |= MPGN_Moveflag_Castle;
                if ((*cursor == '-') && (*(cursor+1) == marker)) {
                    c = mpgn_advance_cursor(&cursor, line, column, 2);
                    if ((*cursor == '-') && (*(cursor+1) == marker)) {
                        c = mpgn_advance_cursor(&cursor, line, column, 2);
                        node->move.flags |= MPGN_Moveflag_Queenside_Castle;
                    }
                }
                c = mpgn_end_move(&cursor, line, column, node);
                node->move.text.count = cursor - node->move.text.p;
                *pgn = cursor;
                return 1;
            } break;

            case '1':
                if (cursor[1] == '/' && cursor[2] == '2' && cursor[3] == '-' && cursor[4] == '1' && cursor[5] == '/' && cursor[6] == '2') { // draw marker!
                    node->type = MPGN_Node_Result;
                    node->result.type = MPGN_Result_Draw;
                    node->result.text.p = cursor;
                    node->result.text.count = 7;
                    c = mpgn_advance_cursor(&cursor, line, column, 7);
                    *pgn = cursor;
                    return 1;

                } else if (cursor[1] == '-' && cursor[2] == '0') { // white victory marker
                    node->type = MPGN_Node_Result;
                    node->result.type = MPGN_Result_White;
                    node->result.text.p = cursor;
                    node->result.text.count = 3;
                    c = mpgn_advance_cursor(&cursor, line, column, 3);
                    *pgn = cursor;
                    return 1;
                }
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                do {
                    // find the first dot. this is a move marker like '12.'
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    if (c == '.') break;
                    if (!c) mpgn_report_error(node, line, column, "unexpected end of string when trying to parse a move");
                    if (c < 48 || c > 57) mpgn_report_error(node, line, column, "Unexpected character '%c' between number and (expected) dot in move number", c);
                } while (1);
            } continue;

            case '.': {
                c = mpgn_advance_cursor(&cursor, line, column, 1);
            } continue;

            // explicit pieces:
            //case 'P': // apparently in some contexts (non-SAN/Standard Algebraic Notation) this is used for pawn.
            case 'K':
            case 'Q':
            case 'R':
            case 'B':
            case 'N': {
                node->move.text.p = cursor;
                node->move.explicit_piece = c;
                c = mpgn_advance_cursor(&cursor, line, column, 1);
                if (mpgn_is_rank(c)) {
                    node->move.source_rank = c;
                    c = mpgn_advance_cursor(&cursor, line, column, 1); // for moves like: N2xf3
                }
                c = mpgn_eat_capture(c, &cursor, line, column, node);
            } continue;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h': {
                if (!node->move.text.p) node->move.text.p = cursor;
                node->type = MPGN_Node_Move;

                char source_file, source_rank;
                if (mpgn_eat_square(cursor, &source_file, &source_rank)) {
                    if (!node->move.source_rank) { // if this is set, there was already a source implied, like in the string 'N2xf3'
                                                   // the 'f3' which would be eaten by this would overwrite the '2' in the source rank,
                                                   // which we don't want.
                        node->move.source_rank = source_rank;
                        node->move.source_file = source_file;
                        c = mpgn_advance_cursor(&cursor, line, column, 2);
                        c = mpgn_eat_capture(c, &cursor, line, column, node);
                    }
                } else {
                    if (cursor[1] == 'x') {
                        node->move.source_rank = c;
                        c = mpgn_advance_cursor(&cursor, line, column, 2);
                        node->move.flags |= MPGN_Moveflag_Capture;

                    } else if (mpgn_is_file(c)) {
                        node->move.source_file = c;
                        c = mpgn_advance_cursor(&cursor, line, column, 1);
                        c = mpgn_eat_capture(c, &cursor, line, column, node);

                    } else if (mpgn_is_rank(c)) {
                        node->move.source_rank = c;
                        c = mpgn_advance_cursor(&cursor, line, column, 1);
                        c = mpgn_eat_capture(c, &cursor, line, column, node);

                    } else {
                        mpgn_report_error(node, line, column, "Unexpected character when parsing move text: '%c'", c);
                    }
                }

                char target_file = 0;
                char target_rank = 0;
                if (!mpgn_eat_square(cursor, &target_file, &target_rank)) {
                    node->move.target_rank = node->move.source_rank;
                    node->move.target_file = node->move.source_file;
                    node->move.source_rank = 0;
                    node->move.source_file = 0;
                } else {
                    node->move.target_rank = target_rank;
                    node->move.target_file = target_file;
                    c = mpgn_advance_cursor(&cursor, line, column, 2);
                }
                if (c == '=') {
                    c = mpgn_advance_cursor(&cursor, line, column, 1);
                    switch (c) {
                        default:
                            mpgn_report_error(node, line, column, "Expected a promotion kind (Q, R, B, N), but got: '%c'", c);

                        case 'Q':
                        case 'R':
                        case 'B':
                        case 'N':
                            node->move.promotion = c;
                            c = mpgn_advance_cursor(&cursor, line, column, 1);
                            break;
                    }
                }
                c = mpgn_end_move(&cursor, line, column, node);
                node->move.text.count = cursor - node->move.text.p;
                *pgn = cursor;
                return 1;
            } continue;
        }
    }
    *pgn = cursor;
    return 0;
}

#endif // MINIPGN_IMPLEMENTATION

/*
MIT License

Copyright 2026 Nick Hayashi

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
