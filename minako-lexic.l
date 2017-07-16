%option noyywrap yylineno nounput noinput never-interactive

WS     [[:space:]]
INT    [[:digit:]]+
FLT    {INT}"."{INT}|"."{INT}

%x COMMENT

%{
	#include <stdlib.h>
	#include "minako-syntax.tab.h"
%}

%%

{WS}+
"//".*\n
"/*"          BEGIN(COMMENT);
<COMMENT>"*/" BEGIN(INITIAL);
<COMMENT>"*"[^/]
<COMMENT>[^*]+

"&&"          return AND;
"||"          return OR;
"=="          return EQ;
"!="          return NEQ;
"<="          return LEQ;
">="          return GEQ;
"<"           return LSS;
">"           return GRT;

"bool"        return KW_BOOLEAN;
"do"          return KW_DO;
"else"        return KW_ELSE;
"float"       return KW_FLOAT;
"for"         return KW_FOR;
"if"          return KW_IF;
"int"         return KW_INT;
"printf"      return KW_PRINTF;
"return"      return KW_RETURN;
"void"        return KW_VOID;
"while"       return KW_WHILE;

{FLT}([eE][-+]?{INT})? |
{INT}([eE][-+]?{INT})  { yylval.floatValue = atof(yytext); return CONST_FLOAT; }
{INT}       { yylval.intValue = atoi(yytext); return CONST_INT; }
"true"      { yylval.intValue = 1; return CONST_BOOLEAN; }
"false"     { yylval.intValue = 0; return CONST_BOOLEAN; }
[[:alpha:]][[:alnum:]_]* {
	yylval.string = malloc(yyleng + 1);
	strcpy(yylval.string, yytext);
	return ID;
}
\"[^\n\"]*\" {
	yylval.string = malloc(yyleng - 2 + 1);
	memcpy(yylval.string, yytext + 1, yyleng - 2);
	yylval.string[yyleng - 2] = 0;
	return CONST_STRING;
}

.             return yytext[0];

%%
