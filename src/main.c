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

	index_t insert_buf_index;
	index_t insert_file_index;
	index_t insert_length;
	index_t delete_length;
	bool selecting = false;
	char c;
	while ((c = getchar()) != EOF) {
		struct FileBuf *fb = &current->filebuf; // alias

		if (mode == MODE_COMMAND) {
			switch (c) {
			case 'h':
				if (fb->file_index > 0) {
					terminal_cursor_left(1);
					fb->file_index--;
				}
				break;
			case 'j':
				for (index_t i = fb->file_index; i < fb->file_length; i++) {
					if (filebuf_char_at(fb, i) == '\n') {
						terminal_cursor_down(1);
						fb->file_index = i;
						break;
					}
				}
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
				insert_file_index = fb->file_index;
				insert_buf_index = fb->table.modify_buf_count;
				insert_length = 0;
				delete_length = 0;
				break;
			}
		} else if (mode == MODE_EDITOR) {
			index_t entry_index;
			struct PieceTableEntry *entry = filebuf_entry_at(fb, insert_file_index - delete_length, &entry_index);
			bool redraw = true;

			switch (c) {
			case '\b': // backspace
				// can't move cursor unless deleting or typing in editor mode,
				// so we only have to worry about counting number of characters deleted via backspace
				if (insert_length > 0) {
					insert_length--;
				} else if (insert_file_index > 0) {
					delete_length++;
				} else {
					redraw = false;
				}
				break;
			case '': // escape
				filebuf_finish_insert(fb, insert_file_index, insert_buf_index, insert_length, delete_length);
				mode = MODE_COMMAND; 
				redraw = false;
				break;
			default: 
				filebuf_insert(fb, c);
				insert_length++;
				break;
			}
			if (redraw) {
			 	// FIXME need to update char/line of window when moving cursor
				draw_chars(entry, current->cursor_line, current->cursor_char, insert_length);
			}
		}
	}
	return EXIT_SUCCESS;
}
