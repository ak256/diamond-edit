/* main.c
 * author: Andrew Klinge
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "window.h"
#include "draw.h"
#include "terminal.h"
#include "filebuf.h"

enum editor_modes {
	MODE_COMMAND,
	MODE_EDITOR
};

// current editor mode
static int mode = MODE_COMMAND;

static void interrupt_handler(int sig) {
	signal(sig, SIG_IGN);
	printf("Are you sure you want to quit? [y/n] ");
	char c = getchar();
	if (c == 'y' || c == 'Y') {
		exit(EXIT_SUCCESS);
	} else {
		signal(SIGINT, &interrupt_handler);
	}
}

int main(int arg_count, char **args) {
	struct Window root;
	window_init(&root);
	terminal_get_size(&root.width, &root.height);

	struct Window *current = &root;
	filebuf_init(&current->filebuf);
	if (arg_count > 2) {
		fprintf(stderr, "Opening multiple files at once not supported yet\n");
		exit(EXIT_FAILURE);
	} else if (arg_count == 2) {
		filebuf_read(&current->filebuf, args[1]);
		current->filebuf.path = args[1];
	}
	
	terminal_init();
	terminal_clear();
	terminal_cursor_home();
	draw_cursor_position(current);
	signal(SIGINT, &interrupt_handler);

	char c;
	bool selecting = false;
	const int buf_insert_text_size = 8192;
	char buf_insert_text[buf_insert_text_size]; // for typed-in characters. do NOT use when pasting text
	index_t insert_length; // also count for buf_insert_text
	index_t insert_file_index;
	index_t delete_before_length;
	index_t delete_after_length;
	while ((c = getchar()) != EOF) {
		struct FileBuf *fb = &current->filebuf; // alias

		if (mode == MODE_COMMAND) {
			switch (c) {
			case 'h': // cursor left
				if (current->cursor_column > 1) {
					terminal_cursor_left(1);
					current->cursor_column--;
					current->cursor_column_jump = current->cursor_column;
					current->file_index--;
				}
				break;
			case 'j': { // cursor down
				index_t new_line_index;
				bool found = filebuf_index_of(fb, current->file_index, (index_t) -1, "\n", &new_line_index);
				if (found) {
					terminal_cursor_down(1);
					current->cursor_line++;

					// determine line length for column number by finding the end index (marked by next new line)
					index_t line_length;
					index_t next_new_line_index;
					found = filebuf_index_of(fb, new_line_index + 1, (index_t) -1, "\n", &next_new_line_index);
					if (found) {
						line_length = next_new_line_index - new_line_index;
					} else {
						line_length = fb->length - new_line_index;
					}

					// reposition cursor column
					if (current->cursor_column_jump < line_length) {
						// inserting at same relative column index
						current->cursor_column = current->cursor_column_jump;
						current->file_index = new_line_index + current->cursor_column;
					} else {
						// inserting BEFORE the new-line char (at the end of the current line)
						current->cursor_column = line_length;
						current->file_index = next_new_line_index; // FIXME next_new_line_index may not be defined here!
					}
					terminal_cursor_set_column(current->cursor_column);
				}
				break; }
			case 'k': { // cursor up
				index_t new_line_index;
				bool found = filebuf_last_index_of(fb, 0, current->file_index, "\n", &new_line_index);
				if (found) {
					terminal_cursor_up(1);
					current->cursor_line--;

					// determine line length for column number by finding the start index (marked by next new line)
					index_t line_length;
					index_t prev_new_line_index;
					found = filebuf_last_index_of(fb, 0, new_line_index, "\n", &prev_new_line_index);
					if (found) {
						line_length = new_line_index - prev_new_line_index;
					} else {
						line_length = new_line_index;
					}

					// reposition cursor column
					if (current->cursor_column_jump < line_length) {
						// inserting at same relative column index
						current->cursor_column = current->cursor_column_jump;
						current->file_index = prev_new_line_index + current->cursor_column; // FIXME prev_new_line_index may not be defined here!
					} else {
						// inserting BEFORE the new-line char (at the end of the current line)
						current->cursor_column = line_length;
						current->file_index = new_line_index;
					}
					terminal_cursor_set_column(current->cursor_column);
				}
				break; }
			case 'l': { // cursor right
				if (current->file_index >= fb->length || filebuf_char_at(fb, current->file_index) == '\n') break; // already at end of the line

				terminal_cursor_right(1);
				current->cursor_column++;
				current->cursor_column_jump = current->cursor_column;
				current->file_index++;
				break; }
			case 'i': // FIXME
				// terminal_cursor_right(word_len);
				break;
			case 'o': // FIXME
				// terminal_cursor_left(word_len);
				break;
			case 'f': 
				mode = MODE_EDITOR;
				insert_file_index = current->file_index;
				insert_length = 0;
				delete_before_length = 0;
				delete_after_length = 0;
				break;
			}
		} else if (mode == MODE_EDITOR) {
			bool redraw = true;

			// TODO delete any currently selected text if character other than escape is inserted
			switch (c) {
			case '\b': { // backspace
				if (insert_length == 0 || insert_file_index == 0) {
					redraw = false;
					break;
				}

				// can't move cursor unless deleting or typing in editor mode,
				// so we only have to worry about counting number of characters deleted via backspace
				if (insert_length > 0) {
					insert_length--;
					if (buf_insert_text[insert_length] == '\n') {
						current->cursor_line--;
					} else {
						current->cursor_column--;
					}
				} else if (insert_file_index > 0) {
					delete_before_length++;
					if (filebuf_char_at(fb, insert_file_index - delete_before_length) == '\n') {
						current->cursor_line--;
					} else {
						current->cursor_column--;
					}
				}
				current->file_index--;
				terminal_cursor_left(delete_before_length);
				draw_char(' ');
				break; }
			case 127: { // delete
				index_t end_index = insert_file_index - (delete_before_length + delete_after_length) + insert_length;
				if (end_index < fb->length) {
					delete_after_length++;
				} else {
					redraw = false;
				}
				break; }
			case '\033': // escape
				filebuf_insert(fb, buf_insert_text, insert_file_index, insert_length, delete_before_length, delete_after_length);
				mode = MODE_COMMAND; 
				redraw = false;
				current->cursor_column_jump = current->cursor_column;
				break;
			default: 
				if (insert_length >= buf_insert_text_size) {
					// TODO probably just want to push the full buffer using filebuf_insert,
					// clear it, then continue inserting anew.
					// (rather than breaking here and preventing additional text entry)
					break;
				}
				draw_char(c);
				buf_insert_text[insert_length] = c;
				insert_length++;
				current->file_index++;
				if (c == '\n') {
					current->cursor_line++;
					current->cursor_column = 1;
				} else {
					current->cursor_column++;
				}
				break;
			}
			if (redraw) {
				draw_line(current, current->file_index + delete_after_length);
			}
		}
		draw_cursor_position(current);
	}
	return EXIT_SUCCESS;
}
