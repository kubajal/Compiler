/***************************************************************************//**
 * @file stack.c
 * @author Dorian Weber
 * @brief Implementation des generalisierten Stacks.
 ******************************************************************************/

#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* ********************************************************* public functions */

/* (die runden Klammern um einige Funktionsnamen sind notwendig, da Makros
 * gleichen Namens existieren und der Präprozessor diese expandieren würde) */

void*
(stackInit)(unsigned int capacity, size_t size)
{
	stack_hdr_t* hdr = malloc(sizeof(*hdr) + size*capacity);
	
	if (hdr == NULL)
		return NULL;
	
	hdr->len = 0;
	hdr->cap = capacity;
	
	return hdr + 1;
}

void
stackRelease(void* self)
{
	free(((stack_hdr_t*) self) - 1);
}

void*
(stackPush)(void* self, size_t size)
{
	stack_hdr_t* hdr = ((stack_hdr_t*) self) - 1;
	
	if (hdr->len == hdr->cap)
	{
		hdr->cap *= 2;
		hdr = realloc(hdr, sizeof(*hdr) + size*hdr->cap);
		
		if (hdr == NULL)
		{
			fputs("out-of-memory error\n", stderr);
			exit(-1);
		}
	}
	
	++hdr->len;
	return hdr + 1;
}

void
(stackPop)(void* self)
{
	stack_hdr_t* hdr = ((stack_hdr_t*) self) - 1;
	assert(hdr->len > 0);
	--hdr->len;
}

int
stackIsEmpty(const void* self)
{
	return ((stack_hdr_t*) self)[-1].len == 0;
}

unsigned int
stackCount(const void* self)
{
	return ((stack_hdr_t*) self)[-1].len;
}
