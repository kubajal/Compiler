%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include "minako.h"
%}

DIGIT [0-9]
LETTER [a-zA-Z]
INTEGER {DIGIT}+
FLOAT {INTEGER}"."{INTEGER}|"."{INTEGER}

%%
"&&"                                                            return AND;
"||"                                                            return OR;
"=="                                                            return EQ;
"!="                                                            return NEQ;
"<="                                                            return LEQ;
">="                                                            return GEQ;
"<"                                                             return LSS;
">"                                                             return GRT;
"bool"                                                          return KW_BOOLEAN;
"do"                                                            return KW_DO;
"else"                                                          return KW_ELSE;
"float"                                                         return KW_FLOAT;
"for"                                                           return KW_FOR;
"if"                                                            return KW_IF;
"int"                                                           return KW_INT;
"printf"                                                        return KW_PRINTF;
"return"                                                        return KW_RETURN;
"void"                                                          return KW_VOID;
"while"                                                         return KW_WHILE;
{INTEGER}                                                       {yylval.intValue = atoi(yytext); return CONST_INT;}
{FLOAT}([eE]([-+])?{INTEGER})?|{INTEGER}[eE]([-+])?{INTEGER}    {yylval.floatValue = atof(yytext); return CONST_FLOAT;}
"true"|"false"                                                  {if(*yytext == 't') yylval.intValue = 1; else yylval.intValue = 0; return CONST_BOOLEAN;}
"\""[^\n\"]*"\""                                                {yylval.string = yytext; return CONST_STRING;}
({LETTER})+({DIGIT}|{LETTER})*                                  {yylval.string = yytext; return ID;}
[ \n\t]+                                                        ;
\/\/[^\n]*                                                      ;
"/*"((\*+[^/*])|([^*]))*\**"*/"                                 ;
"+"                                                             return *yytext;
"*"                                                             return *yytext;
"/"                                                             return *yytext;
"="                                                             return *yytext;
","                                                             return *yytext;
";"                                                             return *yytext;
"("                                                             return *yytext; 
")"                                                             return *yytext; 
"{"                                                             return *yytext; 
"}"                                                             return *yytext;
<<EOF>>                                                         return EOF;
.                                                               {fprintf(stderr, "Ungueltiges Zeichen in Zeile %i \n", yylineno); exit(-1);}
%%

int yywrap(void) {
return 1;
}
