#include <cstdio>

extern "C" FILE *yyin;
extern "C" int yydebug;
extern "C" int yyparse (void);
extern "C" void yyerror(const char *);
extern "C" int yylex (void);

