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
	MODE_INSERT
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
		if (!loaded) filebuf_init_empty(&current.filebuf, args[1]);
	} else {
		filebuf_init_empty(&current.filebuf, NULL);
	}
	
	terminal_init();
	terminal_cursor_home();
	signal(SIGINT, &interrupt_handler);

	int insert_index;
	int insert_length;
	bool selecting = false;
	char c;
	while ((c = getchar()) != EOF) {
		if (mode == MODE_COMMAND) {
			switch (c) {
			case 'h': terminal_cursor_left(1); break;
			case 'j': terminal_cursor_down(1); break;
			case 'k': terminal_cursor_up(1); break;
			case 'l': terminal_cursor_right(1); break;
			case 'i': {
				// TODO
				terminal_cursor_right(word_len);
				break; }
			case 'o': {
				// TODO
				terminal_cursor_left(word_len);
				break; }
			case 'f': 
				mode = MODE_INSERT;
				insert_index = current->cursor_index;
				insert_length = 0;
				break;
			}
		} else if (mode == MODE_INSERT) {
			switch (c) {
			case '\b': break; // TODO backspace
			case '':
				filebuf_finish_insert(&current.filebuf, insert_index, insert_length);
				mode = MODE_COMMAND; 
				break;
			default: 
				filebuf_insert(&current.filebuf, c);
				insert_length++;
				break;
			}
		}
	}
	return EXIT_SUCCESS;
}
