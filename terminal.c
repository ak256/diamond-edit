/* terminal.c
 * author: Andrew Klinge
 * 
 * Functions for drawing to the terminal window.
 */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "terminal.h"

static struct termios terminal;

void terminal_init() {
	// disable automatic echoing of input characters to terminal and let us read them as they are typed (not waiting for user to press enter)
	tcgetattr(STDIN_FILENO, &terminal);
	terminal.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
}

void terminal_clear() {
	printf("[J");
}

void terminal_cursor_home() {
	printf("[H");
}

void terminal_cursor_set(int line, int column) {
	printf("[%i;%iH", line, column);
}

void terminal_cursor_set_line(int line) {
	printf("[%i;0H", line);
}

void terminal_cursor_set_column(int column) {
	printf("[%iG", column);
}

void terminal_cursor_up(int n) {
	printf("[%iA", n);
}

void terminal_cursor_down(int n) {
	printf("[%iB", n);
}

void terminal_cursor_right(int n) {
	printf("[%iC", n);
}

void terminal_cursor_left(int n) {
	printf("[%iD", n);
}
