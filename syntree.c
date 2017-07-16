/***************************************************************************//**
 * @file syntree.c
 * @author Dorian Weber
 * @brief Implementation des Syntaxbaumes.
 ******************************************************************************/

#include "syntree.h"
#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ******************************************************** private functions */

/**@internal
 * @brief Macht Platz für einen neuen Knoten und gibt ihn zurück.
 * @param self  Syntaxbaum
 * @return ein frischer Knoten
 */
static syntree_node_t*
syntreeNodeAlloc(syntree_t* self)
{
	syntree_node_t* node;
	
	if (self->len == self->cap)
	{
		self->cap *= 2;
		node = realloc(self->nodes, sizeof(*self->nodes)*self->cap);
		
		if (node == NULL)
		{
			fputs("out-of-memory error\n", stderr);
			exit(-1);
		}
		
		self->nodes = node;
	}
	
	node = &self->nodes[self->len];
	node->next = 0;
	++self->len;
	return node;
}

/**@internal
 * @brief Gibt Auskunft, ob ein Knoten zu den atomaren Knoten gehört.
 * @param node  der Knoten
 * @return 1, falls ja,\n
 *         0, ansonsten
 */
static inline int
syntreeIsPrimitive(const syntree_node_t* node)
{
	switch (node->tag)
	{
	case SYNTREE_TAG_Integer:
	case SYNTREE_TAG_Float:
	case SYNTREE_TAG_Boolean:
	case SYNTREE_TAG_String:
	case SYNTREE_TAG_LocVar:
	case SYNTREE_TAG_GlobVar:
		return 1;
		
	default:
		return 0;
	}
}

/* ********************************************************* public functions */

/* constructor/destructor */

int
syntreeInit(syntree_t* self)
{
	/* erstelle die Baumstruktur */
	self->len = 0;
	self->cap = 8;
	self->nodes = malloc(sizeof(*self->nodes)*self->cap);
	
	if (self->nodes == NULL)
		return -1;
	
	syntreeNodeEmpty(self, SYNTREE_TAG_Program);
	return 0;
}

void
syntreeRelease(syntree_t* self)
{
	syntree_node_t *it, *end;
	
	/* gib den dynamisch allozierten Speicher aller Knoten frei */
	for (it = self->nodes, end = it + self->len; it < end; ++it)
	{
		if (it->tag == SYNTREE_TAG_String)
			free(it->value.string);
	}
	
	/* gib die Knoten selbst frei */
	free(self->nodes);
}

/* node construction */

syntree_node_t*
syntreeNodePtr(const syntree_t* self, syntree_nid id)
{
	return &self->nodes[id];
}

syntree_nid
syntreeNodeId(const syntree_t* self, const syntree_node_t* node)
{
	return node - self->nodes;
}

syntree_nid
syntreeNodeBoolean(syntree_t* self, int flag)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	node->tag = SYNTREE_TAG_Boolean;
	node->type = SYNTREE_TYPE_Boolean;
	node->value.boolean = flag;
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeInteger(syntree_t* self, int number)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	node->tag = SYNTREE_TAG_Integer;
	node->type = SYNTREE_TYPE_Integer;
	node->value.integer = number;
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeFloat(syntree_t* self, float number)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	node->tag = SYNTREE_TAG_Float;
	node->type = SYNTREE_TYPE_Float;
	node->value.real = number;
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeString(syntree_t* self, char* text)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	node->tag = SYNTREE_TAG_String;
	node->type = SYNTREE_TYPE_String;
	node->value.string = text;
	
	if (node->value.string == NULL)
	{
		fputs("out-of-memory error\n", stderr);
		exit(-1);
	}
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeVariable(syntree_t* self, const symtab_symbol_t* symbol)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	
	if (symbol != NULL)
	{
		node->tag = symbol->is_global ? SYNTREE_TAG_GlobVar : SYNTREE_TAG_LocVar;
		node->type = symbol->type;
		node->value.variable = symbol->pos;
	}
	else
	{
		node->tag = SYNTREE_TAG_GlobVar;
		node->type = SYNTREE_TYPE_Void;
		node->value.variable = 0;
	}
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeCast(syntree_t* self, syntree_node_type target, syntree_nid id)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	
	node->tag = SYNTREE_TAG_Cast;
	node->type = target;
	node->value.container.first = node->value.container.last = id;
	
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeEmpty(syntree_t* self, syntree_node_tag tag)
{
	return syntreeNodeTag(self, tag, 0);
}

syntree_nid
syntreeNodeTag(syntree_t* self, syntree_node_tag tag, syntree_nid id)
{
	syntree_node_t* node = syntreeNodeAlloc(self);
	
	node->tag = tag;
	node->type = SYNTREE_TYPE_Void;
	node->value.container.first = node->value.container.last = id;
	
	assert(!syntreeIsPrimitive(node));
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodePair(syntree_t* self, syntree_node_tag tag,
                syntree_nid id1, syntree_nid id2)
{
	syntree_node_t* node;
	
	/* ignoriere leere Knoten */
	if (id1 == 0)
		return syntreeNodeTag(self, tag, id2);
	
	if (id2 == 0)
		return syntreeNodeTag(self, tag, id1);
	
	node = syntreeNodeAlloc(self);
	node->tag = tag;
	node->type = SYNTREE_TYPE_Void;
	node->value.container.first = id1;
	node->value.container.last = id2;
	
	syntreeNodePtr(self, id1)->next = id2;
	
	assert(!syntreeIsPrimitive(node));
	return syntreeNodeId(self, node);
}

syntree_nid
syntreeNodeAppend(syntree_t* self, syntree_nid listId, syntree_nid elemId)
{
	syntree_node_t* list = syntreeNodePtr(self, listId);
	assert(!syntreeIsPrimitive(list));
	
	/* ignoriere leere Knoten */
	if (elemId == 0)
		return listId;
	
	/* teste, ob das Element das erste der Liste ist */
	if (list->value.container.first)
	{
		syntreeNodePtr(self, list->value.container.last)->next = elemId;
		list->value.container.last = elemId;
	}
	else
	{
		list->value.container.first = list->value.container.last = elemId;
	}
	
	return listId;
}

/* misc routines */

void
syntreePrint(const syntree_t* self, syntree_nid root, FILE* out, int indent)
{
	syntree_node_t* node = syntreeNodePtr(self, root);
	fprintf(out, "%*s", indent*4, "");
	
	switch (node->tag)
	{
	case SYNTREE_TAG_Integer:
		fprintf(out, "Integer %i\n", node->value.integer);
		break;
		
	case SYNTREE_TAG_Float:
		fprintf(out, "Float %g\n", node->value.real);
		break;
		
	case SYNTREE_TAG_Boolean:
		fputs((node->value.boolean ? "true\n" : "false\n"), out);
		break;
		
	case SYNTREE_TAG_String:
		fprintf(out, "\"%s\"\n", node->value.string);
		break;
		
	case SYNTREE_TAG_LocVar:
	case SYNTREE_TAG_GlobVar:
		fprintf(out, "%s %s [pos=%i]\n",
		        node->tag == SYNTREE_TAG_LocVar ? "Local" : "Global",
		        nodeTypeName[node->type], node->value.variable);
		break;
		
	case SYNTREE_TAG_Call:
		fprintf(out, "Call %s [nid=%i] {\n", nodeTypeName[node->type],
		        node->value.container.last);
		syntreePrint(self, node->value.container.first, out, indent+1);
		fprintf(out, "%*s}\n", indent*4, "");
		break;
		
	case SYNTREE_TAG_Program:
		fprintf(out, "Program [globals=%u] {\n",
		        node->value.program.globals);
		syntreePrint(self, node->value.program.body, out, indent+1);
		fprintf(out, "%*s}\n", indent*4, "");
		break;
		
	case SYNTREE_TAG_Function:
		fprintf(out, "%s Function [locals=%u] {\n",
		        nodeTypeName[node->type], node->value.function.locals);
		syntreePrint(self, node->value.function.body, out, indent+1);
		fprintf(out, "%*s}\n", indent*4, "");
		break;
		
	default:
		if (node->type != SYNTREE_TYPE_Void)
			fprintf(out, "%s ", nodeTypeName[node->type]);
		
		fprintf(out, "%s {\n", nodeTagName[node->tag]);
		
		for (root = node->value.container.first; root != 0;
		     root = syntreeNodePtr(self, root)->next)
		{
			syntreePrint(self, root, out, indent + 1);
		}
		
		fprintf(out, "%*s}\n", indent*4, "");
	}
}

/* *** external variables *************************************************** */

const char* const nodeTagName[] = {
#define NAME(NODE) #NODE,
	SYNTREE_NODE_LIST(NAME)
#undef NAME
};

const char* const nodeTypeName[] = {
#define NAME(NODE) #NODE,
	SYNTREE_TYPE_LIST(NAME)
#undef NAME
};
