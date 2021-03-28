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

enum editor_modes {
	MODE_COMMAND,
	MODE_EDITOR
}

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
	if (arg_count > 2) {
		fprintf("Opening multiple files at once not supported yet\n");
		exit(EXIT_FAILURE);
	} else if (arg_count == 2) {
		bool loaded = filebuf_load(&current.filebuf, args[1]);
		if (!loaded) {
			filebuf_init_table(&current.filebuf);
			current.filebuf.path = args[1];
		}
	} else {
		filebuf_init_table(&current.filebuf);
		current.filebuf.path = NULL;
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
		struct FileBuf *fb = &current->fb; // alias

		if (mode == MODE_COMMAND) {
			switch (c) {
			case 'h':
				if (fb->file_index > 0) {
					terminal_cursor_left(1);
					fb->file_index--;
				}
				break;
			case 'j':
				for (int i = fb->file_index; i < fb->file_length; i++) {
					if (filebuf_char_at(i) == '\n') {
						terminal_cursor_down(1);
						fb->file_index = i;
						break;
					}
				}
				break;
			case 'k':
				terminal_cursor_up(1);
				break;
			case 'l':
				terminal_cursor_right(1);
				break;
			case 'i':
				// TODO
				terminal_cursor_right(word_len);
				break;
			case 'o':
				// TODO
				terminal_cursor_left(word_len);
				break;
			case 'f': 
				mode = MODE_EDITOR;
				insert_file_index = fb->file_index;
				insert_buf_index = fb->table.append_buf_count;
				insert_length = 0;
				delete_length = 0;
				break;
			}
		} else if (mode == MODE_EDITOR) {
			switch (c) {
			case '\b':
				// can't move cursor unless deleting or typing in editor mode,
				// so we only have to worry about counting number of characters deleted via backspace
				if (insert_length > 0) {
					insert_length--;
				} else {
					delete_length++;
				}
				break;
			case '':
				filebuf_finish_insert(fb, insert_file_index, insert_buf_index, insert_length, delete_length);
				mode = MODE_COMMAND; 
				break;
			default: 
				filebuf_insert(fb, c);
				insert_length++;
				break;
			}
		}
	}
	return EXIT_SUCCESS;
}
