/*Compilerbau, Sommersemester 2017, Gruppe 20: Wilhelm Bomke, Jakub Jalowiec & David Bachorska,  */
#ifndef SYNTREE_H_INCLUDED
#define SYNTREE_H_INCLUDED

#include <stdlib.h>

#define ERROR_ID -1

/*Beschreibung der Implementation der Datenstruktur:
 Die Implementation benutzt 3 Typen von Strukturen:
 -syntree_t:repräsentiert den Syntaxbaum. syntree_t enthaelt zwei Zeiger(syntree_nlist_elem * first und syntree_nlist_elem * last), die die globale Liste von Knoten repräsentieren sowie eine Zahl(syntree_nid freeID), die die nächste freie Knotenidentifikationsnummer für den Baum speichert.
 -syntree_node: repräsentiert einen Knoten. Diese Struktur enthält: die Identifikationsnummer(syntree_nid ID), den Wert(int value), einen Zeiger auf die Eltern(syntree_node * parent) und zwei Zeiger(syntree_nlist_elem * first und syntree_nlist_elem * last), die die Liste von Kindern repräsentieren. Der Wert "-1"(oder first != last) heißt, dass der Knoten ein Listenknoten ist.
 -syntree_nlist_elem: repräsentiert ein Element in der dynamischen Liste. Die Struktur enthält: einen Zeiger auf das nächste Element in der Liste(synree_nlist_elem * next) und einen Zeiger(syntree_node * node) auf mit dem syntree_nlist_elem verbundenen Knoten.
 Achtung: eine leere Liste enthaelt nur ein triviales Element, welches das Ende der Liste repräsentiert.*/

/* *** structures *********************************************************** */

typedef int syntree_nid;

typedef struct syntree_t
{
	struct syntree_nlist_elem *first, *last;                /* global node list */
    int freeID;
} syntree_t;

struct syntree_node
{
	syntree_nid ID;                                         /* ID=-1 <=> Listenknoten */
    struct syntree_node *parent;
    int value;
	struct syntree_nlist_elem *first, *last;				/* children */
};

struct syntree_nlist_elem                                   /* element of a node list */
{
	struct syntree_node *node;
	struct syntree_nlist_elem *next;
};


/* *** interface ************************************************************ */

/**@brief Initialisiert einen neuen Syntaxbaum.
 * @param self  der zu initialisierende Syntaxbaum
 * @return 0, falls keine Fehler bei der Initialisierung aufgetreten sind\n
 *      != 0 ansonsten
 */
extern int
syntreeInit(syntree_t* self);

/**@brief Gibt den Syntaxbaum und alle assoziierten Strukturen frei.
 * @param self  der freizugebende Syntaxbaum
 */
extern void
syntreeRelease(syntree_t* self);

/**@brief Erstellt einen neuen Knoten mit einem Zahlenwert als Inhalt.
 * @param self    der Syntaxbaum
 * @param number  die Zahl
 * @param ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeNumber(syntree_t* self, int number);

/**@brief Kapselt einen Knoten innerhalb eines anderen Knotens.
 * @param self  der Syntaxbaum
 * @param id    der zu kapselnde Knoten
 * @param ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeTag(syntree_t* self, syntree_nid id);

/**@brief Kapselt zwei Knoten innerhalb eines Knoten.
 * @param self  der Syntaxbaum
 * @param id1   erster Knoten
 * @param id2   zweiter Knoten
 * @param ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodePair(syntree_t* self, syntree_nid id1, syntree_nid id2);

/**@brief Hängt einen Knoten an das Ende eines Listenknotens.
 * @param self  der Syntaxbaum
 * @param list  Listenknoten
 * @param elem  anzuhängender Knoten
 * @param ID des Listenknoten
 */
extern syntree_nid
syntreeNodeAppend(syntree_t* self, syntree_nid list, syntree_nid elem);

/**@brief Hängt einen Knoten an den Anfang eines Listenknotens.
 * @param self  der Syntaxbaum
 * @param elem  anzuhängender Knoten
 * @param list  Listenknoten
 * @param ID des Listenknoten
 */
extern syntree_nid
syntreeNodePrepend(syntree_t* self, syntree_nid elem, syntree_nid list);

/**@brief Gibt alle Zahlenknoten rekursiv (depth-first) aus.
 * @param self  der Syntaxbaum
 * @param root  der Wurzelknoten
 */
extern void
syntreePrint(const syntree_t* self, syntree_nid root);

#endif /* SYNTREE_H_INCLUDED */
