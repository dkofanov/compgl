%{
#include "token_processor.h"
#include <cstdio>
#include "lexer_decls.h"
#include "lang_main.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

using tok = Token;
#define __ IRG->

%}

%token NUM
%token RIGHT_ARROW
%token OP0
%token OP1
%token OP2
%token COLON
%token STATEMENT_TERMINATOR
%token B_BEGIN
%token E_BEGIN
%token B_END
%token E_END
%token A_BEGIN
%token A_END
%token ID
%token ASSIGN
%token IF
%token WHILE
%token DOT
%token CONSOLE_OBSERVE_VAR

%%
PROGRAM: /* use right-recursion to resolve obj-obj link issues: */
    OBJ_DECLARATION PROGRAM |
    %empty;

OBJ_DECLARATION:
    ID[typename_alias] { __ BeginObject($typename_alias.To<Token::Id>()); } DEFINE_OBJECT { __ EndObject($typename_alias.To<Token::Id>()); };

DEFINE_OBJECT:
    E_BEGIN DEFINE_INPUTS_SEQ E_END RIGHT_ARROW
    E_BEGIN DEFINE_OUTPUTS_SEQ E_END COLON
    MEMBER_FUNCTION;

DEFINE_INPUTS_SEQ:
    %empty |
    DEFINE_INPUTS_SEQ DEFINE_INPUT STATEMENT_TERMINATOR;

DEFINE_OUTPUTS_SEQ:
    %empty |
    DEFINE_OUTPUTS_SEQ DEFINE_OUTPUT STATEMENT_TERMINATOR;

DEFINE_INPUT:
    ID[name] COLON ID[type] { __ GetCurrentObject()->AddInput($name.To<Token::Id>(), __ ResolveTypeByName($type.To<Token::Id>())); } 

DEFINE_OUTPUT:
    ID[name] COLON ID[type] { __ GetCurrentObject()->AddOutput($name.To<Token::Id>(), __ ResolveTypeByName($type.To<Token::Id>())); } 

MEMBER_FUNCTION:
    { 
        __ BeginMemberFunction(); 
        __ GetCurrentObject()->SetInitializer( __ GetCurrentFunction());
    }
    SCOPED_BLOCK[entry_bb] { 
        __ EndMemberFunction();
    };

SCOPED_BLOCK:
    B_BEGIN  { __ BeginSB();} STATEMENT_SEQ B_END { $$ = tok( __ EndSB()); };

STATEMENT_SEQ:
    STATEMENT STATEMENT_TERMINATOR STATEMENT_SEQ |
    %empty;

STATEMENT:
    VAR_DECL_SCOPE |
    VAR_INITIALIZATION |
    IF_STATEMENT |
    WHILE_STATEMENT |
    OBJ_INVOCATION |
    VAR_PRINT;

WHILE_STATEMENT:
    WHILE E_BEGIN {
        // Insert scope that encapsulates loop body and header:
        __ BeginLinkedSB();
    } COMPARISON[cmp] E_END SCOPED_BLOCK[sb] {
        __ FinalizeWhileStatement($cmp, $sb);
        __ EndSB();
    };

IF_STATEMENT:
    IF E_BEGIN {
        // Insert scope that encapsulates if-comparison and cond-branch.
        // Otherwise, we can't simply resolve the correct insert point for
        // cond-branch.
        __ BeginLinkedSB();
    } COMPARISON[cmp] E_END SCOPED_BLOCK[sb] {
        __ FinalizeIfStatement($cmp, $sb);
        __ EndSB();
    };

COMPARISON:
    ARITHM_EXPRESSION OP0 ARITHM_EXPRESSION {
        $$ = tok( __ CreateCmp($2, $1, $3));
    };

VAR_DECL_SCOPE:
    ID[var_name] COLON ID[type] A_BEGIN ARITHM_EXPRESSION[ar_len] A_END { 
        $$ = tok( __ DeclareLocalVar($type, $var_name, $ar_len.To<Value>()));
    } |
    ID[var_name] COLON ID[type] { 
        $$ = __ DeclareLocalVar($type, $var_name, __ I64(1));
    };

VAR_INITIALIZATION:
    LVALUE_REF[lval_typed_ref_tok] ASSIGN INITIALIZER[rval_tok] {
        auto lval_typed_ref = $lval_typed_ref_tok.To<Token::TypedRef>();
        auto rval = $rval_tok.To<Token::Value>();
        __ CreateStore(lval_typed_ref.second, rval);
        // return typed reference:
        $$ = $lval_typed_ref_tok;
    };

INITIALIZER:
    ARITHM_EXPRESSION;


VAR_PRINT:
    LVALUE_REF CONSOLE_OBSERVE_VAR {
        __ CreateCallIntrinsicPrintVar($1);
    };

ARITHM_EXPRESSION:
    TERM OP1 ARITHM_EXPRESSION {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |

    TERM {
        $$ = $1;
    };

TERM:
    RVALUE OP2 TERM {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |
    RVALUE;


RVALUE:
    NUM {
        $$ = tok( __ CreateNum($1));
    } |
    XVALUE_REF[xval_ref] {
        $$ = tok( __ LoadRef($xval_ref.To<Token::TypedRef>()));
    } |
    E_BEGIN ARITHM_EXPRESSION E_END {
        $$ = $2;
    };

XVALUE_REF:
    ID[obj] DOT {
        __ AppendAccessChain($obj.To<Token::Id>());
    } XVALUE_REF[xval]  { $$ = $xval; } |
    ID[last_obj] {
        __ AppendAccessChain($last_obj.To<Token::Id>());
        $$ = tok ( __ ResolveAccessChain());
    };

LVALUE_REF:
    XVALUE_REF;

OBJ_INVOCATION:
    LVALUE_REF[lval] E_BEGIN E_END {
        auto this_pointer = $lval.To<Token::TypedRef>().second;
        auto ll_type = $lval.To<Token::TypedRef>().first;
        ASSERT( ll_type->isStructTy());
        auto struct_ll_type = static_cast<llvm::StructType *>(ll_type);
        auto obj_template = __ GetDeclaredHLType(struct_ll_type->getName());
        __ InvokeObject(obj_template, this_pointer);
    }

%%

#undef __
