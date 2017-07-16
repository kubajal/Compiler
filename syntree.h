/***************************************************************************//**
 * @file syntree.h
 * @author Dorian Weber
 * @brief Enthält eine Datenstruktur für abstrakte Syntaxbäume.
 * @details
 * Hier ist ein Beispiel für die Konstruktion eines Syntaxbaumes:
 * @code
 * syntree_t ast;
 * syntree_nid node;
 * 
 * syntreeInit(&ast);
 * 
 * node = syntreeNodeTag(&ast, SYNTREE_TAG_Times, syntreeNodeInteger(&ast, 6));
 * syntreeNodeAppend(&ast, node, syntreeNodeInteger(&ast, 4));
 * node = syntreeNodePair(&ast, SYNTREE_TAG_Plus, syntreeNodeInteger(&ast, 2), node);
 * 
 * syntreePrint(&ast, node, stdout, 0);
 * syntreeRelease(&ast);
 * @endcode
 * mit der Ausgabe
 * @code
 * Plus {
 *     Integer 2
 *     Times {
 *         Integer 6
 *         Integer 4
 *     }
 * }
 * @endcode
 * 
 * Mithilfe der Knoten-IDs, die Ein- und Ausgabewerte der konstruierenden
 * Funktionen sind, kann die Datenstruktur vom Blatt zur Wurzel konstruiert
 * werden. Alle Knotentypen werden in einer X-Liste deklariert und die Literale  
 * (Blätter des abstrakten Syntaxbaumes) haben jeweils ein eigene
 * Konstruktionsroutine, die den entsprechenden Wert als Parameter erhält.
 * Knoten übernehmen im Falle von dynamisch allozierten Werten den Besitz und
 * geben diese beim Abbau der Knoten selbständig frei.
 ******************************************************************************/

#ifndef SYNTREE_H_INCLUDED
#define SYNTREE_H_INCLUDED

/**@brief X-Liste aller benötigten Knotentypen für C1-Programme.
 * @see https://en.wikipedia.org/wiki/X_Macro
 */
#define SYNTREE_NODE_LIST(NODE) \
	/* Literale */ \
	NODE(Integer) \
	NODE(Float) \
	NODE(Boolean) \
	NODE(String) \
	NODE(LocVar) \
	NODE(GlobVar) \
	/* Anweisungen */ \
	NODE(Program) \
	NODE(Function) \
	NODE(Call) \
	NODE(Sequence) \
	NODE(If) \
	NODE(For) \
	NODE(DoWhile) \
	NODE(While) \
	NODE(Print) \
	NODE(Assign) \
	NODE(Return) \
	/* Ausdrücke */ \
	NODE(Cast) \
	NODE(Plus) \
	NODE(Minus) \
	NODE(Times) \
	NODE(Divide) \
	NODE(LogOr) \
	NODE(LogAnd) \
	NODE(Uminus) \
	NODE(Eqt) \
	NODE(Neq) \
	NODE(Leq) \
	NODE(Geq) \
	NODE(Lst) \
	NODE(Grt)

/**@brief X-Liste aller eingebauten Datentypen für C1-Programme.
 * @see https://en.wikipedia.org/wiki/X_Macro
 */
#define SYNTREE_TYPE_LIST(TYPE) \
	TYPE(Void) \
	TYPE(Boolean) \
	TYPE(Integer) \
	TYPE(Float) \
	TYPE(String)

/* *** includes ************************************************************* */

#include <stdio.h>

/* *** structures *********************************************************** */

/* Vorwärtsdeklaration */
struct symtab_symbol_s;

#define TAG(NAME)  SYNTREE_TAG_ ## NAME,
#define TYPE(NAME) SYNTREE_TYPE_ ## NAME,

/**@brief Enumeration aller Knotenarten im Syntaxbaum.
 */
typedef enum syntree_node_tag_e
{
	SYNTREE_NODE_LIST(TAG)
} syntree_node_tag;

/**@brief Enumeration aller semantischen Knotentypen im Syntaxbaum.
 */
typedef enum syntree_node_type_e
{
	SYNTREE_TYPE_LIST(TYPE)
} syntree_node_type;

#undef TAG
#undef TYPE

/**@brief Eindeutiger Identifikator für einen Knoten in einem Syntaxbaum.
 */
typedef unsigned int syntree_nid;

/**@brief Eine partitionierte Vereinigung für Knoten.
 * @see https://en.wikipedia.org/wiki/Tagged_union
 */
typedef struct syntree_node_s
{
	/**@brief Knotentag.
	 * @note Agiert in diesem Kontext als tag für die union.
	 */
	syntree_node_tag tag;
	
	/**@brief Knotentyp.
	 */
	syntree_node_type type;
	
	/**@brief Index des nächsten verbundenen Knoten.
	 */
	syntree_nid next;
	
	/**@brief Nutzlast des Knotens.
	 */
	union syntree_node_value_u {
		/**@brief Boolesche Konstante.
		 */
		int boolean;
		
		/**@brief Integerkonstante.
		 */
		int integer;
		
		/**@brief Fließkommakonstante.
		 */
		float real;
		
		/**@brief Stringkonstante.
		 */
		char* string;
		
		/**@brief Variablenindex.
		 */
		int variable;
		
		/**@brief Funktion.
		 */
		struct {
			syntree_nid body;     /**<@brief Funktionskörper. */
			unsigned int locals;  /**<@brief Variablenanzahl. */
		} function;
		
		/**@brief Programm.
		 * @note Identisch mit der Struktur für Funktionen.
		 */
		struct {
			syntree_nid body;     /**<@brief Programmkörper. */
			unsigned int globals; /**<@brief Variablenanzahl. */
		} program;
		
		/**@brief Behälter weiterer Knoten.
		 */
		struct {
			syntree_nid first;   /**<@brief Erster Knoten. */
			syntree_nid last;    /**<@brief Letzter Knoten. */
		} container;
	} value;
} syntree_node_t;

/**@brief Struktur des abstrakten Syntaxbaumes.
 */
typedef struct syntree_s
{
	/**@brief Knotenarray.
	 * @note Der Knoten mit der ID 0 ist für die Wurzel reserviert.
	 */
	syntree_node_t* nodes;
	
	unsigned int len; /**<@brief Anzahl der allozierten Knoten. */
	unsigned int cap; /**<@brief Kapazität des Knotenarrays. */
} syntree_t;

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

/**@brief Gegeben eine Knoten-ID, ermittelt den entsprechenden Zeiger.
 * @param self  der Syntaxbaum
 * @param id    die Knoten-ID
 * @return ein Zeiger auf den entsprechenden Knoten
 */
extern syntree_node_t*
syntreeNodePtr(const syntree_t* self, syntree_nid id);

/**@brief Gegeben einen Knotenzeiger, ermittelt die entsprechende ID.
 * @param self  der Syntaxbaum
 * @param node  der Zeiger auf einen Knoten in \p self
 * @return die dazugehörige Knoten-ID
 */
extern syntree_nid
syntreeNodeId(const syntree_t* self, const syntree_node_t* node);

/**@brief Erstellt einen neuen Knoten mit einem Wahrheitswert als Inhalt.
 * @param self  der Syntaxbaum
 * @param flag  der Wahrheitswert
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeBoolean(syntree_t* self, int flag);

/**@brief Erstellt einen neuen Knoten mit einem Zahlenwert als Inhalt.
 * @param self    der Syntaxbaum
 * @param number  die Zahl
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeInteger(syntree_t* self, int number);

/**@brief Erstellt einen neuen Knoten mit einem Fließkommawert als Inhalt.
 * @param self    der Syntaxbaum
 * @param number  die Fließkommazahl
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeFloat(syntree_t* self, float number);

/**@brief Erstellt einen neuen Knoten mit einer Zeichenkette als Inhalt.
 * @param self  der Syntaxbaum
 * @param text  die Zeichenkette
 * @note \p text wird nicht kopiert, sondern übernommen;
 *       der Syntaxbaum ist für die Freigabe zuständig
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeString(syntree_t* self, char* text);

/**@brief Erstellt einen neuen Knoten für eine Variablenreferenz.
 * @param self    der Syntaxbaum
 * @param symbol  das Symbol aus der Symboltabelle
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeVariable(syntree_t* self, const struct symtab_symbol_s* symbol);

/**@brief Erstellt einen neuen Knoten für eine Typkonvertierung.
 * @param self    der Syntaxbaum
 * @param target  der Zieltyp
 * @param id      Knoten-ID des Operanden
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeCast(syntree_t* self, syntree_node_type target, syntree_nid id);

/**@brief Erstellt einen leeren Containerknoten.
 * @param self  die Symboltabelle
 * @param tag   die Knotenart
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeEmpty(syntree_t* self, syntree_node_tag tag);

/**@brief Kapselt einen Knoten innerhalb eines anderen Knotens.
 * @param self  der Syntaxbaum
 * @param tag   die Knotenart
 * @param id    der zu kapselnde Knoten
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodeTag(syntree_t* self, syntree_node_tag tag, syntree_nid id);

/**@brief Kapselt zwei Knoten innerhalb eines Knoten.
 * @param self  der Syntaxbaum
 * @param tag   die Knotenart
 * @param id1   erster Knoten
 * @param id2   zweiter Knoten
 * @return ID des neu erstellten Knoten
 */
extern syntree_nid
syntreeNodePair(syntree_t* self, syntree_node_tag tag,
                syntree_nid id1, syntree_nid id2);

/**@brief Hängt einen Knoten an das Ende eines Listenknotens.
 * @param self  der Syntaxbaum
 * @param list  Listenknoten
 * @param elem  anzuhängender Knoten
 * @return ID des Listenknoten
 */
extern syntree_nid
syntreeNodeAppend(syntree_t* self, syntree_nid list, syntree_nid elem);

/**@brief Gibt alle Zahlenknoten rekursiv (depth-first) aus.
 * @param self    der Syntaxbaum
 * @param root    der Wurzelknoten
 * @param out     der Ausgabestrom
 * @param indent  initiale Einrückung
 */
extern void
syntreePrint(const syntree_t* self, syntree_nid root, FILE* out, int indent);

/* *** external variables *************************************************** */

/**@brief Konstantes Array, das von Knotentags auf Strings abbildet.
 */
extern const char* const
nodeTagName[];

/**@brief Konstantes Array, das von Knotentypen auf Strings abbildet.
 */
extern const char* const
nodeTypeName[];

#endif /* SYNTREE_H_INCLUDED */
