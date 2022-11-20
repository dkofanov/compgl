%{
#include "token_processor.h"
#include <cstdio>
#include "lexer_decls.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

using token_t = Token;
%}

%token NUM
%token OP1
%token OP2
%token EXPR_TERMINATOR

%%
PROGRAM:
    EXPRESSION {printf("%ld\n", $1.ToNum());} |
    EXPRESSION {printf("%ld\n", $1.ToNum());} PROGRAM;

EXPRESSION:
    TERM OP1 EXPRESSION {
        switch($2.ToOp()) {
            case Token::Op::ADD: $$ = $1.ToNum() + $3.ToNum(); break;
            case Token::Op::SUB: $$ = $1.ToNum() - $3.ToNum(); break;
            default: UNREACHABLE();
        }
    } |
    TERM {
        $$ = $1;
    } EXPR_TERMINATOR;

TERM:
    NUM OP2 TERM {
        switch($2.ToOp()) {
            case Token::Op::MUL: $$ = $1.ToNum() * $3.ToNum(); break;
            case Token::Op::DIV: $$ = $1.ToNum() / $3.ToNum(); break;
            default: UNREACHABLE();
        }
    }|
    NUM {
        $$ = $1;
    };
%%

