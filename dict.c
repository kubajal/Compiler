/***************************************************************************//**
 * @file dict.c
 * @author Dorian Weber
 * @brief Implementation des assoziativen Arrays.
 ******************************************************************************/

#include "dict.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/* ****************************************************** internal structures */

/**@internal
 * @brief Rückgabetyp für Lokalisierungsanfragen an ein Wörterbuch.
 */
typedef struct locate_result_s
{
	unsigned int i;  /**<@brief Rückgabeindex. */
	dict_entry_t* p; /**<@brief Zeiger auf das Rückgabeelement. */
} locate_result_t;

/**@internal
 * @brief Ganzzahlentyp für den Hashwert.
 */
typedef unsigned int hash_t;

static const hash_t FNVHashSeed = 0x811c9dc5u;
static const hash_t FNVHashPrime = 0x1000193u;

/* ******************************************************** private functions */

/**@internal
 * @brief Hashfunktion FNV-1a von Fowler, Noll und Vo.
 * @param key  der Schlüssel
 * @return der Hashwert von \p key
 */
static hash_t
fnvHash(const char* key)
{
	hash_t hash;
	
	for (hash = FNVHashSeed; *key != 0; ++key)
	{
		hash ^= *key;
		hash *= FNVHashPrime;
	}
	
	return hash;
}

/**@internal
 * @brief Findet den Eintrag zu einem gegebenen Schlüssel oder die erste freie
 * Position, falls der Schlüssel nicht enthalten ist.
 * @param[in]  self    das Wörterbuch
 * @param[in]  key     der Schlüssel
 * @param[out] result  Ergebnis
 * @return 1, falls der Eintrag gefunden wurde,\n
 *         0, ansonsten
 */
static int
locate(const dict_t* self, const char* key, locate_result_t* result)
{
	enum { UNUSED, FORMER, MATCH } state = UNUSED;
	hash_t hash = fnvHash(key);
	const unsigned int mask = (1u << self->bits) - 1;
	const unsigned int initial = hash & mask;
	
	/* berechne den initialen Index */
	locate_result_t probe = { initial, &self->data[initial] };
	
	/* berechne die Schrittweite unter Einsatz weiterer Bits des Hashwertes;
	 * die Schrittweite entsteht aus der bitweisen Rotation des Hashwertes
	 * um die Anzahl der Bits in der Hashmap und ist immer ungerade (erstes
	 * Bit auf eins fixiert); der Grund dafür ist, dass die Größe der
	 * Hashmap eine Potenz von 2 und jede ungerade Zahl relativ prim dazu
	 * ist; mit dem euklidischen Algorithmus lässt sich zeigen, dass in
	 * diesem Fall alle Einträge in einer zufälligen Permutation besucht
	 * werden; die untere Schleife findet daher garantiert einen freien
	 * Platz, falls das Wörterbuch nicht zu 100% ausgelastet ist */
	hash = (hash >> (self->bits - 1))
	     | (hash << (sizeof(hash)*CHAR_BIT - self->bits + 1))
	     | 1;
	
	/* diese Schleife läuft, bis wir den initialen Index erneut sehen */
	do
	{
		/* teste, ob der aktuelle Eintrag schon in Benutzung war */
		if (probe.p->key == NULL)
		{
			/* teste, ob wir bisher noch keinen freien Eintrag
			 * gefunden haben */
			if (state == UNUSED)
				*result = probe;
			
			/* Abbruch: der Eintrag war noch nie in Benutzung */
			break;
		}
		
		/* teste, ob der aktuelle Eintrag im Moment benutzt wird */
		if (probe.p->val == NULL)
		{
			/* teste, ob es sich um das erste Mal handelt,
			 * dass wir einen freien Eintrag finden */
			if (state == UNUSED)
			{
				*result = probe;
				state = FORMER;
			}
		}
		/* teste, ob dieser Eintrag der gesuchte ist */
		else if (strcmp(probe.p->key, key) == 0)
		{
			*result = probe;
			state = MATCH;
			break;
		}
		
		/* gehe zum nächsten Index */
		probe.i = (probe.i + hash) & mask;
		probe.p = &self->data[probe.i];
	}
	while (probe.i != initial);
	
	return state == MATCH;
}

/**@internal
 * @brief Verdoppelt die Größe des Wörterbuchs und hasht alle Werte neu.
 * @param self  das Wörterbuch
 */
static void
grow(dict_t* self)
{
	dict_entry_t* data = self->data, *it, *end;
	unsigned int cap = 1u << self->bits;
	
	/* alloziere ein doppelt so großes Array für die Einträge */
	self->data = calloc(2*cap, sizeof(*self->data));
	
	/* teste auf Speicherfehler */
	if (self->data == NULL)
	{
		fputs("out-of-memory error\n", stderr);
		exit(-1);
	}
	
	/* aktualisiere die Anzahl der Bits und Plätze */
	++self->bits;
	self->left += cap;
	
	/* iteriere über das Originalarray und füge die Elemente neu ein */
	for (it = data, end = it + cap; it < end; ++it)
	{
		if (it->val != NULL)
		{
			locate_result_t loc;
			
			locate(self, it->key, &loc);
			*loc.p = *it;
		}
	}
	
	/* gib' das alte Array frei */
	free(data);
}

/* ********************************************************* public functions */

int
dictInit(dict_t* self)
{
	self->bits = 3;
	self->left = 1u << self->bits;
	self->data = calloc(self->left, sizeof(*self->data));
	return self->data == NULL;
}

void
dictRelease(dict_t* self)
{
	dict_entry_t *it, *end;
	
	/* iteriere über das Array und gib' Schlüssel benutzter Elemente frei */
	for (it = self->data, end = it + (1u << self->bits); it < end; ++it)
	{
		if (it->val != NULL)
			free(it->key);
	}
	
	free(self->data);
}

void*
dictSet(dict_t* self, const char* key, const void* val)
{
	const void* oldVal = NULL;
	locate_result_t loc;
	
	assert(key != NULL && val != NULL);
	
	/* vergrößere die Hashmap bei (potentiellem) Platzmangel */
	if (self->left == 0)
		grow(self);
	
	/* finde den Eintrag */
	if (locate(self, key, &loc))
	{
		/* überschreibe den aktuellen Wert */
		oldVal = loc.p->val;
		loc.p->val = val;
	}
	else
	{
		/* erstelle den Wert neu */
		size_t size = strlen(key) + 1;
		
		loc.p->key = malloc(size);
		memcpy(loc.p->key, key, size);
		
		loc.p->val = val;
		--self->left;
	}
	
	/* gib den vorigen Wert zurück */
	return (void*) oldVal;
}

void*
dictGet(const dict_t* self, const char* key)
{
	locate_result_t loc;
	
	if (locate(self, key, &loc))
		return (void*) loc.p->val;
	
	return NULL;
}

void*
dictDel(dict_t* self, const char* key)
{
	const void* val = NULL;
	locate_result_t loc;
	
	if (locate(self, key, &loc))
	{
		free(loc.p->key);
		val = loc.p->val;
		loc.p->val = NULL;
		++self->left;
	}
	
	return (void*) val;
}
