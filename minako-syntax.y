%define parse.error verbose
%define parse.trace

%code requires {
	#include <stdio.h>
	#define YYDEBUG 1
	
	extern void yyerror(const char*);
	extern FILE* yyin;
}

%code {
	extern int yylex();
	extern int yylineno;
}

%union {
	char* string;
	double floatValue;
	int intValue;
}

%token AND           "&&"
%token OR            "||"
%token EQ            "=="
%token NEQ           "!="
%token LEQ           "<="
%token GEQ           ">="
%token LSS           "<"
%token GRT           ">"
%token KW_BOOLEAN    "bool"
%token KW_DO         "do"
%token KW_ELSE       "else"
%token KW_FLOAT      "float"
%token KW_FOR        "for"
%token KW_IF         "if"
%token KW_INT        "int"
%token KW_PRINTF     "printf"
%token KW_RETURN     "return"
%token KW_VOID       "void"
%token KW_WHILE      "while"
%token CONST_INT     "integer literal"
%token CONST_FLOAT   "float literal"
%token CONST_BOOLEAN "boolean literal"
%token CONST_STRING  "string literal"
%token ID            "identifier"

%left NOELSE
%left KW_ELSE

%%

program						: declassignment program
							| functiondefinition program
							| %empty
							;

functiondefinition			: type id '(' parameterlistoption ')' '{' statementlist '}'
							;

parameterlistoption			: parameterlist
							| %empty
							;

parameterlist				: type id parameterlistcont
							;

parameterlistcont			: ',' type id parameterlistcont
							| %empty
							;

statementlist				: block statementlist
							| %empty
							;

functioncall				: id '(' assignmentlist ')'
							;

assignmentlist				: %empty
							| assignment assignmentlistoption
							;

assignmentlistoption 		: ',' assignment assignmentlistoption
							| %empty;
							;

block						: '{' statementlist '}'
							| statement
							;

statement					: ifstatement
							| forstatement
							| whilestatement
							| returnstatement ';'
							| dowhilestatement ';'
							| printf ';'
							| declassignment ';'
							| statassignment ';'
							| functioncall ';'
							;

statblock					: '{' statementlist '}'
							| statement
							;

ifstatement					: KW_IF '(' assignment ')' statblock KW_ELSE statblock
							| KW_IF '(' assignment ')' statblock %prec NOELSE
							;

forstatement				: KW_FOR '(' forstatementoption ';' expr ';' statassignment ')' statblock
							;

forstatementoption			: assignment
							| declassignment
							;

dowhilestatement			: KW_DO statblock KW_WHILE '(' assignment ')'
							;

whilestatement				:	KW_WHILE '(' assignment ')' statblock
							;

returnstatement				: KW_RETURN assignment
							| KW_RETURN
							;

printf						: KW_PRINTF '(' printfopt ')'
							;

printfopt					: assignment
							| CONST_STRING
							;


declassignment				: type id declassignmentoption
							;

declassignmentoption		: %empty
							| '=' assignment
							;

type						: KW_BOOLEAN
							| KW_FLOAT
							| KW_INT
							| KW_VOID
							;
	
statassignment				:	id '=' assignment
							;

assignment					:	id '=' assignment
							| expr
							;

expr						: simpexpr operation
							;

simpexpr					: simpexprbegin simpexproption
							;

simpexprbegin				:	'-' term
							| term;

simpexproption				: '+' term simpexproption
							| '-' term simpexproption
							| OR term simpexproption
							| %empty
							;
	

operation					: %empty
							| EQ simpexpr
							| NEQ simpexpr
							| LEQ simpexpr
							| GEQ simpexpr
							| LSS  simpexpr
							| GRT  simpexpr
							;

term						: factor compfactor
							;

compfactor					: '*' factor compfactor
							| '/' factor compfactor
							| AND factor compfactor
							| %empty
							;

factor						: CONST_INT
							| CONST_FLOAT
							| CONST_BOOLEAN
							| functioncall
							| id
							| '(' assignment ')'
							;

id							: ID
							;
	
%%

int main(int argc, char *argv[])
{
int result;

	if(argc == 1){
		yyin = stdin;
	}
	
	if(argc == 2){	
		yyin = fopen(argv[1], "rt");
		
		if(yyin == 0){
			printf("Too many arguments\n");
			return -1;			
		}
		
	}
	else if(argc > 2 ){
		printf("zu viele Argumente wurden eingegeben\n");
		return -1;
	}	
	result = yyparse();
	
	if(result == 0){			
		return 0;		
	}
	else{
		return -1;
	}			
		
		
}

void yyerror(const char* msg)
{
	printf("Fehler in Zeile:");
	printf("%i\n", yylineno);	
	fprintf (stderr, "%s\n", msg);
}
