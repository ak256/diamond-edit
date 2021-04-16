/* main.c
 * author: Andrew Klinge
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "window.h"
#include "terminal.h"
#include "filebuf.h"

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
	struct Window root_window;
	window_init(&root_window);
	if (!terminal_get_size(&root_window.width, &root_window.height)) {
		fprintf(stderr, "Failed to get window size!\n");
		exit(EXIT_FAILURE);
	}

	struct Window *current_window = &root_window;
	filebuf_init(&current_window->filebuf);
	if (arg_count > 2) {
		fprintf(stderr, "Opening multiple files at once not supported yet\n");
		exit(EXIT_FAILURE);
	} else if (arg_count == 2) {
		filebuf_read(&current_window->filebuf, args[1]);
		current_window->filebuf.path = args[1];
	}
	
	terminal_init();
	terminal_clear();
	terminal_cursor_home();
	signal(SIGINT, &interrupt_handler);

	char c;
	bool selecting = false;
	const int buf_insert_text_size = 8192;
	char buf_insert_text[buf_insert_text_size]; // for typed-in characters. do NOT use when pasting text
	index_t insert_length; // also count for buf_insert_text
	index_t insert_file_index;
	index_t delete_before_length;
	index_t delete_after_length;
	while (1) {
		struct FileBuf *fb = &current_window->filebuf; // alias

		window_draw_info_line(current_window);

		if (current_window->editor.mode == MODE_COMMAND) {
			char c = getchar();
			switch (c) {
			case 'h': // cursor left
				if (current_window->editor.cursor_column > 1) {
					terminal_cursor_left(1);
					current_window->editor.cursor_column--;
					current_window->editor.cursor_column_jump = current_window->editor.cursor_column;
					current_window->editor.file_index--;
				}
				break;
			// FIXME having a weird issue where sometimes the cursor won't go down even though it can, 
			// because the index is somehow messed up so it thinks its on the last line? probably a bug in
			// both cursor up and down cases
			case 'j': { // cursor down
				index_t new_line_index;
				bool found = filebuf_index_of(fb, current_window->editor.file_index, (index_t) -1, "\n", &new_line_index);
				if (found) {
					// determine line length for column number by finding the end index (marked by next new line)
					index_t line_length;
					index_t next_new_line_index = fb->length; // default is length of file in case on last line of file
					found = filebuf_index_of(fb, new_line_index + 1, (index_t) -1, "\n", &next_new_line_index);
					if (found) {
						line_length = next_new_line_index - new_line_index;
					} else {
						line_length = fb->length - new_line_index;
					}

					// reposition cursor column
					if (current_window->editor.cursor_column_jump < line_length) {
						// inserting at same relative column index
						current_window->editor.cursor_column = current_window->editor.cursor_column_jump;
						current_window->editor.file_index = new_line_index + current_window->editor.cursor_column;
					} else {
						// inserting BEFORE the new-line char (at the end of the current line)
						current_window->editor.cursor_column = line_length;
						current_window->editor.file_index = next_new_line_index;
					}
					terminal_cursor_set_column(current_window->editor.cursor_column);

					terminal_cursor_down(1);
					current_window->editor.cursor_line++;
				}
				break; }
			case 'k': { // cursor up
				index_t new_line_index;
				bool found = filebuf_last_index_of(fb, 0, current_window->editor.file_index, "\n", &new_line_index);
				if (found) {
					// determine line length for column number by finding the start index (marked by next new line)
					index_t line_length;
					index_t prev_new_line_index = 0; // default is 0 (start of file) in case we are on the first line of the file
					found = filebuf_last_index_of(fb, 0, new_line_index, "\n", &prev_new_line_index);
					if (found) {
						line_length = new_line_index - prev_new_line_index;
					} else {
						line_length = new_line_index;
					}

					// reposition cursor column // TODO this doesn't account for line wrap
					if (current_window->editor.cursor_column_jump < line_length) {
						// inserting at same relative column index
						current_window->editor.cursor_column = current_window->editor.cursor_column_jump;
						current_window->editor.file_index = prev_new_line_index + current_window->editor.cursor_column;
					} else {
						// inserting BEFORE the new-line char (at the end of the current line)
						current_window->editor.cursor_column = line_length;
						current_window->editor.file_index = new_line_index;
					}
					terminal_cursor_set_column(current_window->editor.cursor_column);

					terminal_cursor_up(1);
					current_window->editor.cursor_line--;
				}
				break; }
			case 'l': { // cursor right
				if (current_window->editor.file_index >= fb->length || filebuf_char_at(fb, current_window->editor.file_index) == '\n') break; // already at end of the line

				terminal_cursor_right(1);
				current_window->editor.cursor_column++;
				current_window->editor.cursor_column_jump = current_window->editor.cursor_column;
				current_window->editor.file_index++;
				break; }
			case 'i': // FIXME
				// terminal_cursor_right(word_len);
				break;
			case 'o': // FIXME
				// terminal_cursor_left(word_len);
				break;
			case 'f': 
				current_window->editor.mode = MODE_EDITOR;
				insert_file_index = current_window->editor.file_index;
				insert_length = 0;
				delete_before_length = 0;
				delete_after_length = 0;
				break;
			// TODO selection
			case 'H':
				break;
			case 'J':
				break;
			case 'K':
				break;
			case 'L':
				break;
			}
		} else if (current_window->editor.mode == MODE_EDITOR) {
			// TODO delete any currently selected text if character other than escape is inserted
			bool redraw_line = true;
			char c = getchar();
			switch (c) {
			case 127:
			case '\b': { // backspace
				current_window->editor.info_message = "BACKSPACE BTN PRESSED!";
				if (insert_length == 0 || insert_file_index == 0) {
					redraw_line = false;
					break;
				}

				// can't move cursor unless deleting or typing in editor mode,
				// so we only have to worry about counting number of characters deleted via backspace
				if (insert_length > 0) {
					insert_length--;
					if (buf_insert_text[insert_length] == '\n') {
						current_window->editor.cursor_line--;
					} else {
						current_window->editor.cursor_column--;
					}
				} else if (insert_file_index > 0) {
					delete_before_length++;
					if (filebuf_char_at(fb, insert_file_index - delete_before_length) == '\n') {
						current_window->editor.cursor_line--;
					} else {
						current_window->editor.cursor_column--;
					}
				}
				current_window->editor.file_index--;
				terminal_cursor_left(1);
				window_draw_char(' ');
				terminal_cursor_left(1);
				break; }
			/*case 127: { // delete
				window_draw_message(current_window, "DELETE BTN PRESSED!");
				index_t end_index = insert_file_index - (delete_before_length + delete_after_length);
				if (end_index < fb->length + insert_length) {
					delete_after_length++;
				} else {
					redraw_line = false;
				}
				break; }*/
			case '\033': // escape
				filebuf_insert(fb, buf_insert_text, insert_file_index, insert_length, delete_before_length, delete_after_length);
				current_window->editor.info_message = NULL;
				current_window->editor.mode = MODE_COMMAND; 
				redraw_line = false;
				current_window->editor.cursor_column_jump = current_window->editor.cursor_column;
				break;
			default: 
				if (insert_length >= buf_insert_text_size) {
					// TODO probably just want to push the full buffer using filebuf_insert,
					// clear it, then continue inserting anew.
					// (rather than breaking here and preventing additional text entry)
					break;
				}

				buf_insert_text[insert_length] = c;
				insert_length++;
				current_window->editor.file_index++;
				window_draw_char(c);

				if (c == '\n') {
					current_window->editor.cursor_line++;
					current_window->editor.cursor_column = 1;
				} else {
					current_window->editor.cursor_column++;
				}
				break;
			}
			if (redraw_line) {
				window_draw_line(current_window, insert_file_index + delete_after_length);
			}
		}

		// TODO detect terminal resize and update windows accordingly
	}
	return EXIT_SUCCESS;
}
