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

	struct Window *current = &root;
	filebuf_init(&current->filebuf);
	if (arg_count > 2) {
		fprintf(stderr, "Opening multiple files at once not supported yet\n");
		exit(EXIT_FAILURE);
	} else if (arg_count == 2) {
		filebuf_load(&current->filebuf, args[1]);
		current->filebuf.path = args[1]; // if filebuf_load fails, edit empty file with same path
	}
	
	terminal_init();
	terminal_cursor_home();
	signal(SIGINT, &interrupt_handler);

	char c;
	bool selecting = false;
	const int buf_insert_text_size = 8192;
	char buf_insert_text[buf_insert_text_size]; // for typed-in characters. do NOT use when pasting text
	index_t insert_file_index;
	index_t insert_length; // also count for buf_insert_text
	index_t delete_before_length;
	index_t delete_after_length;
	while ((c = getchar()) != EOF) {
		struct FileBuf *fb = &current->filebuf; // alias

		if (mode == MODE_COMMAND) {
			switch (c) {
			case 'h':
				if (current->file_index > 0) {
					terminal_cursor_left(1);
					current->file_index--;
				}
				break;
			case 'j':
				/*for (index_t i = current->file_index; i < fb->file_length; i++) {
					// FIXME
					if (filebuf_char_at(fb, i) == '\n') {
						terminal_cursor_down(1);
						current->file_index = i;
						break;
					}
				}*/
				break;
			case 'k': // FIXME
				terminal_cursor_up(1);
				break;
			case 'l': // FIXME
				terminal_cursor_right(1);
				break;
			case 'i': // FIXME
				// terminal_cursor_right(word_len);
				break;
			case 'o': // FIXME
				// terminal_cursor_left(word_len);
				break;
			case 'f': 
				mode = MODE_EDITOR;
				insert_file_index = current->file_index;
				buf_insert_count = 0;
				insert_length = 0;
				delete_before_length = 0;
				delete_after_length = 0;
				break;
			}
		} else if (mode == MODE_EDITOR) {
			bool redraw = true;

			// TODO delete any currently selected text if character other than escape is inserted
			switch (c) {
			case '\b': // backspace
				// can't move cursor unless deleting or typing in editor mode,
				// so we only have to worry about counting number of characters deleted via backspace
				if (insert_length > 0) {
					insert_length--;
				} else if (insert_file_index > 0) {
					delete_before_length++;
				} else {
					redraw = false;
				}
				break;
			case 127: // delete
				index_t end_index = insert_file_index - (delete_before_length + delete_after_length) + insert_length;
				if (end_index < fb->length) {
					delete_after_length++;
				} else {
					redraw = false;
				}
				break;
			case '': // escape
				filebuf_insert(fb, buf_insert_text, insert_file_index, insert_length, delete_before_length, delete_after_length);
				mode = MODE_COMMAND; 
				redraw = false;
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
				break;
			}

			if (redraw) {
			 	// FIXME need to update char/line of window when moving cursor (elsewhere in code)
				// draw_chars(current, current->file_index, insert_length);
			}
		}
	}
	return EXIT_SUCCESS;
}
