%{
#include "token_processor.h"
#include "grammar.l.hpp"

extern "C" int yylex (void);

%}

%%
[#].*$ {}

[\t] { abort(); }
[ \n] {}

[-]?[0-9]+ {
    yylval = atol(yytext);
    return NUM;
}

[+-] {
    yylval = yytext;
    return OP1;
}

[;] {
    return EXPR_TERMINATOR;
}

[*/] {
    yylval = yytext;
    return OP2;
}
%%