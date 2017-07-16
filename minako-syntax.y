%define parse.error verbose
%define parse.trace

%code requires {
	#include <stdlib.h>
	#include <stdarg.h>
	#include "symtab.h"
	#include "syntree.h"
	
	extern void yyerror(const char*, ...);
	extern int yylex();
	extern int yylineno;
	extern FILE* yyin;
}

%code provides {
	extern symtab_t* tab;
	extern syntree_t* ast;
}

%code {
	/* globale Zeiger auf Symboltabelle und abstrakten Syntaxbaum */
	symtab_t* tab;
	syntree_t* ast;
	
	/* interner (globaler) Zeiger auf die aktuell geparste Funktion */
	static symtab_symbol_t* func;
	
     /**@brief Kombiniert zwei Ausdrücke einer binären Operation und stellt
	* sicher, dass sie auf deren Typen definiert ist.
      * @param lhs  linke Seite
      * @param rhs  rechte Seite
      * @param op   Operator
      * @return ID des Operatorknoten
      */
	static syntree_nid
	combine(syntree_nid id1, syntree_nid id2, syntree_node_tag op);
	
     /**@brief Testet, ob eine Zuweisung zwischen Instanzen zweier Typen
	* erlaubt ist.
      * @param lhs  Zieltyp
      * @param rhs  Quelltyp
      * @return 1, falls \p rhs sich an \p lhs zuweisen lässt,\n
      *         0, ansonsten
      */
	static int
	matchTypes(syntree_node_type type1, syntree_node_type type2);
	
	/* Hilfsfunktionen */
	
	/**@brief Gibt den Zeiger auf einen Knoten der entsprechenden ID zurück.
	 */
	static inline syntree_node_t*
	nodePtr(syntree_nid id) { return syntreeNodePtr(ast, id); }
	
	/**@brief Gibt den Knotentyp zurück.
	 */
	static inline syntree_node_type
	nodeType(syntree_nid id) { return nodePtr(id)->type; }
	
	/**@brief Gibt den Wert eines Knotens zurück.
	 */
	static inline union syntree_node_value_u*
	nodeValue(syntree_nid id) { return &nodePtr(id)->value; }
	
	/**@brief Gibt den ersten Kindknoten eines Containers zurück.
	 */
	static inline syntree_nid
	nodeFirst(syntree_nid id) { return nodePtr(id)->value.container.first; }
	
	/**@brief Gibt den Folgeknoten eines Knotens zurück.
	 */
	static inline syntree_nid
	nodeNext(syntree_nid id) { return nodePtr(id)->next; }
}

%union {
	char* string;
	double floatValue;
	int intValue;
	
	symtab_symbol_t* symbol;
	syntree_nid node;
	syntree_node_type type;
}

%printer { fprintf(yyoutput, "\"%s\"", $$); } <string>
%printer { fprintf(yyoutput, "%g", $$); } <floatValue>
%printer { fprintf(yyoutput, "%i", $$); } <intValue>
%printer {
	if ($$ != 0)
	{
		putc('\n', yyoutput);
		syntreePrint(ast, $$, yyoutput, 1);
	}
} <node>
%printer {
	putc('\n', yyoutput);
	symtabPrint(tab, yyoutput);
} declassignment functiondefinition

%destructor { free($$); } <string>

/* used tokens (KW is abbreviation for keyword) */
%token AND
%token OR
%token EQ
%token NEQ
%token LEQ
%token GEQ
%token LSS
%token GRT
%token KW_BOOLEAN
%token KW_DO
%token KW_ELSE
%token KW_FLOAT
%token KW_FOR
%token KW_IF
%token KW_INT
%token KW_PRINTF
%token KW_RETURN
%token KW_VOID
%token KW_WHILE
%token <intValue>   CONST_INT
%token <floatValue> CONST_FLOAT
%token <intValue>   CONST_BOOLEAN
%token <string>     CONST_STRING
%token <string>     ID

/* definition of association and precedence of operators */
%left '+' '-' OR
%left '*' '/' AND
%nonassoc UMINUS

/* workaround for handling dangling else */
/* LOWER_THAN_ELSE stands for a not existing else */
%nonassoc LOWER_THAN_ELSE
%nonassoc KW_ELSE

%type <node> program declassignment
%type <node> opt_argumentlist argumentlist
%type <node> statementlist statement block body
%type <node> ifstatement forstatement dowhilestatement whilestatement opt_else
%type <node> returnstatement printf statassignment
%type <node> expr simpexpr functioncall assignment

%type <type> type
%type <symbol> parameter

%%

start:
	program {
		symtab_symbol_t* entry = symtabLookup(tab, "main");
	
		if (entry == NULL)
			yyerror("void main() doesn't exist");
	
		if (!entry->is_function)
			yyerror("main() isn't a function");
	
		if (entry->type != SYNTREE_TYPE_Void)
			yyerror("main() cannot have a return value");
	
		if (entry->par_next != NULL)
			yyerror("main() cannot have parameters");
	
		nodeValue(0)->program.body = syntreeNodeAppend(ast, $program, entry->body);
		nodeValue(0)->program.globals = symtabMaxGlobals(tab);
	}
	;

/* see EBNF grammar for further information */
program:
	/* empty */
		{ $$ = syntreeNodeEmpty(ast, SYNTREE_TAG_Sequence); }
	| program[prog] declassignment[decl] ';'
		{ syntreeNodeAppend(ast, $prog, $decl); }
	| program functiondefinition
	;

functiondefinition:
	type ID[name] {
		func = symtabSymbol($name, $type);
		func->is_function = 1;
		func->body = syntreeNodeEmpty(ast, SYNTREE_TAG_Function);
	
		if (symtabInsert(tab, func))
			yyerror("double declaration of function '%s'", $name);
	
		symtabEnter(tab);
	}
	'(' opt_parameterlist ')' '{' statementlist[body] '}' {
		syntreeNodeAppend(ast, func->body, $body);
		symtabLeave(tab);
		nodeValue(func->body)->function.locals = symtabMaxLocals(tab);
	}
	;

opt_parameterlist:
	/* empty */
	| parameterlist
	;

parameterlist:
	parameter
		{ symtabParam(func, $parameter); }
	| parameter ',' parameterlist
		{ symtabParam(func, $parameter); }
	;

parameter:
	type ID[name] {
		$$ = symtabSymbol($name, $type);
	
		if (symtabInsert(tab, $$))
			yyerror("double declaration of parameter '%s'", $name);
	}
	;

functioncall:
	ID[name] '(' opt_argumentlist[args] ')' {
		symtab_symbol_t* fn = symtabLookup(tab, $name);
		symtab_symbol_t* par;
		syntree_nid arg;
	
		if (!fn)
			yyerror("unknown symbol '%s'", $name);
	
		if (!fn->is_function)
			yyerror("'%s' cannot be called", $name);
	
		/* match the argument types with the formal parameters */
		for (par = symtabParamFirst(fn), arg = nodeFirst($args);
		     par != NULL && arg != 0;
		     par = symtabParamNext(par), arg = nodeNext(arg))
		{
			if (par->type != nodeType(arg))
				yyerror("argument of type '%s' doesn't exactly match "
				        "parameter of type '%s' in call to '%s()'",
				        nodeTypeName[nodeType(arg)],
				        nodeTypeName[par->type], $name);
		}
	
		if (par != NULL)
			yyerror("more arguments expected in call to '%s()'", $name);
	
		if (arg != 0)
			yyerror("too many arguments in call to '%s()'", $name);
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_Call, $args, fn->body);
		nodePtr($$)->type = fn->type;
	}
	;

opt_argumentlist:
	/* empty */
		{ $$ = syntreeNodeEmpty(ast, SYNTREE_TAG_Sequence); }
	| argumentlist
	;

argumentlist:
	assignment[expr]
		{ $$ = syntreeNodeTag(ast, SYNTREE_TAG_Sequence, $expr); }
	| argumentlist[list] ',' assignment[elem]
		{ $$ = syntreeNodeAppend(ast, $list, $elem); }
	;

statementlist:
	/* empty */
		{ $$ = syntreeNodeEmpty(ast, SYNTREE_TAG_Sequence); }
	| statementlist[list] statement[elem]
		{ $$ = syntreeNodeAppend(ast, $list, $elem); }
	;

block:
	'{' { symtabEnter(tab); }
		statementlist[body]
	'}' { symtabLeave(tab); $$ = $body; }
	;

body:
	{ symtabEnter(tab); } statement { symtabLeave(tab); $$ = $statement; }
	;

statement:
	  ifstatement
	| forstatement
	| whilestatement
	| returnstatement ';'
	| dowhilestatement ';'
	| printf ';'
	| declassignment ';'
	| statassignment ';'
	| functioncall ';'
	| block
	;

ifstatement:
	KW_IF '(' assignment[cond] ')' body[then] opt_else[else] {
		if (nodeType($cond) != SYNTREE_TYPE_Boolean)
			yyerror("condition needs to be boolean");
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_If, $cond, $then);
		$$ = syntreeNodeAppend(ast, $$, $else);
	}
	;

/* KW_ELSE has higher precedence, so an occuring 'else' will cause the */
/* execution of the second rule */
opt_else:
	/* empty */ %prec LOWER_THAN_ELSE
		{ $$ = 0; }
	| KW_ELSE body[else]
		{ $$ = $else; }
	;

forstatement:
	KW_FOR '(' { symtabEnter(tab); } declassignment[init] ';' expr[cond] ';' statassignment[step] ')' body {
		if (nodeType($cond) != SYNTREE_TYPE_Boolean)
			yyerror("condition needs to be boolean");
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_For, $init, $cond);
		$$ = syntreeNodeAppend(ast, $$, $step);
		$$ = syntreeNodeAppend(ast, $$, $body);
		symtabLeave(tab);
	}
	| KW_FOR '(' statassignment[init] ';' expr[cond] ';' statassignment[step] ')' body {
		if (nodeType($cond) != SYNTREE_TYPE_Boolean)
			yyerror("condition needs to be boolean");
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_For, $init, $cond);
		$$ = syntreeNodeAppend(ast, $$, $step);
		$$ = syntreeNodeAppend(ast, $$, $body);
	}
	;

dowhilestatement:
	KW_DO body KW_WHILE '(' assignment[cond] ')' {
		if (nodeType($cond) != SYNTREE_TYPE_Boolean)
			yyerror("condition needs to be boolean");
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_DoWhile, $cond, $body);
	}
	;

whilestatement:
	KW_WHILE '(' assignment[cond] ')' body {
		if (nodeType($cond) != SYNTREE_TYPE_Boolean)
			yyerror("condition needs to be boolean");
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_While, $cond, $body);
	}
	;

returnstatement:
	KW_RETURN {
		if (func->type != SYNTREE_TYPE_Void)
			yyerror("return value of type '%s' expected",
			       nodeTypeName[func->type]);
	
		$$ = syntreeNodeEmpty(ast, SYNTREE_TAG_Return);
	}
	| KW_RETURN assignment[expr] {
		if (!matchTypes(func->type, nodeType($expr)))
			yyerror("return with incompatible return type (%s -> %s)",
			        nodeTypeName[func->type],
			        nodeTypeName[nodeType($expr)]);
	
		if (func->type != nodeType($expr))
			$expr = syntreeNodeCast(ast, func->type, $expr);
	
		$$ = syntreeNodeTag(ast, SYNTREE_TAG_Return, $expr);
	}
	;

printf:
	KW_PRINTF '(' assignment[arg] ')'
		{ $$ = syntreeNodeTag(ast, SYNTREE_TAG_Print, $arg); }
	| KW_PRINTF '(' CONST_STRING[arg] ')'
		{ $$ = syntreeNodeTag(ast, SYNTREE_TAG_Print, syntreeNodeString(ast, $arg)); }
	;

declassignment:
	type ID[name] {
		if (symtabInsert(tab, symtabSymbol($name, $type)))
			yyerror("double declaration of %s", $name);
		$$ = 0;
	}
	| type ID[name] '=' assignment[expr] {
		symtab_symbol_t* sym = symtabSymbol($name, $type);
	
		if (symtabInsert(tab, sym))
			yyerror("double declaration of symbol '%s'", $name);
	
		if (!matchTypes(sym->type, nodeType($expr)))
			yyerror("cannot assign '%s' to '%s'",
			        nodeTypeName[nodeType($expr)],
				  nodeTypeName[sym->type]);
	
		 if (sym->type != nodeType($expr))
		 	$expr = syntreeNodeCast(ast, sym->type, $expr);
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_Assign,
		                     syntreeNodeVariable(ast, sym), $expr);
	}
	;

type:
	KW_BOOLEAN { $$ = SYNTREE_TYPE_Boolean; }
	| KW_FLOAT { $$ = SYNTREE_TYPE_Float; }
	| KW_INT   { $$ = SYNTREE_TYPE_Integer; }
	| KW_VOID  { $$ = SYNTREE_TYPE_Void; }
	;

statassignment:
	ID[name] '=' assignment[expr] {
		symtab_symbol_t* sym = symtabLookup(tab, $name);
	
		if (sym == NULL)
			yyerror("undeclared symbol '%s'", $name);
	
		if (sym->is_function)
			yyerror("cannot assign value to function '%s'", $name);
	
		if (!matchTypes(sym->type, nodeType($expr)))
			yyerror("cannot assign '%s' to '%s'",
			        nodeTypeName[nodeType($expr)],
				  nodeTypeName[sym->type]);
	
		if (sym->type != nodeType($expr))
			$expr = syntreeNodeCast(ast, sym->type, $expr);
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_Assign,
		                     syntreeNodeVariable(ast, sym), $expr);
	}
	;

assignment:
	ID[name] '=' assignment[expr] {
		symtab_symbol_t* sym = symtabLookup(tab, $name);
	
		if (sym == NULL)
			yyerror("undeclared symbol '%s'", $name);
	
		if (sym->is_function)
			yyerror("cannot assign value to function '%s'", $name);
	
		if (!matchTypes(sym->type, nodeType($expr)))
			yyerror("cannot assign '%s' to '%s'",
			        nodeTypeName[nodeType($expr)],
				  nodeTypeName[sym->type]);
	
		if (sym->type != nodeType($expr))
			$expr = syntreeNodeCast(ast, sym->type, $expr);
	
		$$ = syntreeNodePair(ast, SYNTREE_TAG_Assign,
		                     syntreeNodeVariable(ast, sym), $expr);
		nodePtr($$)->type = sym->type;
	}
	| expr
	;

expr:
	simpexpr
	| simpexpr[lhs] EQ  simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Eqt); }
	| simpexpr[lhs] NEQ simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Neq); }
	| simpexpr[lhs] LEQ simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Leq); }
	| simpexpr[lhs] GEQ simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Geq); }
	| simpexpr[lhs] LSS simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Lst); }
	| simpexpr[lhs] GRT simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Grt); }
	;

simpexpr:
	simpexpr[lhs] '+' simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Plus); }
	| simpexpr[lhs] '-' simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Minus); }
	| simpexpr[lhs] OR simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_LogOr); }
	| simpexpr[lhs] '*' simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Times); }
	| simpexpr[lhs] '/' simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_Divide); }
	| simpexpr[lhs] AND simpexpr[rhs]
		{ $$ = combine($lhs, $rhs, SYNTREE_TAG_LogAnd); }
	| '-' simpexpr[operand] %prec UMINUS {
		if (nodeType($operand) != SYNTREE_TYPE_Integer
		 && nodeType($operand) != SYNTREE_TYPE_Float)
			yyerror("cannot apply unary minus to values of type '%s'",
			        nodeTypeName[nodeType($operand)]);
	
		$$ = syntreeNodeTag(ast, SYNTREE_TAG_Uminus, $operand);
		nodePtr($$)->type = nodeType($operand);
	}
	| CONST_INT[val]
		{ $$ = syntreeNodeInteger(ast, $val); }
	| CONST_FLOAT[val]
		{ $$ = syntreeNodeFloat(ast, $val); }
	| CONST_BOOLEAN[val]
		{ $$ = syntreeNodeBoolean(ast, $val); }
	| functioncall
	| ID[name] {
		symtab_symbol_t* sym = symtabLookup(tab, $name);
	
		if (sym == NULL)
			yyerror("undeclared symbol '%s'", $name);
	
		if (sym->is_function)
			yyerror("cannot read value of function '%s'", $name);
	
		$$ = syntreeNodeVariable(ast, sym);
	}
	| '(' assignment ')'
		{ $$ = $assignment; }
	;

%%

/**@brief Gibt eine Fehlermeldung aus und beendet das Programm mit Exitcode -1.
 * Die Funktion akzeptiert eine variable Argumentliste und nutzt die Syntax von
 * printf.
 * @param msg  die Fehlermeldung
 * @param ...  variable Argumentliste für die Formatierung von \p msg
 */
void yyerror(const char* msg, ...)
{
	va_list args;
	
	va_start(args, msg);
	fprintf(stderr, "Error in line %d: ", yylineno);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	va_end(args);
	
	exit(-1);
}

int
matchTypes(syntree_node_type lhs, syntree_node_type rhs)
{
	return (lhs == rhs
	    || (lhs == SYNTREE_TYPE_Float && rhs == SYNTREE_TYPE_Integer));
}

/**@brief Testet, ob eine binäre Operation zwischen zwei getypten Ausdrücken
 * definiert ist und bricht mit einem Fehler ab, falls nicht.
 * @param lhs  Typ des Operanden auf der linken Seite
 * @param rhs  Typ des Operanden auf der rechten Seite
 * @param op   Operator
 * @return resultierender Typ aus der Operation
 */	
static syntree_node_type
combineTypes(syntree_node_type lhs, syntree_node_type rhs, syntree_node_tag op)
{
	/* no operation allows void-arguments */
	if (lhs == SYNTREE_TYPE_Void || rhs == SYNTREE_TYPE_Void)
		yyerror("void not allowed in arithmetical/logical operation");
	
	switch (op)
	{
	case SYNTREE_TAG_Eqt:
	case SYNTREE_TAG_Neq:
		if (lhs == SYNTREE_TYPE_Boolean && rhs == SYNTREE_TYPE_Boolean)
			return SYNTREE_TYPE_Boolean;
		
		/* fall through */
	case SYNTREE_TAG_Leq:
	case SYNTREE_TAG_Geq:
	case SYNTREE_TAG_Lst:
	case SYNTREE_TAG_Grt:
		switch (lhs)
		{
		case SYNTREE_TYPE_Float:
		case SYNTREE_TYPE_Integer:
			switch (rhs)
			{
			case SYNTREE_TYPE_Float:
			case SYNTREE_TYPE_Integer:
				return SYNTREE_TYPE_Boolean;
				
			default:
				break;
			}
			
			/* fall through */
		default:
			yyerror("'%s' cannot be compared with '%s'",
			        nodeTypeName[lhs], nodeTypeName[rhs]);
		}
		
		break;
		
	case SYNTREE_TAG_LogOr:
	case SYNTREE_TAG_LogAnd:
		if (lhs != SYNTREE_TYPE_Boolean || rhs != SYNTREE_TYPE_Boolean)
			 yyerror("boolean operands expected for logical operation");
		
		return SYNTREE_TYPE_Boolean;
		
	case SYNTREE_TAG_Plus:
	case SYNTREE_TAG_Minus:
	case SYNTREE_TAG_Times:
	case SYNTREE_TAG_Divide:
		if (lhs != SYNTREE_TYPE_Boolean && rhs != SYNTREE_TYPE_Boolean)
		{
			if (lhs == SYNTREE_TYPE_Float || rhs == SYNTREE_TYPE_Float)
				 return SYNTREE_TYPE_Float;
			
			return SYNTREE_TYPE_Integer;
		}
		
		yyerror("'%s' and '%s' are not allowed in arithmetical operation",
		        nodeTypeName[lhs], nodeTypeName[rhs]);
			 
	default:
	 	yyerror("unknown operation (internal error)");
	}
	
	/* just to avoid a warning */
	return 0;
}

syntree_nid
combine(syntree_nid lhs, syntree_nid rhs, syntree_node_tag op)
{
	syntree_nid res;
	syntree_node_type lhs_type = nodeType(lhs);
	syntree_node_type rhs_type = nodeType(rhs);
	syntree_node_type type = combineTypes(lhs_type, rhs_type, op);
	
	if (lhs_type != rhs_type)
	{
		/* Situation: ein Integer und ein Float (impliziter Cast) */
		if (lhs_type != SYNTREE_TYPE_Float)
			lhs = syntreeNodeCast(ast, SYNTREE_TYPE_Float, lhs);
		else if (rhs_type != SYNTREE_TYPE_Float)
			rhs = syntreeNodeCast(ast, SYNTREE_TYPE_Float, rhs);
	}
	
	res = syntreeNodePair(ast, op, lhs, rhs);
	nodePtr(res)->type = type;
	return res;
}