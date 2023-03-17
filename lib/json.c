#include "json.h"

char digit_starters[] = "-0123456789";
char digits[] = "0123456789";
char digits19[] = "123456789";
char hexdigits[] = "0123456789abcdefABCDEF";

#ifdef c99
#define X(a, b) [(a)] = (b),
#else
#define X(a, b) (b),
#endif
char whitespaces[] = {
        WHITESPACE
        '\0',
};

char const * json_errors[] = {
        ERRORS
};
#undef X

#undef WHITESPACE
#undef ERRORS


static inline size_t
in(const char* restrict hay, unsigned char needle) {
    if (needle == '\0') return 0u;
    const char * begin = hay;
    for (;*hay; hay++) {
        if((unsigned char)*hay == needle) return (hay - begin) + 1;
    }
    return 0u;
}

static inline size_t
len_whitespace(unsigned const char * restrict string) {
    int count = 0;
    while(string && in(whitespaces, string[count]) != 0u) {
        count++;
    }
    return count;
}

static inline size_t
remaining(
        unsigned const char * restrict max,
        unsigned const char * restrict where) {
    return max - where;
}

static int aindex(struct token* stack, struct token* which) {
    return (int)(which - stack);
}

/* Better use START_AND_PUSH_TOKEN instead of this directly. */
EXPORT(void, start_string)(unsigned int * restrict cursor,
             unsigned char pool[STRING_POOL_SIZE]) {
    *cursor += (string_size_type_raw)(pool[*cursor]) + sizeof string_size_type;
    cs_memset(&pool[*cursor], 0, sizeof string_size_type);
}

/* Better use START_AND_PUSH_TOKEN instead of this directly. */
EXPORT(void, push_string)(const unsigned int *cursor,
            unsigned char *pool,
            unsigned char* string,
            size_t length) {
    /* todo: memmove if we insert */
    string_size_type_raw* sh = (string_size_type_raw*)(void*)&pool[*cursor];
    cs_memcpy(
            pool + *cursor + sizeof string_size_type + *sh,
            string,
            length);
    *sh += (string_size_type_raw)length;
}

/* Better use the CLOSE_ROOT macro instead of this. */
EXPORT(void, close_root)(struct token * tokens,
        int * root_index) {
    *root_index = tokens[*root_index].support_index;
}

/* Better use PUSH_ROOT macro instead of this. */
EXPORT(void, push_root)(int * root_index, const int * token_cursor) {
    *root_index = *token_cursor - 1;
}

static int children_count(struct tokens* tokens, struct token* root) {
    int count = 0;
    ptrdiff_t root_index = root - tokens->stack;
    for(ptrdiff_t i = root_index + 1;
        (i < tokens->max && tokens->stack[i].support_index >= root_index);
        i++) {
            count++;
    }

    return count;
}

/* Better use START_AND_PUSH_TOKEN instead of this directly. */
EXPORT(void, push_token_kind)(
        enum kind kind,
        void * address,
        struct tokens *tokens,
        int support_index) {

    /* move_token all the tokens coming after the direct children
     * of current support */
    int cc = children_count(tokens, &tokens->stack[support_index]);
    /* all the tokens after the children of our root and before the max */
    size_t to_move =
            sizeof (struct token)
                    * ((tokens->max - 1) - (support_index + cc));
    cs_memmove(
        &tokens->stack[support_index + cc + 2],
        &tokens->stack[support_index + cc + 1],
        to_move
    );

    /* fix up support indices of moved tokens except for
     * the token that supports the others */
    for (int i = support_index + cc + 3; i < tokens->max + 1; ++i) {
        tokens->stack[i].support_index++;
    }

    struct token tok = {
        .kind=kind,
        .support_index=support_index,
        .address=(unsigned char*)address + sizeof string_size_type
    };

    tokens->stack[support_index + cc + 1] = tok;

    tokens->max++;
}

/* Inserts a token at the given root (fetched from query())
 * use the insert_token() macro to not have to cast your string. */
EXPORT(void, insert_token_)(
        struct json_tree * state, unsigned char *token, struct token* root) {

    int target_root = aindex(state->tokens.stack, root);

    if(token[0] == '>') {
        CLOSE_ROOT(state);
        return;
    }

    enum kind kind = (enum kind[]){
            JSON_UNSET, JSON_TRUE, JSON_FALSE, JSON_NULL, JSON_ARRAY, JSON_OBJECT,
            JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_NUMBER,
            JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_NUMBER, JSON_STRING
    }[in("tfn[{-0123456789\"", token[0])];

    if(kind != JSON_UNSET) {
        START_AND_INSERT_TOKEN(state, kind, token, target_root);
    }

    if(in("{[:", token[0])) PUSH_ROOT(state);
}

/* Streams characters into a json_tree without checking,
 * like (json_tree, target, '~', "1~2~3") */
EXPORT(void, stream_tokens_)(
        struct json_tree * state, struct token * where,
        char separator, unsigned char *stream, size_t length) {
    size_t i = 0;
    int old_root = state->current_support;
    state->current_support = (int)(where - state->tokens.stack);

    while (i < length) {
        size_t token_length = 0;
        while (i + token_length < length && stream[i + token_length] != (unsigned char)separator) {
            token_length++;
        }
        stream[i + token_length] = '\0';
        push_token(state, &stream[i]);
        i = i + token_length + sizeof separator;
    }
    state->current_support = old_root;
}

/* Sets a state so you don't have to zero initialize it. */
EXPORT(void, start_state_)(
        struct json_tree * state,
        struct token *stack,
        unsigned char *pool
        ) {
    state->cur_state = EXPECT_BOM;
    state->tokens.stack = stack;
    state->strings.pool = pool;
    state->current_support = -1;
    state->tokens.max = 0;
    state->strings.cursor = 0u;
}

/**
 * Returns support of a node, until the root or virtual
 * root is met (when printing from a pointer).
 */
static inline struct token *
support(struct token* stack,
        struct token* cur,
        struct token* limit) {
    if(!cur) return NULL;
    if(cur->support_index == ROOT_TOKEN) return NULL;
    if(cur == limit) return NULL;
    return &stack[cur->support_index];
}

/**
 * Returns the next sibling from the given root and the given node.
 * If the given node is NULL, returns the first child.
 * If last child or no child exists, returns NULL.
 */
EXPORT(struct token* , next_child_)(
        struct tokens * restrict tokens,
        struct token * root,
        struct token * current) {

    if(root == NULL) return NULL;
    if (root->support_index == EMPTY_CONTAINER_TOKEN) return NULL;
    if(current && current->kind == JSON_UNSET) return NULL;

    int root_idx = aindex(tokens->stack, root);
    int start = aindex(tokens->stack, current) + 1;

    if (current == NULL)
        start = (root_idx + 1);

    int i; // fixme: dont iterate over everything
    for(i = start; i < tokens->max; i++) /* we still have to browse over nested arrays */
        if (tokens->stack[i].support_index == root_idx) {
            return &tokens->stack[i];
        }

    return NULL;
}

/* Fetches a token from a json_tree with a json pointer.
 * Returns NULL if not found. */
EXPORT(struct token*, query_from_)(
        struct json_tree * tree, size_t length,
        unsigned char query[va_(length)],
        struct token * root) {
    /* todo : unescape ~0, ~1, ~2 */
    size_t i = 0;

    if(length == 0) {
        return root;
    }

    do {
        if(!root) {
            return root;
        }

        if (query[i] == '/') {
            if (root->kind != JSON_OBJECT && root->kind != JSON_ARRAY) {
                return NULL;
            }
            i++;
        }

        size_t tlen = 0u;
        while (i + tlen < length && query[i + tlen] != '/') {
            tlen++;
        }

        unsigned char buffer[0x80] = { (unsigned char)('"' * (root->kind == JSON_OBJECT)) };
        cs_memcpy(&buffer[1ull * (root->kind == JSON_OBJECT)], &query[i], tlen);
        buffer[1 + tlen] = '"' * (root->kind == JSON_OBJECT);

        if (root->kind == JSON_ARRAY) {
            int index = 0;
            int j;
            for (j = 0; buffer[j]; j++) {
                index *= 10;
                index += buffer[j] - '0';
            }

            struct token * cur = NULL;
            do {
                cur = next_child(&tree->tokens, root, cur);
            } while (cur && index--);
            root = cur;

        } else if (root->kind == JSON_OBJECT) {
            struct token * cur = NULL;
            while ((cur = next_child(&tree->tokens, root, cur))) {
                if (cs_memcmp(cur->address, buffer, tlen + 2) == 0) {
                    if((i + tlen + 1 < length && /* deal with /< */
                        query[i + tlen] == '/' && query[i + tlen + 1] == '<')) {
                        return cur;
                    }
                    root = next_child(&tree->tokens, cur, NULL);
                    goto end;
                }
            }
            return NULL; /* if we haven't found a matching child */
        }
        end:i = i + tlen;
    } while (i++ < length);

    return root;
}

/* returns the container of a token or itself if it is one */
static struct token* container(struct token* stack, struct token* which) {
    if(which->support_index == ROOT_TOKEN
    || which->kind == JSON_ARRAY
    || which->kind == JSON_OBJECT
    || (which->kind == JSON_STRING && support(stack, which, stack)->kind == JSON_OBJECT))
    {
        return which;
    }
    return support(stack, which, stack);
}

/* deletes a token and all its descendants */
EXPORT(void, delete_token_)(struct tokens* tokens, struct token* which) {
    /* todo: make a function that wipes bound string, in case we really want to delete, and not just move_token */
    int cc = children_count(tokens, which);
    ptrdiff_t addr = which - tokens->stack;
    cs_memmove(which, &tokens->stack[addr + cc + 1], sizeof (struct token) * (tokens->max - (addr + cc + 1)));
    tokens->max -= (cc + 1);

    for(ptrdiff_t i = addr; i < tokens->max; i++) {
        if(tokens->stack[i].support_index > aindex(tokens->stack, which)) {
            tokens->stack[i].support_index -= (cc+1);
        }
    }
}

/* deletes a token and shifts all next ones, fix their support index. */
static void delete_one_token(struct tokens* tokens, struct token* which) {
    cs_memmove(which,
               &tokens->stack[(which - tokens->stack) + 1],
               sizeof (struct token) * ((tokens->max - aindex(tokens->stack, which)) - 1)
    );
    tokens->max --;
    for(int i = aindex(tokens->stack, which); i < tokens->max; i++) {
        if(tokens->stack[i].support_index > aindex(tokens->stack, which)) {
            tokens->stack[i].support_index--;
        }
    }
}

/* move_token tokens and fix offsets that need be. */
EXPORT(void, move_token_)(
        struct json_tree* state, struct token* which, struct token* where) {

    int root_support_index = which->support_index;

    do {
        enum kind kind = which->kind;
        unsigned char* address = which->address - sizeof string_size_type;

        int support_index = aindex(state->tokens.stack, container(state->tokens.stack, where));
        /* target support index is the target,
         * minus the original support distance, minus the nb of siblings */

        delete_one_token(&state->tokens, which);
        INSERT_TOKEN(kind, address, state, support_index - 1);
    } while (
        which->support_index > root_support_index
        /* with the delete_one_token() which points to the next token thanks to the memmove */
    );
}

/* inserts a new string and makes the given token point to it. */
EXPORT(void, rename_string_)(
        struct json_tree* state, struct token* which, size_t len, const char* new_name) {
    START_STRING(state, new_name);
    PUSH_STRING(state, "\"", 1);
    PUSH_STRING(state, new_name, len);
    PUSH_STRING(state, "\"", 1);
    which->address = state->strings.pool + state->strings.cursor + sizeof string_size_type;
}

/* Injects JSON text inside an existing JSON json_tree. */
EXPORT(enum json_errors, inject_)(size_t len,
       unsigned char text[va_(len)],
       struct json_tree * state,
       struct token * where) {
    // todo, we should be able to inject at the end of an array or object
    // with /- pointer, or anywhere inside an array or object.
    enum states old_state = state->cur_state;
    int old_root = state->current_support;

    where = container(state->tokens.stack, where);
    state->current_support = (int)(where - (state->tokens.stack));

    state->cur_state = EXPECT_BOM;
    enum json_errors error = rjson_len(
            len, (unsigned char *) text, state);
    state->cur_state = old_state;
    state->current_support = old_root;
    return error;
}

/* Reads len characters of json text and turns it into an ast. */
EXPORT(enum json_errors, rjson_)(
        size_t len,
        const unsigned char *cursor,
        struct json_tree * state) {

#define peek_at(where) cursor[where]
#define SET_STATE_AND_ADVANCE_BY(which_, advance_) \
  state->cur_state = which_; cursor += advance_

    if (state->tokens.stack == NULL) {
        start_state(state, static_stack, static_pool);
    }

    enum json_errors error = JSON_ERROR_NO_ERRORS;
    unsigned const char * final = cursor + len;

    // todo: fully test reentry
    // todo: make ANSI/STDC compatible
    // todo: add jasmine mode? aka not copy strings+numbers ?
    // todo: pedantic mode? (forbid duplicate keys, enforce order, cap integer values to INT_MAX...)
    // todo: test for overflows
    // fixme: check for bounds
    // todo: no memory move_token mode (simple tokenizer)
    // todo: implement JSON pointer RFC6901
    // todo: implement JSON schema

    for(;;) {
        switch (state->cur_state) {
            case EXPECT_BOM: {
                if(
                        (unsigned char)peek_at(0) == u'\xEF'
                        && (unsigned char)peek_at(1) == u'\xBB'
                        && (unsigned char)peek_at(2) == u'\xBF'
                        ) {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_VALUE, 3);
                } else {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_VALUE, 0);
                }
                break;
            }

            case EXPECT_VALUE:
            {
                if (peek_at(len_whitespace(cursor)+0) == '"') {
                    START_STRING(state, cursor + len_whitespace(cursor));
                    PUSH_STRING(state, (unsigned char[]) {peek_at(len_whitespace(cursor) + 0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_STRING, len_whitespace(cursor) + 1);
                }
                else if (peek_at(len_whitespace(cursor) + 0) == '[') {
                    START_AND_PUSH_TOKEN(state, JSON_ARRAY, "[");
                    PUSH_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(OPEN_ARRAY, len_whitespace(cursor) + 1);
                }
                else if (peek_at(len_whitespace(cursor) + 0) == '{') {
                    START_AND_PUSH_TOKEN(state, JSON_OBJECT, "{");
                    PUSH_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(OPEN_ASSOC, len_whitespace(cursor) + 1);
                }
                else if (in(digit_starters, peek_at(len_whitespace(cursor) + 0))) {
                    START_STRING(state, cursor + len_whitespace(cursor));
                    SET_STATE_AND_ADVANCE_BY(START_NUMBER, len_whitespace(cursor) + 0);
                }
                else if (
                        peek_at(len_whitespace(cursor) + 0) == 't'
                        && peek_at(len_whitespace(cursor) + 1) == 'r'
                        && peek_at(len_whitespace(cursor) + 2) == 'u'
                        && peek_at(len_whitespace(cursor) + 3) == 'e' )
                {
                    START_AND_PUSH_TOKEN(state, JSON_TRUE, "true");
                    SET_STATE_AND_ADVANCE_BY(
                            AFTER_VALUE,
                            len_whitespace(cursor)+sizeof("true") - 1)
                            ;
                }
                else if (peek_at(len_whitespace(cursor) + 0) == 'f'
                         && peek_at(len_whitespace(cursor) + 1) == 'a'
                         && peek_at(len_whitespace(cursor) + 2) == 'l'
                         && peek_at(len_whitespace(cursor) + 3) == 's'
                         && peek_at(len_whitespace(cursor) + 4) == 'e') {
                    START_AND_PUSH_TOKEN(state, JSON_FALSE, "false");
                    SET_STATE_AND_ADVANCE_BY(
                            AFTER_VALUE,
                            len_whitespace(cursor)+sizeof("false") - 1
                            );
                }
                else if (peek_at(len_whitespace(cursor)+0) == 'n'
                         && peek_at(len_whitespace(cursor)+1) == 'u'
                         && peek_at(len_whitespace(cursor)+2) == 'l'
                         && peek_at(len_whitespace(cursor)+3) == 'l') {
                    START_AND_PUSH_TOKEN(state, JSON_NULL, "null");
                    SET_STATE_AND_ADVANCE_BY(
                            AFTER_VALUE,
                            len_whitespace(cursor)+sizeof("null") - 1
                            );
                }
                else if (state->current_support != ROOT_TOKEN) {
                    error = JSON_ERROR_ASSOC_EXPECT_VALUE;
                }
                else if (remaining(final, cursor)) {
                    error = JSON_ERROR_INVALID_CHARACTER;
                }
                else {
                    error = JSON_ERROR_EMPTY;
                }

                break;
            }
            case AFTER_VALUE:
            {
                if (state->current_support == ROOT_TOKEN) {
                    if (remaining(final, cursor + len_whitespace(cursor))) {
                        error = JSON_ERROR_NO_SIBLINGS;
                    } else if(state->mode == JSON1 && state->tokens.stack[state->tokens.stack[state->tokens.max - 1].support_index].kind != JSON_OBJECT) {
                        error = JSON_ERROR_JSON1_ONLY_ASSOC_ROOT;
                    } else {
                        goto exit;
                    }
                }
                else if(state->tokens.stack[state->current_support].kind == JSON_ARRAY) {
                    SET_STATE_AND_ADVANCE_BY(ARRAY_AFTER_VALUE, 0);
                }
                else if(state->tokens.stack[state->current_support].kind == JSON_STRING) {
                    SET_STATE_AND_ADVANCE_BY(ASSOC_AFTER_INNER_VALUE, 0);
                }

                break;
            }
            case OPEN_ARRAY: {
                if (peek_at(len_whitespace(cursor)) == ']') {
                    CLOSE_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, len_whitespace(cursor) + 1);
                } else {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_VALUE, 0);
                }

                break;
            }
            case OPEN_ASSOC: {
                if (peek_at(len_whitespace(cursor)) == '}') {
                    CLOSE_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, len_whitespace(cursor) + 1);
                } else {
                    SET_STATE_AND_ADVANCE_BY(ASSOC_EXPECT_KEY, 0);
                }
                break;
            }
            case LITERAL_ESCAPE: {
                error = (enum json_errors)(JSON_ERROR_INVALID_ESCAPE_SEQUENCE * !in("\\\"/ubfnrt", peek_at(0)));  /* u"\/ */
                if (in("\\\"/bfnrt", peek_at(0))) {
                    PUSH_STRING(state, (unsigned char[]) {peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_STRING, 1);
                }
                else if (peek_at(0) == 'u') {
                    if(!(in(hexdigits, peek_at(1))
                    && in(hexdigits, peek_at(2))
                    && in(hexdigits, peek_at(3))
                    && in(hexdigits, peek_at(4)))
                    ) {
                        error = JSON_ERROR_INCOMPLETE_UNICODE_ESCAPE;
                        break;
                    }
                    PUSH_STRING(state, (char*)cursor, 5);
                    SET_STATE_AND_ADVANCE_BY(IN_STRING, 5);
                    break;
                }
                else {
                    error = JSON_ERROR_INVALID_ESCAPE_SEQUENCE;
                }

                break;
            }
            case IN_STRING: {
                error = (enum json_errors)(JSON_ERROR_UNESCAPED_CONTROL * (peek_at(0) < 0x20));
                PUSH_STRING(state, (unsigned char[]) {peek_at(0)}, error == JSON_ERROR_NO_ERRORS);
                SET_STATE_AND_ADVANCE_BY(
                        (
                                (enum states[]){
                                        IN_STRING,
                                        LITERAL_ESCAPE,
                                        CLOSE_STRING
                                }[in("\\\"", peek_at(0))]),
                        /*state->error == JSON_ERROR_NO_ERRORS);*/
                        1);

                break;
            }

            case START_NUMBER: {
                PUSH_STRING(
                        state,
                        ((unsigned char[]){peek_at(0)}),
                        ((int[]){0, 1}[in("-", peek_at(0))])
                    );
                SET_STATE_AND_ADVANCE_BY(
                    NUMBER_AFTER_MINUS,
                    (
                            (int[]){0, 1}[in("-", peek_at(0))])
                    );

                break;
            }

            case NUMBER_AFTER_MINUS:
            {
                if (peek_at(0) == '0') {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(EXPECT_FRACTION, 1);
                } else if (in(digits19, peek_at(0))) {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_NUMBER, 1);
                } else {
                    error = JSON_ERROR_INVALID_NUMBER;
                }

                break;
            }
            case IN_NUMBER: {
                PUSH_STRING(
                        state,
                        (unsigned char[]){peek_at(0)},
                        (in(digits, peek_at(0)) != 0)
                );
                SET_STATE_AND_ADVANCE_BY(
                        ((enum states[]){
                            EXPECT_FRACTION,
                            IN_NUMBER}[in(digits, peek_at(0)) != 0]),
                (in(digits, peek_at(0)) != 0)
                );

                break;
            }
            case EXPECT_FRACTION: {
                PUSH_STRING(
                        state,
                        (unsigned char[]){peek_at(0)},
                        (peek_at(0) == '.')
                );
                SET_STATE_AND_ADVANCE_BY(
                        ((enum states[]){
                                EXPECT_EXPONENT,
                                IN_FRACTION}[peek_at(0) == '.']),
                        (peek_at(0) == '.')
                );

                break;
            }

            case EXPECT_EXPONENT: {
                if (peek_at(0) == 'e' || peek_at(0) == 'E') {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(EXPONENT_EXPECT_PLUS_MINUS, 1);
                } else {
                    PUSH_STRING_TOKEN(JSON_NUMBER, state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, 0);
                }

                break;
            }

            case IN_FRACTION: {
                error = (enum json_errors)(JSON_ERROR_INVALID_NUMBER * (in(digits, peek_at(0)) == 0));
                PUSH_STRING(state,
                            (unsigned char[]){peek_at(0)},
                            error == JSON_ERROR_NO_ERRORS);
                SET_STATE_AND_ADVANCE_BY(
                        ((enum states[]){
                            IN_FRACTION,
                            IN_FRACTION_DIGIT
                        }[error == JSON_ERROR_NO_ERRORS]),
                        error == JSON_ERROR_NO_ERRORS);

                break;
            }

            case IN_FRACTION_DIGIT: {
                if (in(digits, peek_at(0))) {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_FRACTION_DIGIT, 1);
                } else {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_EXPONENT, 0);
                }

                break;
            }

            case EXPONENT_EXPECT_PLUS_MINUS: {
                if (peek_at(0) == '+' || peek_at(0) == '-') {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(EXPECT_EXPONENT_DIGIT, 1);
                } else {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_EXPONENT_DIGIT, 0);
                }

                break;
            }

            case EXPECT_EXPONENT_DIGIT: {
                if (in(digits, peek_at(0))) {
                    PUSH_STRING(state, (unsigned char[]){peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_EXPONENT_DIGIT, 1);
                } else {
                    error = JSON_ERROR_INVALID_NUMBER;
                }

                break;
            }

            case IN_EXPONENT_DIGIT: {
                if (in(digits, peek_at(0))) {
                    PUSH_STRING(state, (unsigned char[]) {peek_at(0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_EXPONENT_DIGIT, 1);
                } else {
                    PUSH_STRING_TOKEN(JSON_NUMBER, state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, len_whitespace(cursor) + 0);
                }

                break;
            }

            case ARRAY_AFTER_VALUE: {
                if(peek_at(len_whitespace(cursor)) == ',') {
                    SET_STATE_AND_ADVANCE_BY(EXPECT_VALUE, len_whitespace(cursor) + 1);
                } else if(peek_at(len_whitespace(cursor) + 0) == ']') {
                    CLOSE_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, len_whitespace(cursor) + 1);
                } else {
                    error = JSON_ERROR_INVALID_CHARACTER_IN_ARRAY;
                }

                break;
            }

            case ASSOC_EXPECT_KEY: {
                if(peek_at(len_whitespace(cursor) + 0) == '"') {
                    START_STRING(state, cursor + len_whitespace(cursor));
                    PUSH_STRING(state, (unsigned char[]) {peek_at(len_whitespace(cursor) + 0)}, 1);
                    SET_STATE_AND_ADVANCE_BY(IN_STRING, len_whitespace(cursor) + 1);
                } else {
                    error = JSON_ERROR_ASSOC_EXPECT_STRING_A_KEY;
                }
                break;
            }

            case CLOSE_STRING: {  /* fixme: non advancing state */
                PUSH_STRING_TOKEN(JSON_STRING, state);
                if ((state->tokens.stack)[state->current_support].kind == JSON_OBJECT) {
                    PUSH_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(ASSOC_EXPECT_COLON, 0);
                } else {
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, 0);
                }

                break;
            }

            case ASSOC_EXPECT_COLON: {
                error = (enum json_errors)(JSON_ERROR_ASSOC_EXPECT_COLON * (peek_at(len_whitespace(cursor)) != ':'));
                SET_STATE_AND_ADVANCE_BY(
                        ((enum states[]){
                            ASSOC_EXPECT_COLON,
                            EXPECT_VALUE}[error == JSON_ERROR_NO_ERRORS]),
                (size_t)(len_whitespace(cursor) + 1) * (error == JSON_ERROR_NO_ERRORS));
                break;
            }

            case ASSOC_AFTER_INNER_VALUE: {
                if(peek_at(len_whitespace(cursor)) == ',') {
                    CLOSE_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(ASSOC_EXPECT_KEY, len_whitespace(cursor) + 1);
                } else if (peek_at(len_whitespace(cursor)) == '}'){
                    CLOSE_ROOT(state);
                    CLOSE_ROOT(state);
                    SET_STATE_AND_ADVANCE_BY(AFTER_VALUE, len_whitespace(cursor) +1);
                } else {
                    error = JSON_ERROR_ASSOC_EXPECT_VALUE;
                }

                break;
            }
        }

        if(error != JSON_ERROR_NO_ERRORS) {
            goto exit;
        }
    }

    exit: return error;
#undef peek_at
#undef SET_STATE_AND_ADVANCE_BY
}

static char ident_s[0x80];
static char * print_ident(int depth, int indent) {
    if (!depth) return (char*)"";
    cs_memset(ident_s, ' ', ((size_t)indent * (size_t)depth));
    ident_s[((size_t)indent * (size_t)depth)] = '\0';
    return ident_s;
}


static struct token * climb_up(
        struct tokens* tokens,
        struct token** which,
        struct token** container,
        struct token* limit) {
    struct token* immediate = support(tokens->stack, *container, limit);
    if(immediate==NULL) {*which = *container; *container = NULL; goto end;}
    if(immediate->kind == JSON_ARRAY || immediate->kind == JSON_OBJECT)
    { // checkme: codecov
        *which = *container;
        *container = immediate;
    }
    if(immediate->kind == JSON_STRING) {
        *container = support(tokens->stack, immediate, limit);
        *which = immediate;
    }
    end:return *container;
}

static unsigned char output_[STRING_POOL_SIZE];

/* prints JSON text into the sink, better use to_string*
 * macros instead of this directly. */
EXPORT(unsigned char * , to_string_)(
        struct tokens * restrict tokens_to_print,
        struct token * start_token,
        int indent, unsigned char* sink,
        int sink_size) {

    unsigned char * output = output_;
    if(sink) {
        output = sink;
    } else {
        sink_size = sizeof output_;
    }

    #ifndef NDEBUG
        cs_memset(output_, 0, sink_size);
    #endif

    size_t cursor = 0;

    struct token *stack = tokens_to_print->stack;

    if (start_token == NULL) {
        start_token = &stack[0];
    }

    int depth = 0;

    struct token * tok;
    #define idx(tok_) aindex(stack, tok_)
    for(tok = start_token;
        tok == start_token || (
                idx(tok) < tokens_to_print->max
                && tok->support_index > start_token->support_index
                && (
              container(stack, start_token) == start_token
              )); /* whether started with a container or itself */
        tok =
                &stack[idx(tok) + 1]
    ) {
    #undef idx
    #define cat_raw(where, string) ( \
        cs_memcpy((where), (string), cs_strlen((string))), cs_strlen((string)) \
    )

        if (stack[tok->support_index].kind != JSON_STRING) {
            cursor += cat_raw(output + cursor, print_ident(depth, indent));
        }

        size_t len = tok_len(tok);

        cs_memcpy(output+cursor, tok->address, len);
        cursor += len;

        if (stack[tok->support_index].kind == JSON_OBJECT && tok->kind == JSON_STRING) {
            cursor += cat_raw(output + cursor, indent ? ": " :  ":");
        }

        if(tok->kind == JSON_ARRAY || tok->kind == JSON_OBJECT) {
            cursor += cat_raw(output + cursor, !indent ? "" : "\n");
            depth++;
        }

        if(next_child(tokens_to_print, tok, NULL) != NULL) {
            continue; /* if we have children */
        }

        struct token * cur = tok;
        struct token * sup = support(stack, cur, start_token);

        /* check we are container w/o children */
        if((cur->kind == JSON_ARRAY || cur->kind == JSON_OBJECT)
            && next_child(tokens_to_print, cur, NULL) == NULL)
        {
            sup = cur;
            cur = &(struct token) {0};
        }

        if(sup && sup->kind == JSON_STRING) {
            cur = sup;
            sup = support(stack, sup, start_token);
        }


        /** close ]} groups and print them at correct indents */
        while (
                sup /* or check we are last child of our parent container */
                && next_child(tokens_to_print, sup, cur) == NULL
        ) {
            cursor += cat_raw(output + cursor, !indent ? "" : "\n");

            --depth, depth < 0 ? depth = 0 : 0;
            cursor += cat_raw(
                output + cursor, print_ident(depth, indent)
            );

            char end[2] = {"\0]}"[
                in((const char[]){JSON_ARRAY, JSON_OBJECT},sup ? sup->kind : 0)
                       ], '\0'};
            cursor += cat_raw(output + cursor, end);

            climb_up(tokens_to_print, &cur, &sup, start_token);
        }

        if (sup
            && next_child(tokens_to_print, sup, cur) != NULL) {
            cursor += cat_raw(output + cursor,
                              indent ? ",\n" : ",");
        }
    }

    output[cursor] = '\0';
    return output;
    #undef cat
    #undef cat_raw
}

#undef EXPORT
#ifdef restrict
  #undef restrict
#endif
#undef C_BIT
#undef va_
#undef HAS_VLA
#undef suffix
#undef macro_suffix
#undef concat
#undef size_t
#undef ptrdiff_t
