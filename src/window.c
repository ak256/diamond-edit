/* window.c
 * A single file editor view.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "terminal.h"

void window_init(struct Window *window) {
	window->above = NULL;
	window->below = NULL;
	window->left = NULL;
	window->right = NULL;
	window->x = 0;
	window->y = 0;
	window->width = 0;
	window->height = 0;

	struct Editor editor;
	editor.mode = MODE_COMMAND;
	editor.info_message = NULL;
	editor.file_index = 0;
	editor.cursor_line = 1;
	editor.cursor_column = 1;
	window->editor = editor;
}

/* Simply draws the character to the screen at the current cursor position.
 * Does not move the cursor.
 */
inline void window_draw_char(char c) {
	printf("%c", c);
}

/* Draws 'length' number of characters starting at the index in the file and
 * at the current cursor position in the given window.
 */
void window_draw_chars(struct Window *window, index_t file_index, index_t length) {
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

		window_draw_char(text[relative_index]);
		terminal_cursor_right(1);
	}
}

/* Draws characters starting at the file index until either a new line character is
 * encountered or the end of the file is, whichever comes first.
 * This is done at the current cursor position in the given window.
 */
void window_draw_line(struct Window *window, index_t file_index) {
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
		window_draw_char(c);
		//terminal_cursor_right(1);
		relative_index++;
	}
}

/* Draws an informational line at the bottom of the window, containing
 * the current cursor position in the editor as well as any other useful
 * messages or info.
 * Line will be truncated if it exceeds the width of the window.
 */
void window_draw_info_line(struct Window *window) {
	size_t chars_remaining = window->width + 1; // +1 for null term
	char buf[chars_remaining];
	int chars_count = 0;
	int written_chars;

	// cursor position
	written_chars = snprintf(buf, chars_remaining, "%u,%u (%u)", 
		window->editor.cursor_line, window->editor.cursor_column, window->editor.file_index);
	if (written_chars > chars_remaining) goto __window_draw_info_line_cleanup__;
	chars_remaining -= written_chars;
	chars_count += written_chars;

	// current editor mode
	if (window->editor.mode == MODE_EDITOR) {
		written_chars = snprintf(buf + chars_count, chars_remaining, " [EDITING]");
		if (written_chars > chars_remaining) goto __window_draw_info_line_cleanup__;
		chars_remaining -= written_chars;
		chars_count += written_chars;
	}

	// any info message
	if (window->editor.info_message != NULL) {
		snprintf(buf + chars_count, chars_remaining, " %s", window->editor.info_message);
	}

__window_draw_info_line_cleanup__:
	// print whatever text we can display to the line and clear the rest of it
	terminal_cursor_set(window->height, 0);
	printf("%s", buf);
	terminal_clear_line_from_cursor();

	// restore user cursor position
	terminal_cursor_set(window->editor.cursor_line, window->editor.cursor_column); 
}

/* Sets the color for any characters drawn to the terminal later. */
inline void window_set_char_color(int color) {
	// FIXME only sets color to red currently
	printf("\033[0;31m");
}

/* Splits the window evenly and adds a new window below the current window.
 * Returns pointer to the new window (which will be from window.subwindows array).
 * 
 * window - the window to split
 * new_window - pointer to where new window created by split should be stored
 */
void window_split_horizontally(struct Window *window, struct Window *new_window) {
	window_init(new_window);
	new_window->above = window;
	new_window->below = window->below;
	new_window->right = window->right;
	new_window->left = window->left;
	window->below = new_window;

	// FIXME using int width/height of window
	/*
	float h = window->h / (window->h + 1);
	new_window->h = h;

	struct Window *above = window;
	while (above != NULL) {
		above->h = h;
		above = above->above;
	}
	struct Window *below = new_window->below;
	while (below != NULL) {
		below->h = h;
		below = below->below;
	}
	*/
}
