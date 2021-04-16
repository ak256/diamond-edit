/* window.h
 * author: Andrew Klinge
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "filebuf.h"

enum editor_modes {
	MODE_COMMAND,
	MODE_EDITOR
};

// editor and display data for a currently edited file and window
struct Editor {
	char *info_message; // current message being displayed on info line. NULL means no message.
	index_t file_index; // current position in file
	uint32_t cursor_line; // cursor x within terminal
	uint32_t cursor_column; // cursor y within terminal 
	uint32_t cursor_column_jump; // when moving to a line that has less columns, jump to it's last char, but save the char position here for jumping back to same char position on lines that have enough columns
	int8_t mode; // current editor mode
};

struct Window {
	struct Window *above; // nearby window
	struct Window *below;
	struct Window *left;
	struct Window *right;
	struct FileBuf filebuf;
	struct Editor editor;
	uint32_t x; // position in terminal
	uint32_t y;
	uint32_t width; // number of columns 
	uint32_t height; // number of lines
};

void window_init(struct Window *window);

void window_split_horizontally(struct Window *window, struct Window *new_window);

void window_draw_char(char c);
void window_draw_chars(struct Window *window, index_t file_index, index_t length);
void window_draw_line(struct Window *window, index_t file_index);
void window_draw_info_line(struct Window *window);

void window_set_char_color(int color);

#endif
