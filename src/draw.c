/* draw.c
 * author: Andrew Klinge
 */

#include "draw.h"
#include "terminal.h"

static void draw_next_char(char c) {
	printf("%c", c);
}

/* Draws 'length' number of characters starting at the index relative to the start of the entry and the current cursor position of
 * the given window.
 */
void draw_chars(struct Window *window, struct PieceTableEntry *entry, index_t entry_index, index_t length) {
	//const int save_cursor_line = window->cursor_line;
	//const int save_cusor_

	struct PieceTableEntry *at = entry;
	char *buffer = filebuf_get_buffer(&window->filebuf, at);
	index_t relative_index = entry_index;
	for (int count = 0; count < length; count++) {
		if (relative_index >= at->length) {
			if (at->next == NULL) break;

			relative_index = 0;
			at = at->next;
			buffer = filebuf_get_buffer(&window->filebuf, at);
		}

		draw_next_char(buffer[relative_index]);
		terminal_cursor_right(1);
	}

	//window->cursor_line = save_cursor_line;
	//window->cursor_char = save_cursor_char;
}
