%{
#include "token_processor.h"
#include <cstdio>
#include "lexer_decls.h"
#include "lang_main.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

using tok = Token;
#define __ IRG. 

%}

%token NUM
%token OP0
%token OP1
%token OP2
%token STATEMENT_TERMINATOR
%token B_BEGIN
%token E_BEGIN
%token B_END
%token E_END
%token ID
%token ASSIGN
%token CONSOLE_OBSERVE_VAR

%%
FUNCTION: BB;

BB:
B_BEGIN  { __ BeginBB();} STATEMENT_SEQ B_END;

STATEMENT_SEQ:
    STATEMENT {printf("ReadStatement\n");} STATEMENT_TERMINATOR STATEMENT_SEQ |
    %empty;

STATEMENT:
    VAR_INITIALIZATION |
    VAR_PRINT;


COMPARISON:
    ARITHM_EXPRESSION OP0 ARITHM_EXPRESSION {
        $$ = tok( __ CreateCmp($2, $1, $3));
    };

VAR_INITIALIZATION:
    VAR_DECL_LOCAL ASSIGN INITIALIZER {
        $$ = tok( __ CreateStore($1, $3));
    };

INITIALIZER:
    ARITHM_EXPRESSION;

VAR_DECL_LOCAL:
    ID ID { 
        $$ = tok( __ DeclareLocalVar($1, $2));
    };

VAR_PRINT:
    ID CONSOLE_OBSERVE_VAR {
        auto typed_var_ref = __ ResolveVar($1);
        __ CreateCallIntrinsicPrintVar(typed_var_ref.second);
    };

ARITHM_EXPRESSION:
    TERM OP1 ARITHM_EXPRESSION {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |

    TERM {
        $$ = $1;
    };

TERM:
    ARITHM_OPERAND OP2 TERM {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |
    ARITHM_OPERAND;


ARITHM_OPERAND:
    NUM {
        $$ = tok( __ CreateNum($1));
    } |
    ID {
        $$ = tok( __ LoadVar($1));
    } |
    E_BEGIN ARITHM_EXPRESSION E_END {
        $$ = $2;
    };

%%

#undef __
