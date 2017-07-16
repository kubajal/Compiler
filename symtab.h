/***************************************************************************//**
 * @file symtab.h
 * @author Dorian Weber
 * @brief Enthält eine Datenstruktur für die Verwaltung von Symbolen in einer
 * Tabelle mit geschichteten Sichtbarkeitsbereichen.
 * @details
 * Hier ist ein Beispiel für die Benutzung der Symboltabelle:
 * @code
 * symtab_t tab;
 * symtab_symbol_t* sym;
 * 
 * symtabInit(&tab);
 * 
 * sym = symtabSymbol("foo", SYNTREE_TYPE_Void);
 * sym->is_function = 1;
 * symtabParam(sym, symtabSymbol("c", SYNTREE_TYPE_Integer));
 * symtabParam(sym, symtabSymbol("b", SYNTREE_TYPE_Float));
 * symtabParam(sym, symtabSymbol("a", SYNTREE_TYPE_Boolean));
 * symtabInsert(&tab, sym);
 * 
 * sym = symtabSymbol("main", SYNTREE_TYPE_Void);
 * sym->is_function = 1;
 * symtabInsert(&tab, sym);
 * 
 * symtabEnter(&tab);
 * symtabInsert(&tab, symtabSymbol("foo", SYNTREE_TYPE_Float));
 * symtabInsert(&tab, symtabSymbol("bar", SYNTREE_TYPE_Float));
 * 
 * symtabPrint(&tab, stdout);
 * symtabRelease(&tab);
 * @endcode
 * mit der Ausgabe
 * @code
 * +---------------------
 * |Symbol count: 4
 * |Block count:  2
 * |--------< 0 >--------
 * |(0) Void foo(Boolean a:[0], Float b:[0], Integer c:[0])
 * |(1) Void main()
 * |--------< 1 >--------
 * |(2) Float foo:[0] {shadows 0}
 * |(3) Float bar:[1]
 * +---------------------
 * @endcode
 * Im oberen Teil der Ausgabe stehen generelle Kenngrößen der Symboltabelle,
 * danach folgen die gestapelten Schichten, jeweils mit allen deklarierten
 * Symbolen. Funktionen haben Parameter und Variablen einen Index. Der Index
 * wird im Syntaxbaum später zur Adressierung der Variablen verwendet. Falls
 * ein Variablenname in mehreren Schichten auftaucht, wird der Verweis auf die
 * überdeckte Schicht mit ausgegeben. Die Parameter haben in der Beispielausgabe
 * alle den Index 0, weil sie niemals in die Symboltabelle eingefügt worden sind
 * und es daher niemals möglich war auf sie zuzugreifen. Die Position im Stack
 * wird erst beim Einfügen in die Symboltabelle berechnet.
 ******************************************************************************/

#ifndef SYMTAB_H_INCLUDED
#define SYMTAB_H_INCLUDED

/* *** includes ************************************************************* */

#include <stdio.h>
#include "syntree.h"
#include "stack.h"
#include "dict.h"

/* *** structures *********************************************************** */

/**@brief Struktur eines Symbols in der Symboltabelle.
 * @see http://en.cppreference.com/w/c/language/bit_field
 */
typedef struct symtab_symbol_s
{
	char* name;                       /**<@brief Bezeichner im Quellcode. */
	struct symtab_symbol_s* rec_prev; /**<@brief Zeiger auf Vorgängerdefinition. */
	struct symtab_symbol_s* par_next; /**<@brief Zeiger auf (Folge-) Parameter. */
	
	syntree_nid  body;  /**<@brief Knoten-ID für Funktionskörper. */
	unsigned int id;    /**<@brief Position im decl-Stack. */
	unsigned int pos;   /**<@brief Position im Variablenkontext. */
	
	unsigned int is_function : 1; /**<@brief 1 für Funktionen. */
	unsigned int is_param    : 1; /**<@brief 1 für Funktionsparameter. */
	unsigned int is_global   : 1; /**<@brief 1 für globale Variablen */
	unsigned int type        : 3; /**<@brief Symboltyp. */
} symtab_symbol_t;

/**@brief Struktur der Symboltabelle.
 */
typedef struct symtab_s
{
	dict_t map; /**<@brief Wörterbuch Bezeichner -> aktuelle Definition */
	symtab_symbol_t** decl; /**<@brief Stack aller deklarierten Symbole. */
	unsigned int* block; /**<@brief Stack der Anzahl von Blockvariablen. */
	unsigned int maxpos; /**<@brief Maximale Anzahl lokaler Variablen. */
} symtab_t;

/* *** public interface ***************************************************** */

/**@brief Initialisiert eine neue Symboltabelle.
 * 
 * Alle Variablen, die auf der obersten Ebene deklariert werden, werden
 * automatisch als global markiert (is_global == 1). Variablen in Blöcken sind
 * lokal (is_global == 0).
 * 
 * @param self  die Symboltabelle
 * @return 0, falls keine Fehler aufgetreten sind,\n
 *      != 0, falls nicht genug Speicher zur Verfügung steht
 */
extern int
symtabInit(symtab_t* self);

/**@brief Gibt eine Symboltabelle und alle darin enthaltenen Symbole frei.
 * @param self  die Symboltabelle
 */
extern void
symtabRelease(symtab_t* self);

/**@brief Öffnet einen neuen Sichtbarkeitsbereich.
 * @param self  die Symboltabelle
 */
extern void
symtabEnter(symtab_t* self);

/**@brief Schließt den Sichtbarkeitsbereich, räumt alle Variablen ab und stellt
 * den Zustand vor der Öffnung des Blocks wieder her.
 * @param self  die Symboltabelle
 */
extern void
symtabLeave(symtab_t* self);

/**@brief Deklariert ein neues Symbol im aktuellen Sichtbarkeitsbereich.
 * 
 * Falls es sich um die oberste Ebene handelt (bevor der erste Block geöffnet
 * wurde), wird das Symbol als global markiert (is_global == 1). Die Funktion
 * prüft, ob ein Symbol dieses Namens bereits im aktuellen Sichtbarkeitsbereich
 * existert. Sollte das der Fall sein, wird die Operation ignoriert und ein
 * Rückgabewert != 0 zurückgegeben.
 * 
 * Symbole mit gesetztem Parameterbit (is_param == 1) können ganz normal als
 * lokale Symbole eingefügt werden, werden beim Abräumen der Symbole am Ende
 * des lokalen Blocks jedoch nicht freigegeben, sondern erst, wenn das Symbol
 * mit gesetztem Funktionsbit (is_function == 1) freigegeben wird.
 * 
 * @param self  die Symboltabelle
 * @param sym   das Symbol
 * @return 0, falls erfolgreich,\n
 *      != 0, falls eine Doppel-Deklaration vorliegt
 */
extern int
symtabInsert(symtab_t* self, symtab_symbol_t* sym);

/**@brief Hängt ein neues Symbol an den Anfang der Parameterliste eines
 * Funktionssymbols.
 * 
 * Das anzuhängende Symbol wird automatisch als Parameter markiert.
 * 
 * @param func  das Funktionssymbol
 * @param sym   das Parametersymbol
 */
extern void
symtabParam(symtab_symbol_t* func, symtab_symbol_t* sym);

/**@brief Sucht nach einem Symbol mit einem Bezeichner.
 * 
 * Das gefundene Symbol entspricht der aktuell gültigen Definition.
 * 
 * @param self  die Symboltabelle
 * @param id    der Bezeichner
 * @return das assoziierte Symbol, falls vorhanden,\n
 *      \c NULL, ansonsten
 */
extern symtab_symbol_t*
symtabLookup(const symtab_t* self, const char* id);

/**@brief Erstellt und initialisiert ein neues Symbol für die Symboltabelle.
 * 
 * Das Symbol sollte konfiguriert und dann in die Symboltabelle eingetragen
 * werden.
 * 
 * @param name  der Name des Symbols (später der Bezeichner)
 * @param type  der Datentyp des Symbols
 * 
 * @return das neu-erstellte Symbol
 */
extern symtab_symbol_t*
symtabSymbol(const char* name, syntree_node_type type);

/**@brief Wählt den ersten Funktionsparameter aus.
 * @param func  das Funktionssymbol
 * @return der erste Funktionsparameter
 */
extern symtab_symbol_t*
symtabParamFirst(const symtab_symbol_t* func);

/**@brief Wählt den nächsten Funktionsparameter aus.
 * @param param  der aktuelle Funktionsparameter
 * @return der nächste Parameter oder\n
 *      \c NULL, falls \p param der letzte Parameter ist 
 */
extern symtab_symbol_t*
symtabParamNext(const symtab_symbol_t* param);

/**@brief Berechnet die maximale Anzahl der lokalen Variablen auf dem Stack für
 * den aktuellen oder letzten lokalen Block.
 * 
 * @param self  die Symboltabelle
 * @return maximale Anzahl lokaler Variablen im lokalen Block
 */
extern unsigned int
symtabMaxLocals(const symtab_t* self);

/**@brief Berechnet die maximale Anzahl der globalen Variablen auf dem Stack.
 * @param self  die Symboltabelle
 * @return maximale Anzahl globaler Variablen
 */
extern unsigned int
symtabMaxGlobals(const symtab_t* self);

/**@brief Gibt den gesamten Inhalt der Symboltabelle aus.
 * @param self  die Symboltabelle
 * @param out   der Ausgabestrom
 */
extern void
symtabPrint(const symtab_t* self, FILE* out);

#endif /* SYMTAB_H_INCLUDED */
