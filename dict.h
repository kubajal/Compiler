/***************************************************************************//**
 * @file dict.h
 * @author Dorian Weber
 * @brief Enthält die Schnittstelle für ein assoziatives Array (Wörterbuch).
 * @details
 * Hier ist ein Beispiel für die Benutzung des Wörterbuches:
 * @code
 * dict_t dict;
 * 
 * dictInit(&dict);
 * 
 * dictSet(&dict, "One", "Eins");
 * dictSet(&dict, "Two", "Zwei");
 * dictSet(&dict, "Three", "Drei");
 * 
 * printf("One -> %s\n", dictGet(&dict, "One"));
 * printf("Two -> %s\n", dictGet(&dict, "Two"));
 * printf("Three -> %s\n", dictGet(&dict, "Three"));
 * 
 * dictSet(&dict, "One", "Uno");
 * dictDel(&dict, "Two");
 * 
 * printf("One -> %s\n", dictGet(&dict, "One"));
 * printf("Two -> %s\n", dictGet(&dict, "Two"));
 * printf("Three -> %s\n", dictGet(&dict, "Three"));
 * 
 * dictRelease(&dict);
 * @endcode
 * mit der Ausgabe
 * @code
 * One -> Eins
 * Two -> Zwei
 * Three -> Drei
 * One -> Uno
 * Two -> (null)
 * Three -> Drei
 * @endcode
 ******************************************************************************/

#ifndef DICT_H_INCLUDED
#define DICT_H_INCLUDED

/* *** structures *********************************************************** */

/**@brief Wörterbucheintrag.
 */
typedef struct dict_entry_s
{
	char* key;       /**<@brief Der Schlüssel. */
	const void* val; /**<@brief Der Wert. */
} dict_entry_t;

/**@brief Wörterbuch.
 */
typedef struct dict_s
{
	/**@brief Datenfeld für Schlüssel-/Wertpaare.
	 */
	dict_entry_t* data;
	
	/**@brief Anzahl der Elemente, die eingefügt werden können, bevor neuer
	 * Platz benötigt wird.
	 */
	unsigned int left;
	
	/**@brief Anzahl der Bits für die Kapazität des Datenfelds.
	 * 
	 * Es wird statt der Kapazität die Anzahl der verwendeten Bits
	 * gespeichert, da die Größe der Hashmap per Konstruktion eine Potenz
	 * von zwei ist. Dadurch vereinfacht sich bitweise Rotation in der
	 * Lokalisationsroutine. Die tatsächliche Kapazität findet man via
	 * 1 << bits.
	 */
	unsigned int bits;
} dict_t;

/* *** interface ************************************************************ */

/**@brief Initialisiert das Wörterbuch.
 * @param self  das Wörterbuch
 * @return 0, falls keine Fehler aufgetreten sind,\n
 *      != 0, falls nicht genug Speicher zur Verfügung steht
 */
extern int
dictInit(dict_t* self);

/**@brief Gibt ein Wörterbuch und alle darin gespeicherten Schlüssel frei.
 * @param self  das Wörterbuch
 */
extern void
dictRelease(dict_t* self);

/**@brief Assoziiert einen Schlüssel mit einem Wert.
 * @param self    das Wörterbuch
 * @param key     der Schlüssel
 * @param val     der neue Wert
 * @return der alte Wert, falls er überschrieben wurde,\n
 *      \c NULL, ansonsten
 */
extern void*
dictSet(dict_t* self, const char* key, const void* val);

/**@brief Gibt den Wert zu einem Schlüssel zurück.
 * @param self  das Wörterbuch
 * @param key   der Schlüssel
 * @return der assoziierte Wert, falls der Schlüssel enthalten ist,\n
 *      \c NULL, ansonsten
 */
extern void*
dictGet(const dict_t* self, const char* key);

/**@brief Entfernt eine Schlüssel-/Wert-Assoziation.
 * @param self  das Wörterbuch
 * @param key   der Schlüssel
 * @return der entfernte Wert, falls der Schlüssel enthalten ist,\n
 *      \c NULL, ansonsten
 */
extern void*
dictDel(dict_t* self, const char* key);

#endif /* DICT_H_INCLUDED */
