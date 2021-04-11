/* draw.c
 * author: Andrew Klinge
 */

#include "draw.h"
#include "terminal.h"

/* Simply draws the character to the screen at the current cursor position.
 * Does not move the cursor.
 */
inline void draw_char(char c) {
	printf("%c", c);
}

/* Draws 'length' number of characters starting at the index in the file and
 * at the current cursor position in the given window.
 */
void draw_chars(struct Window *window, index_t file_index, index_t length) {
	struct FileBuf *fb = &window->filebuf; // alias
	index_t relative_index;
	struct PieceTableEntry *at = filebuf_entry_at(fb, file_index, &relative_index);
	if (at == NULL) return; // empty filebuf
	char *text = filebuf_get_text(fb, at);
	for (int count = 0; count < length; count++) {
		if (relative_index >= at->length) {
			if (at->next == NULL) break;

			relative_index = 0;
			at = at->next;
			text = filebuf_get_text(fb, at);
		}

		draw_char(text[relative_index]);
		terminal_cursor_right(1);
	}
}

/* Draws characters starting at the file index until either a new line character is
 * encountered or the end of the file is, whichever comes first.
 * This is done at the current cursor position in the given window.
 */
void draw_line(struct Window *window, index_t file_index) {
	struct FileBuf *fb = &window->filebuf; // alias
	index_t relative_index;
	struct PieceTableEntry *at = filebuf_entry_at(fb, file_index, &relative_index);
	if (at == NULL) return; // empty filebuf
	char *text = filebuf_get_text(fb, at);
	while (1) {
		if (relative_index >= at->length) {
			if (at->next == NULL) break;

			relative_index = 0;
			at = at->next;
			text = filebuf_get_text(fb, at);
		}

		char c = text[relative_index];
		if (c == '\n') return;
		draw_char(c);
		terminal_cursor_right(1);
	}
}

/* Draws the line and column numbers for the cursor position at the bottom-left of the window. */
void draw_cursor_position(struct Window *window) {
	terminal_cursor_set(window->height - 1, 0);
	terminal_clear_line();
	printf("%u,%u (%u)", window->cursor_line, window->cursor_char, window->file_index);
	terminal_cursor_set(window->cursor_line, window->cursor_char); // restore cursor position
}

inline void draw_set_char_color(int color) {
	printf("\033[0;31m");
}
