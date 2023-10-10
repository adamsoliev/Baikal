#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DATA STRUCTURES
// clang-format off
enum TokenKind { /* KEYWORDS */
                INT,
                IF,
                RETURN,
                OPAR,
                CPAR,
                OCBR,
                CCBR,
                ADD,
                SUB,
                MUL,
                DIV,
                MOD,
                LT,     // <
                GT,     // >
                LE,     // <=
                GE,     // >=
                EQ,     // ==
                NEQ,    // !=
                LOR,    // ||       
                LAND,   // &&       
                BOR,    // |        
                BAND,   // &        
                XOR,    // ^
                LSH,    // <<
                RSH,    // >>
                SEMIC,  // ;
                ASGN,   // =
                IDENT,
                ICON,
};
// clang-format on

struct Token {
        enum TokenKind kind;
        union {
                int64_t icon;
                const char *scon;  // identifier string | string literal
        } value;
        struct Token *next;
};

struct Edecl {
        /* DECL */
        uint64_t type;
        char *name;
        struct Expr *value; /* can act like 
                                - value for decl
                                - value for 'return'
                                - ident for 'goto' 
                                - expr for expr-stmt
                            */

        enum EdeclKind { FUNC, DECL, S_IF, S_RETURN, S_COMP, S_EXPR } kind;
        /* STMT */
        struct Expr *cond;
        struct Edecl *then;
        struct Edecl *body;  // compound stmt

        struct Edecl *next;
};

enum ExprType {
        // clang-format off
        E_ADD, E_SUB, E_MUL, E_DIV, E_MOD,
        E_ICON, E_IDENT, E_LT, E_GT, E_LE, E_GE, E_EQ, E_NEQ,
        E_LOR, E_LAND, E_BOR, E_BAND, E_XOR, E_LSH, E_RSH,
        E_ASGN
        // clang-format on
};
struct Expr {
        enum ExprType kind;
        uint64_t value;
        char *ident;
        struct Expr *lhs;
        struct Expr *rhs;
};

/* GLOBALS */
int LEN; /* used in the scanning step to keep track of string length for identifiers and scon */
#define TYPE_INT 0x0000000000000003  // 0000,0000,0011

/* --------- HASH TABLE --------- */
struct KeyValuePair {
        const char *key;
        struct Sym *sym;
};

struct Sym {
        int64_t value;
        int offset;
};

#define TABLE_SIZE 4096
struct KeyValuePair ht[TABLE_SIZE];

int hash(const char *key) {
        unsigned hash = 1;
        int c;
        while ((c = *key++)) {
                hash = hash * 263 + c;
        }
        return (int)(hash % TABLE_SIZE);
}

void insert(const char *key, int64_t value) {
        int index = hash(key);
        ht[index].key = key;
        struct Sym *sym = malloc(sizeof(struct Sym));
        sym->value = value;
        sym->offset = 0;
        ht[index].sym = sym;
}

struct Sym *get(const char *key) {
        int index = hash(key);
        return ht[index].sym;
}
/* --------- END --------- */

// UTILS
bool iswhitespace(char c) { return c == ' ' || c == '\t' || c == '\n'; }
bool isidentifier(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isicon(char c) { return c >= '0' && c <= '9'; }
bool ispunctuation(char c) {
        return c == '(' || c == ')' || c == '{' || c == '}' || c == '>' || c == '<' || c == '=' ||
               c == ';' || c == '!' || c == '|' || c == '&' || c == '^' || c == '+' || c == '-' ||
               c == '*' || c == '/' || c == '%';
}

// FORWARD DECLARATIONS
struct Edecl *declaration(struct Token **token);
struct Expr *expr(int k, struct Token **token);
struct Expr *primary(struct Token **token);
struct Edecl *stmt(struct Token **token);
void cg_stmt(struct Edecl *lstmt);
char *cg_expr(struct Expr *cond);
char *nextr(void);
void prevr(char *r);
int indexify(struct Token *token);
struct Expr *asgn(struct Token **token);
void assignoffsets(struct Edecl **decls);

struct Token *newtoken(enum TokenKind kind, const char *lexeme) {
        struct Token *token = (struct Token *)malloc(sizeof(struct Token));
        assert(token != NULL);
        token->kind = kind;
        switch (kind) {
                        // clang-format off
                case INT: case IF: 
                case RETURN: break;
                case ICON: token->value.icon = strtoll(lexeme, NULL, 10); break;
                case IDENT: token->value.scon = strndup(lexeme, LEN); break;
                case OPAR:  case CPAR:  case OCBR:  case CCBR:  case LT:
                case GT:    case LE:    case GE:    case EQ:    case NEQ: 
                case LOR:   case LAND:  case BOR:   case BAND:  case XOR:
                case LSH:   case RSH:   case ADD:   case SUB:   case MUL:
                case DIV:   case MOD:
                case ASGN:
                case SEMIC: break;
                default: assert(0);
                        // clang-format on
        }
        return token;
}

struct Expr *newexpr(enum ExprType kind, struct Expr *lhs, struct Expr *rhs) {
        struct Expr *expr = malloc(sizeof(struct Expr));
        expr->kind = kind;
        expr->lhs = lhs;
        expr->rhs = rhs;
        return expr;
}

void addtoken(struct Token **head, struct Token **tail, struct Token *newtoken) {
        if (*tail == NULL)
                *head = *tail = newtoken;
        else {
                (*tail)->next = newtoken;
                *tail = newtoken;
        }
}

void consume(struct Token **token, enum TokenKind kind) {
        if ((*token)->kind != kind) {
                printf("Expected %d, but got %d\n", kind, (*token)->kind);
                assert(0);
        }
        *token = (*token)->next;
}

/* ----------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------ SCANNER -------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */
void scan(const char *program, struct Token **tokenlist) {
        int length = strlen(program);
        int start = 0;
        int current = 0;
        struct Token *head = NULL;
        struct Token *tail = NULL;

        while (current < length) {
                // skip whitespace
                while (current < length && iswhitespace(program[current])) current++;

                while (current < length && !iswhitespace(program[current])) {
                        start = current;
                        enum TokenKind kind = -1;
                        if (strncmp(program + current, "int", 3) == 0) { /* KEYWORDS */
                                current += 3;
                                kind = INT;
                        } else if (strncmp(program + current, "if", 2) == 0) {
                                current += 2;
                                kind = IF;
                        } else if (strncmp(program + current, "return", 6) == 0) {
                                current += 6;
                                kind = RETURN;
                        } else if (isidentifier(program[current])) { /* IDENTIFIER */
                                while (isidentifier(program[current])) current++;
                                LEN = current - start;
                                kind = IDENT;
                        } else if (ispunctuation(program[current])) { /* PUNCTUATION */
                                if (program[current] == '+') {
                                        current++;
                                        kind = ADD;
                                } else if (program[current] == '-') {
                                        current++;
                                        kind = SUB;
                                } else if (program[current] == '*') {
                                        current++;
                                        kind = MUL;
                                } else if (program[current] == '/') {
                                        current++;
                                        kind = DIV;
                                } else if (program[current] == '%') {
                                        current++;
                                        kind = MOD;
                                } else if (program[current] == '(') {
                                        current++;
                                        kind = OPAR;
                                } else if (program[current] == ')') {
                                        current++;
                                        kind = CPAR;
                                } else if (program[current] == '{') {
                                        current++;
                                        kind = OCBR;
                                } else if (program[current] == '}') {
                                        current++;
                                        kind = CCBR;
                                } else if (program[current] == '<') {
                                        current++;
                                        if (program[current] == '=') {
                                                current++;
                                                kind = LE;
                                        } else if (program[current] == '<') {
                                                current++;
                                                kind = LSH;
                                        } else
                                                kind = LT;
                                } else if (program[current] == '>') {
                                        current++;
                                        if (program[current] == '=') {
                                                current++;
                                                kind = GE;
                                        } else if (program[current] == '>') {
                                                current++;
                                                kind = RSH;
                                        } else
                                                kind = GT;
                                } else if (program[current] == ';') {
                                        current++;
                                        kind = SEMIC;
                                } else if (program[current] == '=') {
                                        current++;
                                        if (program[current] == '=') {
                                                current++;
                                                kind = EQ;
                                        } else
                                                kind = ASGN;
                                } else if (program[current] == '!') {
                                        current++;
                                        if (program[current] == '=') {
                                                current++;
                                                kind = NEQ;
                                        } else
                                                assert(0);
                                } else if (program[current] == '|') {
                                        current++;
                                        if (program[current] == '|') {
                                                current++;
                                                kind = LOR;
                                        } else
                                                kind = BOR;
                                } else if (program[current] == '&') {
                                        current++;
                                        if (program[current] == '&') {
                                                current++;
                                                kind = LAND;
                                        } else
                                                kind = BAND;
                                } else if (program[current] == '^') {
                                        current++;
                                        kind = XOR;
                                } else {
                                        assert(0);
                                }
                        } else if (isicon(program[current])) { /* INT LITERAL */
                                while (isicon(program[current])) current++;
                                kind = ICON;
                        } else {
                                printf("Unrecognized char: %c\n", program[current]);
                                assert(0);
                        }
                        struct Token *token = newtoken(kind, program + start);
                        addtoken(&head, &tail, token);
                }
        }
        *tokenlist = head;
}

void printTokens(struct Token *head) {
        struct Token *current = head;
        while (current != NULL) {
                if (current->kind == ICON) {
                        printf("ICON, Value: %lu\n", current->value.icon);
                } else if (current->kind == IDENT) {
                        printf("IDENT, Value: %s\n", current->value.scon);
                } else {
                        printf("%d\n", current->kind);
                }
                current = current->next;
        }
}

/* ----------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------- PARSER -------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */
struct Edecl *parse(struct Token *head) {
        struct Token *current = head;
        struct Edecl *decl = malloc(sizeof(struct Edecl)); /* FUNCTION */
        decl->kind = FUNC;
        while (current != NULL) {
                decl->type |= TYPE_INT;
                consume(&current, INT);

                decl->name = strdup(current->value.scon);
                consume(&current, IDENT);

                consume(&current, OPAR);
                consume(&current, CPAR);

                struct Edecl *ldeclhead = malloc(sizeof(struct Edecl));
                struct Edecl *ldecltail = ldeclhead;

                consume(&current, OCBR);

                while (current->kind != CCBR) {
                        if (current->kind == INT) {
                                ldecltail = ldecltail->next = declaration(&current);
                        } else {
                                struct Edecl *lstmt = stmt(&current);
                                ldecltail = ldecltail->next = lstmt;
                        }
                }
                assert(current->kind == CCBR);
                consume(&current, CCBR);

                decl->body = ldeclhead->next;
        }
        return decl;
}

struct Edecl *declaration(struct Token **token) {
        struct Edecl *ldecl = malloc(sizeof(struct Edecl));
        ldecl->kind = DECL;

        struct Token *current = *token;

        ldecl->type |= TYPE_INT;
        consume(&current, INT);

        ldecl->name = strdup(current->value.scon);
        consume(&current, IDENT);

        consume(&current, ASGN);

        struct Expr *value = malloc(sizeof(struct Expr));
        value->kind = E_ICON;
        value->value = current->value.icon;
        ldecl->value = value;
        consume(&current, ICON);

        insert(ldecl->name, ldecl->value->value);

        consume(&current, SEMIC);

        *token = current;
        return ldecl;
}

struct Edecl *stmt(struct Token **token) {
        struct Token *current = *token;

        struct Edecl *lstmt = malloc(sizeof(struct Edecl));
        if (current->kind == IF) {
                lstmt->kind = S_IF;
                consume(&current, IF);
                consume(&current, OPAR);

                /* EXPR */
                struct Expr *cond = expr(4, &current);
                lstmt->cond = cond;

                consume(&current, CPAR);
                consume(&current, OCBR);

                /* STMT */
                struct Edecl *then = stmt(&current);
                lstmt->then = then;

                consume(&current, CCBR);
        } else if (current->kind == RETURN) {
                lstmt->kind = S_RETURN;
                consume(&current, RETURN);
                lstmt->value = expr(4, &current);
                consume(&current, SEMIC);
        } else if (current->kind == IDENT) {
                lstmt->kind = S_EXPR;
                lstmt->value = asgn(&current);
                consume(&current, SEMIC);
        } else
                assert(0);
        *token = current;
        return lstmt;
}

/*
-----------------------------------------------------------------------------
prec    assoc   purpose     op
-----------------------------------------------------------------------------
1       left                ,
2       right   asgn        =, *=, /=, +=, -=, %=, <<=, >>=, &=, ^=, |=
3       right   cond        ? :
4       left    logor       ||
5       left    logand      &&
6       left    inclor      |
7       left    exclor      ^
8       left    and         &
9       left    equal       ==, !=
10      left    rel         <, >, <=, >=
11      left    shift       <<, >>
12      left    add         +, -
13      left    mul         *, /, %
        left    cast        
14      left    unary       ++, --, &, *, -, ~, !
15      left    postfix     ++, --, ->, .
                primary
-----------------------------------------------------------------------------
*/
// clang-format off
char prec[] = {4,     5,      6,     7,     8,      9,     9,     10,    10, 
               10,    10,     11,    11,    12,     12,    13,    13,    13,    -1};
int oper[] = { E_LOR, E_LAND, E_BOR, E_XOR, E_BAND, E_EQ,  E_NEQ, E_LT,  E_GT, 
               E_LE,  E_GE,   E_LSH, E_RSH, E_ADD,  E_SUB, E_MUL, E_DIV, E_MOD};

int indexify(struct Token *token) {
    if (token->kind == LOR) return 0;
    else if (token->kind == LAND) return 1;
    else if (token->kind == BOR) return 2;
    else if (token->kind == XOR) return 3;
    else if (token->kind == BAND) return 4;
    else if (token->kind == EQ) return 5;
    else if (token->kind == NEQ) return 6;
    else if (token->kind == LT) return 7;
    else if (token->kind == GT) return 8;
    else if (token->kind == LE) return 9;
    else if (token->kind == GE) return 10;
    else if (token->kind == LSH) return 11;
    else if (token->kind == RSH) return 12;
    else if (token->kind == ADD) return 13;
    else if (token->kind == SUB) return 14;
    else if (token->kind == MUL) return 15;
    else if (token->kind == DIV) return 16;
    else if (token->kind == MOD) return 17;
    else return 18;
}
// clang-format on

// assignment-expression:
//      conditional-expression
//      unary-expression assign-operator assignment-expression
struct Expr *asgn(struct Token **token) {
        struct Token *current = *token;
        /* will recognize all correct exprs as well as some incorrect ones since 
        if it isn't a conditinal expr, it must be a unary, not binary as here */
        struct Expr *lhs = expr(4, &current);
        if (current->kind == ASGN) {
                consume(&current, ASGN);
                struct Expr *rhs = asgn(&current);
                lhs = newexpr(E_ASGN, lhs, rhs);
        }
        *token = current;
        return lhs;
}

// binary expression
struct Expr *expr(int k, struct Token **token) {
        struct Token *current = *token;
        struct Expr *lhs = primary(&current);
        for (int k1 = prec[indexify(current)]; k1 >= k; k1--) {
                while (prec[indexify(current)] == k1) {
                        int op = indexify(current);
                        current = current->next;

                        struct Expr *rhs = expr(k1 + 1, &current);
                        lhs = newexpr(oper[op], lhs, rhs);
                }
        }
        *token = current;
        return lhs;
}

struct Expr *primary(struct Token **token) {
        struct Token *current = *token;
        struct Expr *expr = malloc(sizeof(struct Expr));

        if (current->kind == IDENT) {
                expr->ident = strdup(current->value.scon);
                expr->kind = E_IDENT;
                consume(&current, IDENT);
        } else if (current->kind == ICON) {
                expr->value = current->value.icon;
                expr->kind = E_ICON;
                consume(&current, ICON);
        } else
                assert(0);
        *token = current;
        return expr;
}

/* ----------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------- CODEGEN ------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */
void codegen(struct Edecl *decl) {
        printf("\n  .globl %s\n", decl->name);
        printf("\n%s:\n", decl->name);

        // prologue
        printf("  addi    sp,sp,-16\n");
        printf("  sd      s0,8(sp)\n");
        printf("  addi    s0,sp,16\n");

        // body
        struct Edecl *body = decl->body;

        struct Edecl *decls = body;
        assignoffsets(&decls);

        while (body != NULL) {
                cg_stmt(body);
                body = body->next;
        }

        // epilogue
        printf(".Lend:\n");
        printf("  ld      s0,8(sp)\n");
        printf("  addi    sp,sp,16\n");
        printf("  jr      ra\n");
}

void assignoffsets(struct Edecl **decls) {
        struct Edecl *current = *decls;
        int cnt = -20; /* 5 vars */
        while (current != NULL) {
                if (current->kind != DECL) {
                        current = current->next;
                        continue;
                }
                struct Sym *sym = get(current->name);
                sym->offset = cnt;
                cnt += 4;

                char *rg = nextr();
                printf("  li      %s,%lu\n", rg, sym->value);
                printf("  sw      %s,%d(s0)\n", rg, sym->offset);
                prevr(rg);

                current = current->next;
        }
}

void cg_stmt(struct Edecl *lstmt) {
        if (lstmt->kind == S_IF) {
                char *rg = cg_expr(lstmt->cond);
                printf("  beqz    %s,.L1end\n", rg);
                prevr(rg);
                cg_stmt(lstmt->then);
                printf(".L1end:\n");
        } else if (lstmt->kind == S_RETURN) {
                char *rg = cg_expr(lstmt->value);
                printf("  mv      a0,%s\n", rg);
                printf("  j      .Lend\n");
                prevr(rg);
        } else if (lstmt->kind == S_EXPR) {
                cg_expr(lstmt->value);
        } else
                ; /* declaration */
}

char *cg_expr(struct Expr *cond) {
        assert(cond != NULL);
        char *rg = nextr();
        if (cond->kind == E_ICON) {
                printf("  li      %s,%lu\n", rg, cond->value);
                return rg;
        } else if (cond->kind == E_IDENT) {
                struct Sym *sym = get(cond->ident);
                printf("  lw      %s,%d(s0)\n", rg, sym->offset);
                return rg;
        } else if (cond->kind == E_ASGN) {
                struct Sym *sym = get(cond->lhs->ident);
                char *rhs = cg_expr(cond->rhs);
                printf("  sw      %s,%d(s0)\n", rhs, sym->offset);
                printf("  mv      %s,%s\n",
                       rg,
                       rhs); /* just so that we can return rg. otherwise, no need to have this and return anything here */
                prevr(rhs);
                return rg;
        } else {
                char *lhs = cg_expr(cond->lhs);
                char *rhs = cg_expr(cond->rhs);

                if (cond->kind == E_ADD) {
                        printf("  add      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_SUB) {
                        printf("  sub      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_MUL) {
                        printf("  mul      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_DIV) {
                        printf("  div      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_MOD) {
                        printf("  rem      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_GT) {
                        printf("  slt      %s,%s,%s\n", rg, rhs, lhs);
                } else if (cond->kind == E_LT) {
                        printf("  slt      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_LE) {
                        printf("  slt      %s,%s,%s\n", rg, rhs, lhs);
                        printf("  xori      %s,%s,1\n", rg, rg); /* invert least significant bit */
                } else if (cond->kind == E_GE) {
                        printf("  slt      %s,%s,%s\n", rg, lhs, rhs);
                        printf("  xori      %s,%s,1\n", rg, rg);
                } else if (cond->kind == E_EQ) {
                        printf("  xor      %s,%s,%s\n", rg, lhs, rhs);
                        printf("  sltiu     %s,%s,1\n", rg, rg);
                } else if (cond->kind == E_NEQ) {
                        printf("  xor      %s,%s,%s\n", rg, lhs, rhs);
                        printf("  sltu     %s,x0,%s\n", rg, rg);
                } else if (cond->kind == E_LOR || cond->kind == E_LAND || cond->kind == E_BOR ||
                           cond->kind == E_BAND || cond->kind == E_XOR) {
                        if (cond->kind == E_LOR || cond->kind == E_BOR)
                                printf("  or      %s,%s,%s\n", rg, lhs, rhs);
                        else if (cond->kind == E_XOR)
                                printf("  xor      %s,%s,%s\n", rg, lhs, rhs);
                        else
                                printf("  and      %s,%s,%s\n", rg, lhs, rhs);
                } else if (cond->kind == E_LSH || cond->kind == E_RSH) {
                        if (cond->kind == E_LSH)
                                printf("  sll      %s,%s,%s\n", rg, lhs, rhs);
                        else
                                printf("  srl      %s,%s,%s\n", rg, lhs, rhs);
                } else
                        assert(0);

                prevr(lhs);
                prevr(rhs);
        }
        return rg;
}

static int regCount = 1;

// get next register
char *nextr(void) {
        int size = 0;
        if (regCount < 10)
                size = 1;
        else if (regCount < 100)
                size = 2;
        else {
                printf("Too many registers in use\n");
                exit(1);
        }

        char *reg = calloc(size + 1, sizeof(char));
        snprintf(reg, sizeof(reg), "a%d", regCount);
        regCount++;
        return reg;
}

void prevr(char *r) {
        free(r);
        regCount--;
}
/* ----------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------- MAIN --------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */

int main(int argc, char **argv) {
        if (argc < 2) assert(0);

        struct Token *tokenlist = NULL;
        scan(argv[1], &tokenlist);
        struct Edecl *decllist = parse(tokenlist);
        codegen(decllist);

        return 0;
}
