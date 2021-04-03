/* draw.c
 * author: Andrew Klinge
 */

#include "draw.h"
#include "terminal.h"

/* Draws the line starting at index i (ending at new line character)
 * at the given character coordinates on the terminal.
 */
void draw_line(struct Window *window, index_t i, int x, int y) {
	// TODO
}

/* Draws 'n' characters starting at the relative index within the given entry. */
void draw_chars(struct PieceTableEntry *entry, int line, int character, index_t n) {
	// TODO
}
