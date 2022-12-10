%{
#include "token_processor.h"
#include <cstdio>
#include "lexer_decls.h"
#include "lang_main.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

using tok = Token;
#define __ IRG->

extern "C" size_t line_no;

void yyerror (char const *s)
{

  fprintf (stderr, "%s (lineno:%lu)\n", s, line_no);
}


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
%token COMMA

%token CONSOLE_OBSERVE_VAR
%token PUT_PIXOL
%token INIT_GRAPHICS
%token FLUSH
%token RAND
%token SLEEP


%%
PROGRAM: /* use right-recursion to resolve obj-obj link issues: */
    OBJ_DECLARATION PROGRAM |
    %empty;

OBJ_DECLARATION:
    ID[typename_alias] { __ BeginObject($typename_alias.To<Token::Id>()); } DEFINE_OBJECT { __ EndObject($typename_alias.To<Token::Id>()); };

DEFINE_OBJECT:
    E_BEGIN DEFINE_INITIALIZERS_SEQ E_END RIGHT_ARROW
    E_BEGIN DEFINE_MEMBERS_SEQ E_END COLON
    MEMBER_FUNCTION;

DEFINE_INITIALIZERS_SEQ:
    %empty |
    DEFINE_INITIALIZERS_SEQ DEFINE_INITIALIZER STATEMENT_TERMINATOR;

DEFINE_MEMBERS_SEQ:
    %empty |
    DEFINE_MEMBERS_SEQ DEFINE_MEMBER STATEMENT_TERMINATOR;

DEFINE_INITIALIZER:
    ID[name] COLON ID[type] A_BEGIN A_END { 
        __ GetCurrentObject()->AddInitializer($name.To<Id>(), __ ResolveTypeByName<true>($type.To<Id>()));
    } | 
    ID[name] COLON ID[type] { __ GetCurrentObject()->AddInitializer($name.To<Id>(), __ ResolveTypeByName($type.To<Id>())); } 

DEFINE_MEMBER:
    ID[name] COLON ID[type] { __ GetCurrentObject()->AddMember($name.To<Id>(), __ ResolveTypeByName($type.To<Id>())); } 

MEMBER_FUNCTION:
    { 
        __ BeginMemberFunction(); 
        __ GetCurrentObject()->SetFunction( __ GetCurrentFunction());
    }
    SCOPED_BLOCK[entry_bb] { 
        __ EndMemberFunction();
    };

SCOPED_BLOCK:
    B_BEGIN  { __ BeginSB();} STATEMENT_SEQ B_END { $$ = __ EndSB(); };

STATEMENT_SEQ:
    STATEMENT STATEMENT_TERMINATOR STATEMENT_SEQ |
    %empty;

STATEMENT:
    VAR_DECL_SCOPE |
    VAR_INITIALIZATION |
    IF_STATEMENT |
    WHILE_STATEMENT |
    OBJ_INVOCATION |
    VAR_PRINT |
    GRAPHIC_INTRINSICS;

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
    RVALUE OP0 RVALUE {
        $$ = tok( __ CreateCmp($2, $1, $3));
    };

VAR_DECL_SCOPE:
    ID[var_name] COLON ID[type] A_BEGIN ARITHM_EXPRESSION[ar_len] A_END { 
        __ DeclareLocalVar($type, $var_name, $ar_len.To<Value>());
        $$ = __ PromoteToLocalArray($var_name);
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
    ARITHM_EXPRESSION |
    INTRINSIC_RAND;


VAR_PRINT:
    LVALUE_REF CONSOLE_OBSERVE_VAR {
        __ CreateCallIntrinsicPrintVar($1);
    };

ARITHM_EXPRESSION:
    ARITHM_EXPRESSION OP1 TERM  {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |

    TERM {
        $$ = $1;
    };

TERM:
    TERM OP2 RVALUE {
        $$ = tok( __ CreateOp($2, $1, $3));
    } |
    RVALUE;


RVALUE:
    NUM {
        $$ = __ CreateNum($1);
    } |
    XVALUE_REF[xval_ref] {
        $$ = __ LoadRef($xval_ref.To<TypedRef>());
    } |
    E_BEGIN ARITHM_EXPRESSION E_END {
        $$ = $2;
    };

XVALUE_REF:
    DOT XVALUE_REF[xval] { $$ = $xval; }
    |
    ID[ptr] A_BEGIN
    { __ AccessChainAllocCtx(); }
    ARITHM_EXPRESSION[offset]
    { __ AccessChainFreeCtx(); } 
    A_END {
        __ AppendAccessChain($ptr.To<Id>(), $offset.To<Value>());
    } XVALUE_REF[xval]  { $$ = $xval; }
    |
    ID[obj] {
        __ AppendAccessChain($obj.To<Id>());
    } XVALUE_REF[xval]  { $$ = $xval; }
    |
    %empty { $$ =  __ ResolveAccessChain(); };


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

GRAPHIC_INTRINSICS:
    PUT_PIXOL E_BEGIN RVALUE[x] COMMA RVALUE[y] E_END {
        __ intrinsicPutPixol($x.To<Value>(), $y.To<Value>());
    } |
    INIT_GRAPHICS E_BEGIN RVALUE[w] COMMA RVALUE[h] COMMA RVALUE[scale] E_END {
        __ intrinsicInitGraphics($w.To<Value>(), $h.To<Value>(), $scale.To<Value>());
    } |
    FLUSH E_BEGIN E_END {
        __ intrinsicFlush();
    } |
    SLEEP E_BEGIN RVALUE[us] E_END {
        __ intrinsicSleep($us.To<Value>());
    };
    
INTRINSIC_RAND: RAND E_BEGIN E_END {
        $$ = __ intrinsicRand();
    }

%%

#undef __
