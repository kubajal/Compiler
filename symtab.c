/***************************************************************************//**
 * @file symtab.c
 * @author Dorian Weber
 * @brief Implementation der Symboltabelle.
 ******************************************************************************/

#include "symtab.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************** private functions */

/**@internal
 * @brief Gibt den Speicher eines Symtab-Symbols frei.
 * @param sym  das freizugebende Symbol
 */
static void
symtabSymbolFree(symtab_symbol_t* sym)
{
	/* gib die Parameter der Funktion frei */
	if (sym->is_function)
	{
		symtab_symbol_t *curr, *next = sym->par_next;
		
		while (next != NULL)
		{
			curr = next;
			next = next->par_next;
			symtabSymbolFree(curr);
		}
	}
	
	/* gib Funktionsnamen und Symbol frei */
	free(sym->name);
	free(sym);
}

/**@internal
 * @brief Gibt zurück, ob Symbole gerade in die globale Schicht eingefügt werden.
 * @param tab  die Symboltabelle
 * @return 1 falls ja, 0 ansonsten
 */
static inline int
symtabIsGlobal(const symtab_t* tab)
{
	return stackCount(tab->block) == 1;
}

/* ********************************************************* public functions */

int
symtabInit(symtab_t* self)
{
	if (stackInit(self->decl))
		goto err0;
	
	if (stackInit(self->block))
		goto err1;
	
	if (dictInit(&self->map))
		goto err2;
	
	/* initialisiere mit einem globalen Block */
	stackPush(self->block) = 0;
	return 0;
	
err2:	stackRelease(self->block);
err1:	stackRelease(self->decl);
err0:	return -1;
}

void
symtabRelease(symtab_t* self)
{
	/* entferne alle lokalen Variablen */
	while (!stackIsEmpty(self->decl))
	{
		symtab_symbol_t* sym = stackPop(self->decl);
		
		if (!sym->is_param)
			symtabSymbolFree(sym);
	}
	
	/* entferne die Stacks und das Wörterbuch */
	stackRelease(self->decl);
	stackRelease(self->block);
	dictRelease(&self->map);
}

void
symtabEnter(symtab_t* self)
{
	if (symtabIsGlobal(self))
		self->maxpos = 0;
	
	stackPush(self->block) = 0;
}

void
symtabLeave(symtab_t* self)
{
	symtab_symbol_t* sym;
	unsigned int count;
	
	for (count = stackPop(self->block); count > 0; --count)
	{
		/* entferne das nächste Symbol vom Stack */
		sym = stackPop(self->decl);
		
		/* stelle den vorigen Zustand wieder her */
		if (sym->rec_prev == NULL)
			dictDel(&self->map, sym->name);
		else
			dictSet(&self->map, sym->name, sym->rec_prev);
		
		/* gib das Symbol frei, falls es kein Parameter ist */
		if (!sym->is_param)
			symtabSymbolFree(sym);
	}
	
	assert(!stackIsEmpty(self->block));
}

int
symtabInsert(symtab_t* self, symtab_symbol_t* sym)
{
	unsigned int i;
	
	/* berechne Kenngrößen */
	sym->rec_prev = dictSet(&self->map, sym->name, sym);
	sym->is_global = symtabIsGlobal(self);
	sym->id = stackCount(self->decl);
	sym->pos = sym->id;
	
	/* berechne die Position im Stack (für die Interpretation) */
	if (!sym->is_global)
	{
		sym->pos -= self->block[0];
		
		if (sym->pos > self->maxpos)
			self->maxpos = sym->pos;
	}
	else for (i = 0; i < stackCount(self->decl); ++i)
		if (self->decl[i]->is_function)
			--sym->pos;
	
	/* teste, ob schon eine Deklaration im gleichen Block vorliegt */
	if (sym->rec_prev != NULL
	 && sym->rec_prev->id >= sym->id - stackTop(self->block))
	{
		/* stelle die Originaldefinition wieder her */
		dictSet(&self->map, sym->name, sym->rec_prev);
		
		/* gib das Symbol frei */
		symtabSymbolFree(sym);
		
		/* Rückgabe mit Fehlercode */
		return -1;
	}
	
	/* merke dir die Variable */
	stackPush(self->decl) = sym;
	++stackTop(self->block);
	
	/* keine Fehler aufgetreten */
	return 0;
}

void
symtabParam(symtab_symbol_t* func, symtab_symbol_t* sym)
{
	/* markiere das Symbol als Parameter und hänge es an den Anfang */
	sym->is_param = 1;
	sym->par_next = func->par_next;
	func->par_next = sym;
}

symtab_symbol_t*
symtabLookup(const symtab_t* self, const char* id)
{
	return dictGet(&self->map, id);
}

symtab_symbol_t*
symtabSymbol(const char* name, syntree_node_type type)
{
	symtab_symbol_t* sym = calloc(1, sizeof(*sym));
	size_t size;
	
	if (sym == NULL)
		goto err0;
	
	size = strlen(name) + 1;
	sym->name = malloc(size);
	
	if (sym->name == NULL)
		goto err1;
	
	memcpy(sym->name, name, size);
	sym->type = type;
	return sym;
	
err1:	free(sym);
err0:	fputs("out-of-memory error\n", stderr);
	exit(-1);
}

symtab_symbol_t*
symtabParamFirst(const symtab_symbol_t* func)
{
	assert(func->is_function);
	return func->par_next;
}

symtab_symbol_t*
symtabParamNext(const symtab_symbol_t* param)
{
	assert(param->is_param);
	return param->par_next;
}

unsigned int
symtabMaxLocals(const symtab_t* self)
{
	return self->maxpos + 1;
}

unsigned int
symtabMaxGlobals(const symtab_t* self)
{
	unsigned int size, i;
	
	for (size = i = 0; i < self->block[0]; ++i)
		if (!self->decl[i]->is_function)
			++size;
	
	return size;
}

void
symtabPrint(const symtab_t* self, FILE* out)
{
	symtab_symbol_t** it;
	symtab_symbol_t* sym;
	unsigned int blockId;
	unsigned int symId;
	
	/* generelle Statistiken der Symboltabelle */
	fprintf(out,
	        "+---------------------\n"
	        "|Symbol count: %u\n"
	        "|Block count:  %u\n",
	        stackCount(self->decl), stackCount(self->block));
	
	/* Tabelleninhalt */
	it = self->decl;
	for (blockId = 0; blockId < stackCount(self->block); ++blockId)
	{
		fprintf(out, "|--------< %u >--------\n", blockId);
		
		for (symId = self->block[blockId]; symId > 0; --symId, ++it)
		{
			/* wähle das nächste Symbol */
			sym = *it;
			
			/* nur zur Sicherheit */
			if (sym == NULL)
			{
				fprintf(out, "|NULL");
				continue;
			}
			
			/* Symbol */
			fprintf(out, "|(%u) %s %s", sym->id,
			        nodeTypeName[sym->type], sym->name);
			
			/* Funktionsparameter */
			if (sym->is_function)
			{
				symtab_symbol_t* param;
				const char* sep = "";
				
				/* geklammerte, kommaseparierte Liste;
				 * (sep ist nur in der ersten Iteration leer) */
				putc('(', out);
				for (param = sym->par_next; param != NULL;
				     param = param->par_next, sep = ", ")
				{
					fprintf(out, "%s%s %s:[%u]", sep,
					        nodeTypeName[param->type],
					        param->name, param->pos);
				}
				putc(')', out);
			}
			else
				fprintf(out, ":[%u]", sym->pos);
			
			/* überdeckte Definition des Symbols */
			if (sym->rec_prev)
				fprintf(out, " {shadows %u}", sym->rec_prev->id);
			
			putc('\n', out);
		}
	}
	
	fputs("+---------------------\n", out);
}
