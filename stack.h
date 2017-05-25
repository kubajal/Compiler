/*Compilerbau, Sommersemester 2017, Gruppe 20: Wilhelm Bomke, Jakub Jalowiec & David Bachorska  */
#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

/* *** structures *********************************************************** */

typedef struct stackelement  /*Struktur eines Elements*/
{
    int value;
    struct stackelement* ptr;
} stackelement_t;

/**@brief Struktur des Stacks.
 */
typedef struct intstack
{
    stackelement_t * top;
} intstack_t;


/* *** interface ************************************************************ */

/**@brief Initialisiert einen neuen intstack_t.
 * @param self  der zu initialisierende intstack_t
 * @return 0, falls keine Fehler bei der Initialisierung aufgetreten sind\n
 *      != 0 ansonsten
 */
extern int
stackInit(intstack_t* self);

/**@brief Gibt den intstack_t und alle assoziierten Strukturen frei.
 * @param self  der freizugebende intstack_t
 */
extern void
stackRelease(intstack_t* self);

/**@brief Legt einen Wert auf den intstack_t.
 * @param self  der intstack_t
 * @param i     der Wert
 */
extern void
stackPush(intstack_t* self, int i);

/**@brief Gibt das oberste Element des Stacks zurück.
 * @param self  der intstack_t
 * @return das oberste Element von \p self
 */
extern int
stackTop(const intstack_t* self);

/**@brief Entfernt und liefert das oberste Element des Stacks.
 * @param self  der intstack_t
 * @return das oberste Element von \p self
 */
extern int
stackPop(intstack_t* self);

/**@brief Gibt zurück, ob der intstack_t leer ist.
 * @param self  der intstack_t
 * @return 0, falls nicht leer\n
        != 0, falls leer
 */
extern int
stackIsEmpty(const intstack_t* self);

/**@brief Gibt den Inhalt des Stacks beginnend mit dem obersten Element auf der Standardausgabe aus.
 * @param self  der intstack_t
 */
extern void
stackPrint(const intstack_t* self);

#endif /* STACK_H_INCLUDED */
