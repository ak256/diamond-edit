/* draw.c
 * author: Andrew Klinge
 */

#include <string.h>

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
		//terminal_cursor_right(1);
		relative_index++;
	}
}

/* Draws a message at the bottom-most line of the window. Message will be drawn higher
 * if its length exceeds the width of the window.
 * Returns: the number of characters drawn
 * message - the message to display (valid null-terminated string)
 */
int draw_message(struct Window *window, const char *message) {
	terminal_cursor_set(window->height, 0);
	terminal_clear_line();

	int message_length = strlen(message);
	int message_height = message_length / window->width;
	if (message_height > 0) {
		terminal_cursor_up(message_height);
	}
	printf("%s", message);

	// restore user cursor position
	terminal_cursor_set(window->cursor_line, window->cursor_column); 
	return message_length;
}

/* Clears the message line of the window. */
void draw_clear_message(struct Window *window) {
	terminal_cursor_set(window->height, 0);
	terminal_clear_line();
}

/* Draws the current cursor and file position at the bottom-most line of the screen. 
 * column - the terminal column to draw starting at (default: 0)
 */
void draw_current_position(struct Window *window, uint32_t column) {
	terminal_cursor_set(window->height, column);
	terminal_clear_line_from_cursor();
	printf("%u,%u (%u)", window->cursor_line, window->cursor_column, window->file_index);

	// restore user cursor position
	terminal_cursor_set(window->cursor_line, window->cursor_column); 
}

/* Sets the color for any characters drawn to the terminal later. */
inline void draw_set_char_color(int color) {
	// FIXME only sets color to red currently
	printf("\033[0;31m");
}
