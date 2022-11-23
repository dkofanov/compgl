%{
#include "token_processor.h"
#include "grammar.l.hpp"

extern "C" int yylex (void);

%}

%%
[#].*$ {}
[\t] { abort(); }
[ \n] {}

[{] { return B_BEGIN; }
[}] { return B_END; }

[(] { return E_BEGIN; }
[)] { return E_END; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval = std::string(yytext);
    return ID;
}
[-]?[0-9]+ {
    yylval = atol(yytext);
    return NUM;
}

!=|==|>=|<=|<|> {
    // Logical operations
    yylval = yytext;
    return OP0;
}

[=] {
    return ASSIGN;
}

[+-] {
    yylval = yytext;
    return OP1;
}
[*/] {
    yylval = yytext;
    return OP2;
}
[;] {
    return STATEMENT_TERMINATOR;
}

[?] {
    return CONSOLE_OBSERVE_VAR;
}
%%