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
	struct FileBuf *fb = &window->filebuf; // alias
	struct PieceTableEntry *at = entry;
	char *text = filebuf_get_text(fb, at);
	index_t relative_index = entry_index;
	for (int count = 0; count < length; count++) {
		if (relative_index >= at->length) {
			if (at->next == NULL) break;

			relative_index = 0;
			at = at->next;
			buffer = filebuf_get_text(fb, at);
		}

		draw_next_char(text[relative_index]);
		terminal_cursor_right(1);
	}
}
