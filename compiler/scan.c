#include "baikalc.h"

static const int MAX_IDENT_LEN = 20;
static const int MAX_INT_LEN = 10;
static const int MAX_PUNCT_LEN = 2;
static struct Token *currentToken;

static struct Token *new_token(enum TokenKind kind, char *buffer, int len) {
    struct Token *token = calloc(1, sizeof(struct Token));
    token->kind = kind;
    token->buffer = buffer;
    token->len = len;
    return token;
}

static bool LexIdentContinue(char **stream) {
    char *start = *stream;
    while (isalnum(**stream) || **stream == '_') {
        (*stream)++;
    }
    currentToken = new_token(TK_IDENT, start, *stream - start);
    return true;
}

static bool LexNumContinue(char **stream) {
    char *start = *stream;
    while (isdigit(**stream)) {
        (*stream)++;
    }
    currentToken = new_token(TK_NUM, start, *stream - start);
    return true;
}

static bool LexPunctContinue(char **stream) {
    char *start = *stream;
    (*stream)++;
    // while (ispunct(**stream)) {
    //     (*stream)++;
    // }
    currentToken = new_token(TK_PUNCT, start, *stream - start);
    return true;
}

struct Token *b_scan(char *stream) {
    struct Token head = {};
    struct Token *cur = &head;
    while (*stream) {
        if (isspace(*stream)) {
            stream++;
            continue;
        } else if (isalpha(*stream)) {
            LexIdentContinue(&stream);
        } else if (isdigit(*stream)) {
            LexNumContinue(&stream);
        } else if (ispunct(*stream)) {
            LexPunctContinue(&stream);
        } else {
            fprintf(stderr, "cannot tokenize char: %s\n", stream);
            exit(1);
        }
        cur = cur->next = currentToken;
    }
    cur->next = new_token(TK_EOF, NULL, 0);
    return head.next;
}

void print(struct Token *token) {
    const char *TokenKindNames[] = {
        "TK_IDENT",
        "TK_NUM",
        "TK_PUNCT",
        "TK_EOF",
    };
    while (token->kind != TK_EOF) {
        // enum name
        printf("%-10s", TokenKindNames[token->kind]);
        // token buffer
        for (int i = 0; i < token->len; i++) {
            printf("%c", token->buffer[i]);
        }
        printf("\n");
        token = token->next;
    }
}