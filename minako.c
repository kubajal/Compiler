/***************************************************************************//**
 * @file minako.c
 * @author Dorian Weber und die Studenten
 * @brief Enthält den Interpreter sowie den Einstiegspunkt in das Programm.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "minako-syntax.tab.h"
#include "symtab.h"
#include "syntree.h"

/**@brief Maximale Anzahl gleichzeitig verwendeter Variablen im Interpreter.
 */
#define MINAKO_STACK_SIZE 1024
#define DUMMY -1 // leerer Platz

/* ******************************************************* private structures */

/**@brief Ein Variablenwert im Interpreter.
 */
typedef struct minako_value_s
{
	/**@brief Typ des Variablenwertes.
	 */
	syntree_node_type type;

	/**@brief Variablenwert.
	 */
	union
	{
		int boolean;  /**<@brief Boolescher Wert. */
		int integer;  /**<@brief Ganzzahliger Wert. */
		float real;   /**<@brief Fließkommawert. */
		char* string; /**<@brief Zeiger auf Zeichenkette. */
	} value;
} minako_value_t;

/**@brief Struktur des Laufzeitzustands des Interpreters.
 */
typedef struct minako_vm_s
{
	minako_value_t stack[MINAKO_STACK_SIZE]; /**<@brief Variablenstack. */
	minako_value_t eax;  /**<@brief Ausgaberegister. */
	minako_value_t* ebp; /**<@brief Base pointer. */
	minako_value_t* esp; /**<@brief Stack pointer. */
	int returnFlag; /**<@brief Signalisiert das Verlassen einer Funktion. */
} minako_vm_t;

/**@brief Prototyp von Funktionen, die einen Knoten interpretieren.
 * @note Der Zustand der virtuellen Maschine und der ausgeführte Syntaxbaum
 * werden der Einfachheit halber implizit als globale Variablen bereitgestellt.
 * @param node  der zu interpretierende Knoten
 */
typedef void minako_exec_p(const syntree_node_t* node);

/**@brief Funktionszeigertyp der Interpreter-Funktionen.
 */
typedef minako_exec_p* minako_exec_f;

/* ****************************************************************** globals */

/* Deklaration aller Interpreterfunktionen */
#define DECL(NODE) \
	static minako_exec_p exec ## NODE;
	SYNTREE_NODE_LIST(DECL)
#undef DECL

/**@brief Globaler Zeiger auf den aktuellen Zustand der virtuellen Maschine.
 */
static minako_vm_t* vm;

#define CALLBACK(NODE) \
	&exec ## NODE,

/**@brief Statische Dispatchtabelle zur Lokalisierung der richtigen
 * Interpreterfunktion für einen gegebenen Knotentyp im Syntaxbaum.
 */
static const minako_exec_f
dispatchTable[] = {
	SYNTREE_NODE_LIST(CALLBACK)
};

#undef CALLBACK

/* ******************************************************** private functions */

/* Trace-Unterstützung zum Debuggen des Interpreters */

#ifndef NDEBUG
	static unsigned int indent;

	/**@brief Schreibt den Namen eines Knotentags eingerückt in die
	 * Standardausgabe und erhöht die Einrückung.
	 */
	#define TRACE_ENTER(TAG) \
		if (yydebug) { \
			printf("%*s<%s>\n", indent*4, "", nodeTagName[TAG]); \
			++indent; \
		}

     	/**@brief Verringert die Einrückung und schreibt den Namen eines
	 * Knotentags eingerückt in die Standardausgabe.
     	 */
	#define TRACE_LEAVE(TAG) \
		if (yydebug) { \
			--indent; \
			printf("%*s</%s>\n", indent*4, "", nodeTagName[TAG]); \
		}

     	/**@brief Schreibt den Wert einer Stackvariablen eingerückt in die Ausgabe.
     	 */
	#define TRACE_VALUE(VAL) \
		if (yydebug) { \
			printf("%*s", indent*4, ""); \
			switch (VAL.type) { \
			case SYNTREE_TYPE_Boolean: \
				printf("%s", VAL.value.boolean ? "true" : "false"); \
				break; \
			case SYNTREE_TYPE_Integer: \
				printf("%i", VAL.value.integer); \
				break; \
			case SYNTREE_TYPE_Float: \
				printf("%g", VAL.value.real); \
				break; \
			case SYNTREE_TYPE_String: \
				printf("\"%s\"", VAL.value.string); \
				break; \
			case SYNTREE_TYPE_Void: \
				printf("(void)"); \
				break; \
			} \
			putc('\n', stdout); \
		}
#else
	#define TRACE_ENTER(TAG)
	#define TRACE_LEAVE(TAG)
	#define TRACE_VALUE(VAL)
#endif

/* Hilfsfunktionen */

/**@brief Gibt den ersten Kindknoten eines Containers zurück.
 */
static inline const syntree_node_t*
nodeFirst(const syntree_node_t* node)
{
	return syntreeNodePtr(ast, node->value.container.first);
}

/**@brief Gibt den letzten Kindknoten eines Containers zurück.
 */
static inline const syntree_node_t*
nodeLast(const syntree_node_t* node)
{
	return syntreeNodePtr(ast, node->value.container.last);
}

/**@brief Gibt den Folgeknoten eines Knotens zurück.
 */
static inline const syntree_node_t*
nodeNext(const syntree_node_t* node)
{
	return syntreeNodePtr(ast, node->next);
}

/**@brief Prüft ob der gegebene Knoten der Terminatorknoten ist.
 * Terminatorknoten terminieren Container, analog zum terminierenden 0-Byte für
 * C-Strings.
 */
static inline int
nodeSentinel(const syntree_node_t* node)
{
	return syntreeNodeId(ast, node) == 0;
}

/* Dispatcher */

/**@brief Ruft für einen gegebenen Knoten die entsprechende Ausführungsfunktion.
 */
static inline minako_value_t
dispatch(const syntree_node_t* node)
{
	/* rufe die dem Knotentyp entsprechende Funktion */
	TRACE_ENTER(node->tag);
	//printf("dispatching Tag: %s, Type: %s\n", nodeTagName[node->tag], nodeTypeName[node->type]);
	dispatchTable[node->tag](node);
	TRACE_LEAVE(node->tag);

	return vm->eax;
}

/* ********************************* */
/* Literale */
/* ********************************* */

static void
execInteger(const syntree_node_t* node)
{
	vm->eax.type = node->type;
	vm->eax.value.integer = node->value.integer;
	TRACE_VALUE(vm->eax);
}

static void
execFloat(const syntree_node_t* node)
{
	vm->eax.type = node->type;
	vm->eax.value.real = node->value.real;
	TRACE_VALUE(vm->eax);
}

static void
execBoolean(const syntree_node_t* node)
{
	vm->eax.type = node->type;
	vm->eax.value.boolean = node->value.boolean;
	TRACE_VALUE(vm->eax);
}

static void
execString(const syntree_node_t* node)
{
	vm->eax.type = node->type;
	vm->eax.value.string = node->value.string;
	TRACE_VALUE(vm->eax);
}

static void
execLocVar(const syntree_node_t* node)
{
	vm->eax = vm->ebp[node->value.variable];
}

static void
execGlobVar(const syntree_node_t* node)
{
	vm->eax = vm->stack[node->value.variable];
}

/* ********************************* */
/* Anweisungen */
/* ********************************* */

static void
execProgram(const syntree_node_t* node)
{
	/* prepare the VM for execution */
	vm->returnFlag = 0;
	vm->ebp = vm->esp = vm->stack;

	for(unsigned int i = 0; i < MINAKO_STACK_SIZE; i++)
	{
        vm->stack[i].type = SYNTREE_TYPE_Void;
        vm->stack[i].value.integer = DUMMY; // -1
	}

	/* allocate space for global variables */
	vm->esp += node->value.program.globals;

	/* protect from stack overflow */
	if (vm->esp >= vm->stack + MINAKO_STACK_SIZE)
	{
		fprintf(stderr, "stack overflow\n");
		exit(-1);
	}

	execSequence(node);
}

static void
execFunction(const syntree_node_t* node)
{

    unsigned int locals = node->value.function.locals;
	vm->ebp = vm->esp;
	vm->esp += locals;

	execSequence(nodeFirst(node));

	vm->returnFlag = 0;
}

static void
execCall(const syntree_node_t* node)
{
	syntree_node_t *func = nodeLast(node);
	if(vm->esp + func->value.function.locals - vm->stack >= MINAKO_STACK_SIZE)
    {
        fputs("stack overflow\n", stderr);
        exit(-1);
    }

    unsigned int locals = func->value.function.locals;

    minako_value_t *params = vm->esp;
    vm->esp += locals;
    minako_value_t *old_ebp = vm->ebp;

	unsigned int i = 0;
	syntree_node_t *sequence = nodeFirst(node); // Sequence von Argumenten
	syntree_node_t *argument = nodeFirst(sequence);   // Argumenten

	while(!nodeSentinel(argument))
    {
        params[i] = dispatch(argument);
        vm->esp = vm->esp + 1;
        i++;
		argument = nodeNext(argument);
	}

    vm->esp = params;

	dispatch(func);

	for(unsigned int i = 0; i < locals; i++)
    {
        params[i].type = SYNTREE_TYPE_Void;
        params[i].value.integer = DUMMY;    // -1
    }
	vm->esp = vm->ebp;
	vm->ebp = old_ebp;
}

static void
execSequence(const syntree_node_t* node)
{

	syntree_node_t *ptr = nodeFirst(node);

	while(!nodeSentinel(ptr)){
		if(vm->returnFlag == 1){
            break;
		}
		dispatch(ptr);
		ptr = nodeNext(ptr);
	}
}

static void
execIf(const syntree_node_t* node)
{
	const syntree_node_t* test = nodeFirst(node);
	const syntree_node_t* cons = nodeNext(test);
	const syntree_node_t* opt_else = nodeNext(cons);

	/* test if we need to select the else block */
	dispatch(test);
	if (vm->eax.value.boolean)
    {
		dispatch(cons);
    }
	else
    {
        if(!nodeSentinel(opt_else))
            dispatch(opt_else);
    }
}

static void
execDoWhile(const syntree_node_t* node)
{
	const syntree_node_t* cond = nodeFirst(node);
	const syntree_node_t* exec = nodeLast(node);

	do
	{
		dispatch(exec);

		if (vm->returnFlag)
			break;
	}
	while (dispatch(cond).value.boolean);
}

static void
execWhile(const syntree_node_t* node)
{
	const syntree_node_t* cond = nodeFirst(node);
	const syntree_node_t* body = nodeLast(node);

	do
	{
		dispatch(body);

		if (vm->returnFlag){
			break;
		}
	}
	while (dispatch(cond).value.boolean);
}

static void
execFor(const syntree_node_t* node)
{
    syntree_node_t *init = nodeFirst(node);
    syntree_node_t *cond = nodeNext(init);
    syntree_node_t *step = nodeNext(cond);
    syntree_node_t *body = nodeNext(step);

    dispatch(init);
    minako_value_t v_cond, v_step, v_body;
    while(1)
    {
        v_cond = dispatch(cond);
        if(v_cond.value.boolean == 0)
        {
            break;
        }
        v_body = dispatch(body);
        v_step = dispatch(step);
    }
}

static void
execPrint(const syntree_node_t* node)
{
	switch (dispatch(nodeFirst(node)).type)
	{
	case SYNTREE_TYPE_Boolean:
		fputs(vm->eax.value.boolean ? "true" : "false", stdout);
		break;

	case SYNTREE_TYPE_Integer:
		printf("%i", vm->eax.value.integer);
		break;

	case SYNTREE_TYPE_Float:
		printf("%g", vm->eax.value.real);
		break;

	case SYNTREE_TYPE_String:
		fputs(vm->eax.value.string, stdout);
		break;
	}

	putc('\n', stdout);
}

static void
execAssign(const syntree_node_t* node)
{
    syntree_node_t *var = nodeFirst(node);
    syntree_node_t *expr = nodeNext(var);
    dispatch(expr);
    unsigned int offset = var->value.variable;
    if(var->tag == SYNTREE_TAG_GlobVar)
        vm->stack[offset] = vm->eax;
    if(var->tag == SYNTREE_TAG_LocVar)
        vm->ebp[offset] = vm->eax;
}

static void
execReturn(const syntree_node_t* node)
{
	node = nodeFirst(node);

	if (!nodeSentinel(node))
		dispatch(node);

	vm->returnFlag = 1;
}

/* ********************************* */
/* Ausdrücke */
/* ********************************* */

static void
execCast(const syntree_node_t* node)
{
	dispatch(nodeFirst(node));

	switch (node->type)
	{
	case SYNTREE_TYPE_Float:
		switch (vm->eax.type)
		{
		case SYNTREE_TYPE_Integer:
			vm->eax.type = node->type;
			vm->eax.value.real = vm->eax.value.integer;
			break;

		default:
			assert(!"unexpected source type");
		}

		break;

	default:
		assert(!"unexpected target type");
	}
}

static void
execPlus(const syntree_node_t* node)
{

	minako_value_t lhs = dispatch(nodeFirst(node));
	minako_value_t rhs = dispatch(nodeLast(node));

	switch (node->type)
	{
	case SYNTREE_TYPE_Integer:
		vm->eax.value.integer = lhs.value.integer + rhs.value.integer;
		break;

	case SYNTREE_TYPE_Float:
		vm->eax.value.real = lhs.value.real + rhs.value.real;
		break;

	default:
		assert(!"unexpected type in operation");
	}
}

static void
execMinus(const syntree_node_t* node)
{
	minako_value_t lhs = dispatch(nodeFirst(node));
	minako_value_t rhs = dispatch(nodeLast(node));

	switch (node->type)
	{
	case SYNTREE_TYPE_Integer:
		vm->eax.value.integer = lhs.value.integer - rhs.value.integer;
		break;

	case SYNTREE_TYPE_Float:
		vm->eax.value.real = lhs.value.real - rhs.value.real;
		break;

	default:
		assert(!"unexpected type in operation");
	}
}

static void
execTimes(const syntree_node_t* node)
{
	minako_value_t lhs = dispatch(nodeFirst(node));
	minako_value_t rhs = dispatch(nodeLast(node));

	switch (node->type)
	{
	case SYNTREE_TYPE_Integer:
		vm->eax.value.integer = lhs.value.integer * rhs.value.integer;
		break;

	case SYNTREE_TYPE_Float:
		vm->eax.value.real = lhs.value.real * rhs.value.real;
		break;

	default:
		assert(!"unexpected type in operation");
	}
}

static void
execDivide(const syntree_node_t* node)
{
	minako_value_t lhs = dispatch(nodeFirst(node));
	minako_value_t rhs = dispatch(nodeLast(node));

	switch (node->type)
	{
	case SYNTREE_TYPE_Integer:
		vm->eax.value.integer = lhs.value.integer / rhs.value.integer;
		break;

	case SYNTREE_TYPE_Float:
		vm->eax.value.real = lhs.value.real / rhs.value.real;
		break;

	default:
		assert(!"unexpected type in operation");
	}
}

static void
execLogOr(const syntree_node_t* node)
{
	(void) (dispatch(nodeFirst(node)).value.boolean
	|| dispatch(nodeLast(node)).value.boolean);
}

static void
execLogAnd(const syntree_node_t* node)
{
	(void) (dispatch(nodeFirst(node)).value.boolean
	&& dispatch(nodeLast(node)).value.boolean);
}

static void
execUminus(const syntree_node_t* node)
{
    dispatch(nodeFirst(node));
    if(vm->eax.type == SYNTREE_TYPE_Integer)
        vm->eax.value.integer = -vm->eax.value.integer;
    if(vm->eax.type == SYNTREE_TYPE_Float)
        vm->eax.value.real = -vm->eax.value.real;
}

static void
execEqt(const syntree_node_t* node)
{
    syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Boolean:
		if(vlhs.value.boolean == vrhs.value.boolean)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer == vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real == vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

static void
execNeq(const syntree_node_t* node)
{
    syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Boolean:
		if(vlhs.value.boolean != vrhs.value.boolean)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer != vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real != vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

static void
execLeq(const syntree_node_t* node)
{
	syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer <= vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real <= vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

static void
execGeq(const syntree_node_t* node)
{
	syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer >= vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real >= vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

static void
execLst(const syntree_node_t* node)
{
	syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer < vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real < vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

static void
execGrt(const syntree_node_t* node)
{
	syntree_node_t *lhs = nodeFirst(node), *rhs = nodeLast(node);

    minako_value_t vlhs = dispatch(lhs);
    minako_value_t vrhs = dispatch(rhs);

	switch (vlhs.type)
	{
	case SYNTREE_TYPE_Integer:
		if(vlhs.value.integer > vrhs.value.integer)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	case SYNTREE_TYPE_Float:
		if(vlhs.value.real > vrhs.value.real)
            vm->eax.value.boolean = 1;
        else
            vm->eax.value.boolean = 0;
		break;

	default:
		assert(!"unexpected type in operation");
	}
	vm->eax.type = SYNTREE_TYPE_Boolean;
}

/* *************************************************************** driver *** */

int main(int argc, const char* argv[])
{
	symtab_t symtab;
	syntree_t syntree;
	minako_vm_t engine;
	int rc;

	/* belege die globalen Zeiger mit den lokalen Werten */
	tab = &symtab;
	ast = &syntree;
	vm = &engine;

	/* versuche die Datei aus der Kommandozeile zu öffnen
	 * oder lies aus der Standardeingabe */
	yyin = (argc != 2) ? stdin : fopen(argv[1], "r");

	if (yyin == NULL)
		yyerror("couldn't open file %s\n", argv[1]);

	/* initialisiere die Hilfsstrukturen */
	if (symtabInit(tab))
	{
		fputs("out-of-memory error\n", stderr);
		exit(-1);
	}

	if (syntreeInit(ast))
	{
		fputs("out-of-memory error\n", stderr);
		exit(-1);
	}

	/* parse das Programm */
	yydebug = 0;
	rc = yyparse();

	/* gib die Symboltabelle wieder frei */
	//symtabRelease(&symtab);

    /*syntreePrint(ast, 0, stdout, 1);
	printf("---------------\n");
    syntreePrint(ast, 2, stdout, 1);
	printf("---------------\n");*/

	/* führe den Syntaxbaum aus */
	if (rc == 0)
	{
		yydebug = 0;
		dispatch(syntreeNodePtr(ast, 0));
	}

	/* gib den Syntaxbaum wieder frei */
	syntreeRelease(&syntree);

	return rc;
}
