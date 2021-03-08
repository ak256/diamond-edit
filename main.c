/* main.c
 * author: Andrew Klinge
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "window.h"
#include "terminal.h"

void interrupt_handler(int sig) {
	signal(sig, SIG_IGN);
	printf("Are you sure you want to quit? [y/n] ");
	char c = getchar();
	if (c == 'y' || c == 'Y') {
		exit(0);
	} else {
		signal(SIGINT, &interrupt_handler);
	}
}

int main() {
	struct Window root;
	window_init(&root);
	struct Window *current = &root;
	
	terminal_init();
	terminal_cursor_home();
	signal(SIGINT, &interrupt_handler);

	char c;
	while ((c = getchar()) != EOF) {
		printf("%c", c);
	}

	return 0;
}
