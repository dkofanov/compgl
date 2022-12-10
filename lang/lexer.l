%{
#include "token_processor.h"
#include "grammar.l.hpp"

extern "C" int yylex (void);
size_t line_no = 1;

int yyerror() {
    printf("lex_error line_no: %lu\n", line_no);
    return line_no;
}
%}

%%
[/][/].*$ {}
[\t] { abort(); }
[ ] { }
[\n] { line_no++; }

[{] { return B_BEGIN; }
[}] { return B_END; }

[[] { return A_BEGIN; }
[]] { return A_END; }

[(] { return E_BEGIN; }
[)] { return E_END; }

while {
    return WHILE;
}

if {
    return IF;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval = std::string(yytext);
    printf("Token ([%s])\n", yytext);
    return ID;
}
[-]?[0-9]+ {
    yylval = atol(yytext);
    return NUM;
}

[-][>] {
    return RIGHT_ARROW;
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
[*/%] {
    yylval = yytext;
    return OP2;
}
[:] {
    return COLON;
}
[;] {
    return STATEMENT_TERMINATOR;
}
[.] {
    return DOT;
}
[,] {
    return COMMA;
}
[?] {
    return CONSOLE_OBSERVE_VAR;
}

[#]PUT_PIXOL {
    return PUT_PIXOL;
}
[#]INIT_GRAPHICS {
    return INIT_GRAPHICS;
}
[#]FLUSH {
    return FLUSH;
}
[#]RAND {
    return RAND;
}
[#]SLEEP_US {
    return SLEEP;
}
[#]SRAND {
    return SRAND;
}
%%
