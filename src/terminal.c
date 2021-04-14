/* terminal.c
 * author: Andrew Klinge
 * 
 * Functions for drawing to the terminal window.
 * See https://en.wikipedia.org/wiki/ANSI_escape_code#DL
 */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#include "terminal.h"

static struct termios terminal;

void terminal_init() {
	// disable automatic echoing of input characters to terminal and let us read them as they are typed (not waiting for user to press enter)
	tcgetattr(STDIN_FILENO, &terminal);
	terminal.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
}

void terminal_clear() {
	printf("\033[2J");
}

void terminal_clear_line() {
	printf("\033[2K");
}

void terminal_clear_line_from_cursor() {
	printf("\033[0K");
}

void terminal_cursor_home() {
	printf("\033[H");
}

void terminal_cursor_set(uint32_t line, uint32_t column) {
	printf("\033[%u;%uH", line, column);
}

void terminal_cursor_set_line(uint32_t line) {
	printf("\033[%u;0H", line);
}

void terminal_cursor_set_column(uint32_t column) {
	printf("\033[%uG", column);
}

void terminal_cursor_up(uint32_t n) {
	printf("\033[%uA", n);
}

void terminal_cursor_down(uint32_t n) {
	printf("\033[%uB", n);
}

void terminal_cursor_right(uint32_t n) {
	printf("\033[%uC", n);
}

void terminal_cursor_left(uint32_t n) {
	printf("\033[%uD", n);
}

/* Attempts to get the current window size.
 * Returns whether succeeded.
 *
 * Modified from: VIM - Vi IMproved by Bram Moolenaar
 *		https://github.com/vim/vim/blob/master/src/os_unix.c
 *		int mch_get_shellsize(void);
 */
bool terminal_get_size(uint32_t *cols, uint32_t *rows) {
	// using ioctl
	// Try using TIOCGWINSZ first, some systems that have it also define
	// TIOCGSIZE but don't have a struct ttysize.
	#ifdef TIOCGWINSZ
	{
		struct winsize ws;
		int fd = 1;
		int read_cmd_fd = fileno(stdin);

		// When stdout is not a tty, use stdin for the ioctl().
		if (!isatty(fd) && isatty(read_cmd_fd)) {
			fd = read_cmd_fd;
		}
		if (ioctl(fd, TIOCGWINSZ, &ws) == 0) {
			*cols = ws.ws_col;
			*rows = ws.ws_row;
		}
		return true;
	}
	#else // TIOCGWINSZ
	#ifdef TIOCGSIZE
	{
		struct ttysize ts;
		int fd = 1;
		int read_cmd_fd = fileno(stdin);

		// When stdout is not a tty, use stdin for the ioctl().
		if (!isatty(fd) && isatty(read_cmd_fd)) {
			fd = read_cmd_fd;
		}
		if (ioctl(fd, TIOCGSIZE, &ts) == 0) {
			*cols = ts.ts_cols;
			*rows = ts.ts_lines;
		}
		return true;
	}
	#endif // TIOCGSIZE
	#endif // TIOCGWINSZ
	return false;
}
