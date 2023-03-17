#ifndef JSON_JSON_H
#define JSON_JSON_H

#define C_BIT

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define c99
#else
#define restrict
#endif


#ifdef WANT_LIBC
#ifdef __STDC_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#endif
#include<stdio.h>
#include<string.h>
#include<stddef.h>
#include<stdbool.h>
#endif

#ifndef PREFIX
#define PREFIX
#endif

#ifndef _MSC_VER
/* msvc doesn't handle vla */
#define HAS_VLA
#define va_(val) val
#else
#define va_(val)
#endif


#ifndef bool
# ifdef c99
#  define bool _Bool
# else
#  define bool int
# endif
#endif

#define cat3(a, b) a ## b
#define cat2(a, b) cat3(a, b)
#ifdef HAS_VLA
#define EXPORT(type, symbol) extern C_BIT type cat2(PREFIX,symbol)
#else
#define EXPORT(type, symbol) extern C_BIT __declspec(dllexport) type cat2(PREFIX,symbol)
#endif

#if !defined(WANT_LIBC)   /* No libC */
#define NULL ((void *)0)
#endif

#ifndef STRING_POOL_SIZE
#define STRING_POOL_SIZE 0x2000
#endif
#ifndef MAX_TOKENS
#define MAX_TOKENS 0x200
#endif

#ifdef WANT_LIBC
#ifdef __STDC_WANT_LIB_EXT1__
#define cs_strlen(s) strlen_s(s, RSIZE_MAX)
  #define cs_memset(dest, val, repeat) memset_s((dest), (repeat), (val), (repeat))
  #define cs_memcpy(dest, val, repeat) memcpy_s((dest), (repeat), (val), (repeat))
  #define cs_memcmp(v1, v2, size) memcmp((v1), (v2), (size))
  #define cs_memmove(dest, src, size) memmove((dest), (size), (src), (size))
#else
#define cs_strlen(s) strlen(s)
#define cs_memset(dest, val, repeat) memset((dest), (val), (repeat))
#define cs_memcpy(dest, val, repeat) memcpy((dest), (val), (repeat))
#define cs_memcmp(v1, v2, size) memcmp((v1), (v2), (size))
#define cs_memmove(dest, src, size) memmove((dest), (src), (size))
#endif  /* __STDC_WANT_LIB_EXT1__ */
#else
#define size_t unsigned long long
#define ptrdiff_t signed long long

/* from MuslC */
static inline size_t cisson_strlen(const unsigned char *s)
{
    const unsigned char *a = s;
    for (; *s; s++);
    return s-a;
}
#define cs_strlen(s) cisson_strlen((const unsigned char*)(s))

static inline void *cisson_memset(void *dest, int c, size_t n)
{
    unsigned char *s = (unsigned char*)dest;

    for (; n; n--, s++) *s = (unsigned char )c;
    return dest;
}
#define cs_memset(dest, val, repeat) (cisson_memset((dest), (val), (repeat)))

static inline void * cisson_memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (unsigned char *)src;

    for (; n; n--) *d++ = *s++;
    return dest;
}
#define cs_memcpy(dest, val, repeat) (cisson_memcpy((dest), (val), (repeat)))

static inline int cisson_memcmp(const void *vl, const void *vr, size_t n)
{
    const unsigned char *l=(unsigned char *)vl, *r=(unsigned char *)vr;
    for (; n && *l == *r; n--, l++, r++);
    return n ? *l-*r : 0;
}
#define cs_memcmp(v1, v2, size) (cisson_memcmp((v1), (v2), (size)))

static inline void * cisson_memmove(void *dest, const void *src, size_t n)
{
    char *d = (char*)dest;
    const char *s = (char*)src;

    if (d==s) return d;
    if (s-d-n <= -2*n) return cisson_memcpy(d, s, n);

    if (d<s) {
        for (; n; n--) *d++ = *s++;
    } else {
        while (n) n--, d[n] = s[n];
    }

    return dest;
}
#define cs_memmove(dest, src, size) (cisson_memmove((dest), (src), (size)))
#endif  /* WANT_LIBC */


/*****************************************************/


/**
 * Json structures are stored as a flat array of objects holding
 * a link to their parent whose value is their index in that array, index zero being the root
 * Json doesn't require json arrays elements to have an order so sibling data is not stored.
 */

#define WHITESPACE \
  X(TABULATION, '\t') \
  X(LINE_FEED, '\n') \
  X(CARRIAGE_RETURN, '\r') \
  X(SPACE, ' ')

#define ERRORS \
  X(JSON_ERROR_NO_ERRORS, "No errors found.") \
  X(JSON_ERROR_INVALID_CHARACTER, "Found an unknown token.") \
  X(JSON_ERROR_TWO_OBJECTS_HAVE_SAME_PARENT, "Two values have the same parent.") \
  X(JSON_ERROR_EMPTY, "A JSON document can't be empty.") \
  X(JSON_ERROR_INVALID_ESCAPE_SEQUENCE, "Invalid escape sequence.")                       \
  X(JSON_ERROR_INVALID_NUMBER, "Malformed number.")              \
  X(JSON_ERROR_INVALID_CHARACTER_IN_ARRAY, "Invalid character in array.") \
  X(JSON_ERROR_ASSOC_EXPECT_STRING_A_KEY, "A JSON object key must be a quoted string.") \
  X(JSON_ERROR_ASSOC_EXPECT_COLON, "Missing colon after object key.") \
  X(JSON_ERROR_ASSOC_EXPECT_VALUE, "Missing value after JSON object key.")  \
  X(JSON_ERROR_NO_SIBLINGS, "Only Arrays and Objects can have sibling descendants.")      \
  X(JSON_ERROR_JSON1_ONLY_ASSOC_ROOT, "JSON1 only allows objects as root element.")  \
  X(JSON_ERROR_UTF16_NOT_SUPPORTED_YET, "Code points greater than 0x0800 not supported yet.") \
  X(JSON_ERROR_INCOMPLETE_UNICODE_ESCAPE, "Incomplete unicode character sequence.") \
  X(JSON_ERROR_UNESCAPED_CONTROL, "Control characters must be escaped.") \

#define X(a, b) a,
enum whitespace_tokens { WHITESPACE };
enum json_errors{ ERRORS };
#undef X

extern char whitespaces[];

extern char const * json_errors[];


extern char digit_starters[];
extern char digits[];
extern char digits19[];
extern char hexdigits[];

enum states {
    EXPECT_BOM = 0,
    EXPECT_VALUE,
    AFTER_VALUE,
    OPEN_ARRAY,
    OPEN_ASSOC,
    LITERAL_ESCAPE,
    IN_STRING,
    START_NUMBER,
    NUMBER_AFTER_MINUS,
    IN_NUMBER,
    EXPECT_FRACTION,
    EXPECT_EXPONENT,
    IN_FRACTION,
    IN_FRACTION_DIGIT,
    EXPONENT_EXPECT_PLUS_MINUS,
    EXPECT_EXPONENT_DIGIT,
    IN_EXPONENT_DIGIT,
    ARRAY_AFTER_VALUE,
    ASSOC_EXPECT_KEY,
    CLOSE_STRING,
    ASSOC_EXPECT_COLON,
    ASSOC_AFTER_INNER_VALUE
};


enum kind {
    JSON_UNSET,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
    JSON_STRING,
    JSON_NUMBER,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_LAST /* corresponds to the last element as specified by json pointer */
};

#define string_size_type_raw unsigned int
#define string_size_type (string_size_type_raw)

struct token {
    enum kind kind;
    int support_index;
    unsigned char * address;
    string_size_type_raw length;
};

struct json_tree {
    int do_not_touch_; /* when compiling in C++, if the
    * first field is an enum, the whole thing can't be
    * { 0 }  initialized, this is just to make sure the
    * first field will never be an enum. */
    enum states cur_state;
    int current_support;
    struct tokens {
        struct token *stack;
        int max;
    } tokens;
    struct strings {
        unsigned char *pool;
        unsigned int cursor;
    } strings;
    enum {
        JSON2,
        JSON1,
    } mode;
    int no_copy;
    const unsigned char *cur_string_start;
    string_size_type_raw cur_string_len;
};

static struct token static_stack[MAX_TOKENS];
static unsigned char static_pool[STRING_POOL_SIZE];

/* State maintenance */
EXPORT(void, start_state_)(
        struct json_tree * state,
        struct token *stack,
        unsigned char *pool);
EXPORT(struct token*, query_from_)(
        struct json_tree * state,
        size_t length,
        unsigned char query[va_(length)],
        struct token * host
        );
EXPORT(struct token* , next_child_)(
        struct tokens * restrict tokens,
        struct token * root,
        struct token * current);

/* Parsing */
EXPORT(enum json_errors, rjson_)(
        size_t len,
        const unsigned char *cursor,
        struct json_tree * state
);
EXPORT(enum json_errors, inject_)(
        size_t len,
        unsigned char text[va_(len)],
        struct json_tree * state,
        struct token * where
);
/* Output */

EXPORT(unsigned char* , to_string_)(
        struct tokens * restrict tokens_to_print,
        struct token * start_token,
        int compact, 
        unsigned char* sink,
        int sink_size);
/* Building */
EXPORT(void, start_string)(unsigned int *, unsigned char [STRING_POOL_SIZE]);
EXPORT(void, push_string)(const unsigned int * restrict cursor, unsigned char * restrict pool, unsigned char* restrict string, size_t length);
EXPORT(void, close_root)(struct token * restrict, int * restrict);
EXPORT(void, push_root)(int * restrict, const int * restrict);
EXPORT(void, push_token_kind)(enum kind kind, void *restrict address
                            , struct tokens *tokens, int support_index);
EXPORT(void, delete_token_)(struct tokens* tokens, struct token* which);
EXPORT(void, move_token_)(struct json_tree* state, struct token* which, struct token* where);
EXPORT(void, rename_string_)(struct json_tree* state, struct token* which, size_t len, const char* new_name);
/* EZ JSON */
EXPORT(void, insert_token_)(struct json_tree * state, unsigned char *token, struct token* root);
EXPORT(void, stream_tokens_)(struct json_tree * state, struct token * where, char separator, unsigned char *stream, size_t length);

#define tok_len(token) ( \
    (token)->length          \
    ? (token)->length        \
    : *(string_size_type_raw *)((token)->address - sizeof string_size_type))

#define ROOT_TOKEN (-1)
#define EMPTY_CONTAINER_TOKEN (-2)

#define next_child(tokens, root, current) cat2(PREFIX,next_child_)(tokens, root, current)

#define start_state(state,stack,pool) cat2(PREFIX, start_state_)((state), (stack), (pool))

#define rjson(text, state) cat2(PREFIX,rjson_)(cs_strlen(((char*)(text))), (unsigned char*)(text), (state))
#define rjson_len(len, text, state) cat2(PREFIX,rjson_)((len), (unsigned char*)(text), (state))

#define inject(text, state, where) cat2(PREFIX,inject_)(cs_strlen(((char*)(text))), (unsigned char*)(text), (state), (where))

#define to_string(state_) (char * restrict)cat2(PREFIX, to_string_)(&(state_)->tokens, (state_)->tokens.stack, 2, NULL, -1)
#define to_string_compact(state_) (char * restrict)cat2(PREFIX, to_string_)(&(state_)->tokens, (state_)->tokens.stack, 0, NULL, -1)
#define to_string_pointer(state_, start) (char * restrict)cat2(PREFIX, to_string_)(&(state_)->tokens, start, 0, NULL, -1)
#define to_string_pointer_fail(state_, start, fail) (start ? (char * restrict)cat2(PREFIX, to_string_)(&(state_)->tokens, start, 0, NULL, -1): fail)
#define to_string_sink(state_, start, sink, size) (char * restrict)cat2(PREFIX, to_string_)(&(state_)->tokens, start, 0, sink, size)

#define delete_token(tokens, which) cat2(PREFIX, delete_token_)(tokens, which)
#define move_token(state, which, where) cat2(PREFIX, move_token_)(state, which, where)
#define rename_string(state, which, new_name) cat2(PREFIX,rename_string_)(state, which, cs_strlen((char*)(new_name)), new_name)
#define insert_token(state, token, root) cat2(PREFIX,insert_token_)((state), (unsigned char*)(token), root)
#define push_token(state, token) cat2(PREFIX,insert_token_)((state), (unsigned char*)(token), &(state)->tokens.stack[(state)->current_support])

#define query(state, string) query_from(state, string, (state)->tokens.stack)
#define query_from(state, string, from) cat2(PREFIX,query_from_)((state), cs_strlen((char*)(string)), (unsigned char*)(string), from)
#define stream_tokens(state, sep, stream) cat2(PREFIX,stream_tokens_)( \
    (state),                                              \
    &(state)->tokens.stack[(state)->current_support],     \
    (sep), (unsigned char*)(stream), cs_strlen(((char*)(stream))))
#define stream_into(state, where, sep, stream) cat2(PREFIX,stream_tokens_)((state), (where), (sep), (stream), cs_strlen(((char*)(stream))))
#define START_STRING(state_, string_) \
    (state_)->no_copy        \
    ? (state_)->cur_string_start = (const unsigned char *)(string_) \
    , (state_)->cur_string_len = 0\
    : (cat2(PREFIX,start_string)(&(state_)->strings.cursor, (state_)->strings.pool), 0)
#define PUSH_STRING(state_, string_, length_) \
    (state_)->no_copy                         \
    ? (state_)->cur_string_len += (string_size_type_raw)(length_) \
    : (cat2(PREFIX,push_string)(                             \
        &(state_)->strings.cursor,             \
        (state_)->strings.pool,                \
        (unsigned char*)(string_),                             \
        (length_)), 0)
#define CLOSE_ROOT(state_) cat2(PREFIX,close_root) \
    ((*(state_)).tokens.stack, &(*(state_)).current_support)
#define PUSH_ROOT(state_) cat2(PREFIX,push_root) \
    (&(state_)->current_support, &(state_)->tokens.max)
#define SET_HOST(state_, host) cat2(PREFIX,push_root) \
    (&(state_)->current_support, host)
#define PUSH_TOKEN(kind_, address_, state_) \
    cat2(PREFIX,push_token_kind)(                             \
        (kind_),                            \
        (void*)(address_),                         \
        &(state_)->tokens,                  \
        (state_)->current_support)
#define INSERT_TOKEN(kind_, address_, state_, root) \
    cat2(PREFIX,push_token_kind)(                        \
        (kind_),                            \
        (address_),                         \
        &(state_)->tokens,                  \
        (root))
#define PUSH_STRING_TOKEN(kind_, state_) \
    do {                                          \
        if((state_)->no_copy) {                    \
            PUSH_TOKEN(                        \
                (kind_),                             \
                (state_)->cur_string_start - sizeof string_size_type, \
                (state_));               \
                (state_)->tokens.stack[(state_)->tokens.max - 1].length \
                = (state_)->cur_string_len; \
        } else {                                             \
            PUSH_TOKEN(                          \
                (kind_),                             \
                (state_)->strings.pool + (state_)->strings.cursor, \
                (state_)                             \
            ); \
        } \
    }while(0)

#define INSERT_STRING_TOKEN(kind_, state_, root) \
    INSERT_TOKEN(                                \
        (kind_),                                 \
        (state_)->strings.pool + (state_)->strings.cursor, \
        (state_),                                \
        (root)                                   \
    )

#define START_AND_PUSH_TOKEN(state, kind, string) \
    do {                                          \
        START_STRING(state, string);               \
        PUSH_STRING((state), string, (cs_strlen ((char*)(string)))); \
        PUSH_STRING_TOKEN(kind, state);       \
    }while(0)

#define START_AND_INSERT_TOKEN(state, kind, string, root) \
    START_STRING(state, string);               \
    PUSH_STRING((state), (string), (cs_strlen ((char*)(string)))); \
    INSERT_STRING_TOKEN((kind), (state), (root))
/* __/ */


#undef c99

#endif //JSON_JSON_H
