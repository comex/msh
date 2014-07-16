#pragma once
#include "util/vecs.h"
#include <stdint.h>
#include <stdbool.h>

enum ast_type {
    AST_EXPAND,
    AST_LIT,
    AST_VAR,
    AST_PATH,
    AST_EXEC,
    AST_LIST,
    AST_BLOCK,
    AST_PIPE,
    AST_SEQUENCE,
};

#define AST_PTR(ty) struct { typeof(typeof(ty) *) p; }
#define AST_GET(ptr) ((ptr).p)

struct ast_str {
    uint32_t len;
    char_ptr data;
};

union ast {
    enum ast_type type;
    // expand: *expr - only valid inside EXEC or LIST
    struct {
        enum ast_type type;
        AST_PTR(union ast) sub;
    } expand;
    // lit: a literal string
    struct {
        enum ast_type type;
        // xxx - intval
        struct ast_str str;
    } lit;
    // var: $x, &$x
    struct {
        enum ast_type type;
        bool is_ref;
        struct ast_str name;
    } var;
    // path: &foo`bar```
    struct {
        enum ast_type type;
        bool is_ref;
        uint16_t num_keys;
        AST_PTR(union ast) name;
        AST_PTR(AST_PTR(union ast)) keys;
    } var;
    // exec: [foo expr expr... redirs... &?]
    struct {
        enum ast_type type;
        bool is_async;
        uint16_t num_args;
        uint16_t num_redirs;
        AST_PTR(union ast) cmd;
        AST_PTR(AST_PTR(union ast)) args;
        AST_PTR(AST_PTR(struct ast_redir)) redirs;
    } exec;
    // list: (expr expr...)
    struct {
        enum ast_type type;
        AST_PTR(AST_PTR(union ast)) elems;
    } list;
    // block: {cmds...}
    struct {
        enum ast_type type;
        AST_PTR(union ast) cmd;
    } block;
    // pipe: [exec1 | exec2 | exec3]
    // sequence: [cmd1; cmd2; cmd3]
    struct {
        enum ast_type type;
        AST_PTR(AST_PTR(union ast)) execs;
    } pipe, sequence;
};

str ast_dump(union ast *bit);
