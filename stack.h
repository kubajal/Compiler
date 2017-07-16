/***************************************************************************//**
 * @file stack.h
 * @author Dorian Weber
 * @brief Enthält die Schnittstelle eines generalisierten Stacks.
 * @details
 * Hier ist ein Beispiel für die Benutzung des Stacks:
 * @code
 * int* stack;
 * 
 * stackInit(stack);
 * stackPush(stack) = 1;
 * stackPush(stack) = 2;
 * stackPush(stack) = 3;
 * 
 * while (!stackIsEmpty(stack))
 * 	printf("%i\n", stackPop(stack));
 * 
 * stackRelease(stack);
 * @endcode
 * mit der Ausgabe
 * @code
 * 3
 * 2
 * 1
 * @endcode
 * 
 * Jede Art von Daten kann auf diese Weise als Stack organisiert werden, sogar
 * komplexe Strukturen. Eine Einschränkung ist, dass man keine Zeiger auf
 * Stackelemente halten sollte, da der Stack beim Einfügen neuer Elemente unter
 * Umständen relokalisiert werden muss, was absolute Zeiger auf Elemente
 * invalidiert. Um auf Elemente zu verweisen sollten Indizes verwendet werden,
 * es sei denn du weißt was du tust!
 ******************************************************************************/

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

/* *** includes ************************************************************* */

#include <stddef.h>

/* *** structures *********************************************************** */

/**@brief Stackheader.
 * 
 * Diese Struktur wird jedem Stack im Speicher vorangestellt und beinhaltet
 * Informationen über Kapazität und aktuelle Auslastung des Stacks. Die
 * Stackelemente schließen sich dieser Struktur unmittelbar an, so dass der
 * Nutzer von dieser versteckten Information nichts bemerkt.
 */
typedef struct stack_hdr_s
{
	unsigned int len; /**<@brief Anzahl der Elemente auf dem Stack. */
	unsigned int cap; /**<@brief Kapazität des reservierten Speichers. */
} stack_hdr_t;

/* *** interface ************************************************************ */

/**@internal
 * @brief Initialisiert und gibt einen Zeiger auf den Start des Stacks zurück.
 * @param size      Größe der Stackelemente
 * @param capacity  initiale Kapazität
 * @return ein Zeiger auf den Start des Stacks, falls erfolgreich,\n
 *      \c NULL im Falle eines Speicherfehlers
 */
extern void*
stackInit(unsigned int capacity, size_t size);

/**@brief Initialisiert einen neuen Stack.
 * @param self  der zu initialisierende Stack
 * @return 0, falls keine Fehler bei der Initialisierung aufgetreten sind\n
 *      != 0 ansonsten
 */
#define stackInit(self) \
	((self = stackInit(8, sizeof((self)[0]))) == NULL ? -1 : 0)

/**@brief Gibt den Stack und alle assoziierten Strukturen frei.
 * @param self  der freizugebende Stack
 */
extern void
stackRelease(void* self);

/**@internal
 * @brief Reserviert Platz für einen neuen Wert im Stack.
 * @param self  der Stack
 * @param size  Größe der Stackelemente
 * @return der neue Zeiger auf den Start des Stacks
 */
extern void*
stackPush(void* self, size_t size);

/**@brief Legt einen Wert auf den Stack.
 * @param self  der Stack
 */
#define stackPush(self) \
	(self = stackPush(self, sizeof((self)[0])), (self)+stackCount(self)-1)[0]

/**@brief Entfernt das oberste Element des Stacks.
 * @param self  der Stack
 */
extern void
stackPop(void* self);

/**@brief Entfernt und liefert das oberste Element des Stacks.
 * @param self  der Stack
 * @return das oberste Element von \p self
 */
#define stackPop(self) \
	(stackPop(self), (self)+stackCount(self))[0]

/**@brief Gibt das oberste Element des Stacks zurück.
 * @param self  der Stack
 * @return das oberste Element von \p self
 */
#define stackTop(self) \
	(self)[stackCount(self) - 1]

/**@brief Gibt zurück, ob der Stack leer ist.
 * @param self  der Stack
 * @return 0, falls nicht leer\n
        != 0, falls leer
 */
extern int
stackIsEmpty(const void* self);

/**@brief Gibt die Anzahl der Elemente auf dem Stack zurück.
 * @param self  der Stack
 * @return Anzahl der Elemente in \p self
 */
extern unsigned int
stackCount(const void* self);

#endif /* STACK_H_INCLUDED */
