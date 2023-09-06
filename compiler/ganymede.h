
#ifndef HEADER_FILE
#define HEADER_FILE

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ht ht;

// scan.c
extern FILE *outfile;
extern char *limit;

enum Kind {
        LT,      // <
        GT,      // >
        LEQ,     // <=
        GEQ,     // >=
        LSHIFT,  // <<
        RSHIFT,  // >>
        DEREF,   // ->
        DECR,    // --
        EQ,      // ==
        NEQ,     // !=
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,        // %
        ADDASSIGN,  // +=
        SUBASSIGN,  // -=
        MULASSIGN,  // *=
        DIVASSIGN,  // /=
        MODASSIGN,  // %=
        OROR,       // ||
        ANDAND,     // &&
        INCR,       // ++
        EOI,        // end of input
        IF,
        INT,
        OBR,        // [
        CBR,        // ]
        OCBR,       // {
        CCBR,       // }
        OPAR,       // (
        CPAR,       // )
        SEMIC,      // ;
        COMMA,      // ,
        TILDA,      // ~
        AND,        // &
        OR,         // |
        XOR,        // ^
        NOT,        // !
        ANDASSIGN,  // &=
        ORASSIGN,   // |=
        XORASSIGN,  // ^=
        NOTASSIGN,  // !=
        STRGIZE,    // #
        TKPASTE,    // ##
        ASSIGN,     // =
        QMARK,      // ?
        IDENT,
        INTCONST,
        FLOATCONST,
        STRCONST,
        CHARCONST,
        ELLIPSIS,
        AUTO,
        CASE,
        CHAR,
        CONST,
        CONTINUE,
        DEFAULT,
        DO,
        DOUBLE,
        ELSE,
        ENUM,
        EXTERN,
        FLOAT,
        FOR,
        GOTO,
        LONG,
        REGISTER,
        RETURN,
        SHORT,
        SIGNED,
        SIZEOF,
        STATIC,
        STRUCT,
        SWITCH,
        TYPEDEF,
        UNION,
        UNSIGNED,
        VOID,
        VOLATILE,
        WHILE,
        DOT,
        BREAK,
        COLON,
        RSHIFTASSIGN,
        LSHIFTASSIGN,
        INCLUDE,
        DEFINE,
        BACKSLASH,
        STMT_EXPR,  // different from other stmt types
        STMT_COMPOUND,
        DOUBLECONST,
        LONGDOUBLECONST,
        NONE,  // for error handling
};

struct Token {
        enum Kind kind;
        char *start;  // for IDENT
        int len;      // for IDENT
        struct Token *next;
        union {
                int ivalue;
                float fvalue;
                double dvalue;
                long double ldvalue;
        };
};

extern char *token_names[];
void error(char *fmt, ...);
void printTokens(struct Token *head, FILE *outfile);
struct Token *scan(char *stream);

// parser
struct ExtDecl *parse(struct Token *tokens);
void printExtDecl(struct ExtDecl *extDecl, int level);

struct scope {
        struct scope *next;
        ht *vars;  // key: name, value: declspec
};

// either function or declaration
struct ExtDecl {
        struct ExtDecl *next;
        struct declspec *declspec;
        struct decltor *decltor;
        struct expr *expr;         // for declaration
        struct initializer *init;  // for array declaration
        struct stmt *compStmt;     // for function
};

struct declspec {
        enum Kind type;
        int array[2];  // row, col
        int pointer;   // pointer
};

struct decltor {
        //
        char *name;
        enum {
                FUNCTION,
                DECLARATION,
        } kind;
        struct params *params;  // function
        int row;                // array
        int col;                // array
        int pointer;            //  0 - isn't pointer, 1 - is pointer, 2 - is pointer pointer
};

struct params {
        struct params *next;
        struct declspec *declspec;
        struct decltor *decltor;
};

/*
int array[4] = {1, 2, 3, 4}
                   -------------               
ExcDecl --init--> | INITIALIZER | --chilren--> 1 --> 2 --> 3 --> 4
                   -------------               

int array[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}
                   -------------                -------------
ExcDecl --init--> | INITIALIZER | --chilren--> | INITIALIZER | --children--> 1 --> 2 --> 3 --> 4
                   -------------                -------------
                                                      |
                                                     next
                                                      |
                                                      V
                                                ------------- 
                                               | INITIALIZER | --children--> 5 --> 6 --> 7 --> 8
                                                ------------- 
                                                      |
                                                     next
                                                      |
                                                      V
                                                ------------- 
                                               | INITIALIZER | --children--> 9 --> 10 --> 11 --> 12
                                                ------------- 
*/
struct initializer {
        struct initializer *next;
        struct initializer *children;
        union {
                int ivalue;
        } value;
        enum Kind type;
};

struct expr {
        enum Kind kind;
        union {
                int ivalue;
                float fvalue;
                double dvalue;
        };
        char *strLit;

        struct expr *lhs;
        struct expr *rhs;
};

// statement or declaration
struct block {
        struct block *next;
        struct ExtDecl *decl;
        struct stmt *stmt;
};

/*
label_stmt
    ident ':' stmt                              | value then
    'case' expr ':' stmt                        | cond then
    'default' ':' stmt                          | then

compound_stmt
    '{' block '}'                               | body

expression_stmt
    expr ';'                                    | value 

selection_stmt
    'if' '(' expr ')' stmt                      | cond then
    'if' '(' expr ')' stmt 'else' stmt          | cond then els
    'switch' '(' expr ')' stmt                  | cond then

iteration_stmt
    'while' '(' expr ')' stmt                   | cond then
    'do' stmt 'while' '(' expr ')' ';'          | then cond
    'for' '(' expr ';' expr ';' expr ')' stmt   | init cond inc then
    'for' '(' decl expr ';' expr ')' stmt       | init cond inc then

jump_stmt
    'goto' ident ';'                            | value
    'continue' ';'                              | 
    'break' ';'                                 | 
    'return' expr ';'                           | value
*/
struct stmt {
        enum Kind kind;
        struct expr *cond;
        struct stmt *then;
        struct stmt *els;
        union {
                struct expr *expr;
                struct ExtDecl *decl;
        } init;
        int init_kind;  // 0 for expr, 1 for decl
        struct expr *inc;
        struct expr *value;
        struct block *body;
};

// hashmap.c


typedef struct {
        const char *key;  // current key
        void *value;      // current value

        ht *_table;     // reference to hash table being iterated
        size_t _index;  // current index into ht._entries
} hti;

typedef struct {
        const char *key;  // key is NULL if this slot is empty
        void *value;
} ht_entry;

struct ht {
        ht_entry *entries;  // hash slots
        size_t capacity;    // size of _entries array
        size_t length;      // number of items in hash table
};

ht *ht_create(void);
void ht_destroy(ht *table);
void *ht_get(ht *table, const char *key);
const char *ht_set(ht *table, const char *key, void *value);
size_t ht_length(ht *table);
hti ht_iterator(ht *table);
bool ht_next(hti *it);
void ht_test(void);
char *strdup(const char *s);

// codegen
void codegen(struct ExtDecl *program);

#endif
