/* window.h
 * author: Andrew Klinge
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "filebuf.h"

struct Window {
	struct Window *above; // nearby window
	struct Window *below;
	struct Window *left;
	struct Window *right;
	struct FileBuf filebuf;
	index_t file_index; // current position in file
	uint32_t cursor_line; // cursor x within terminal
	uint32_t cursor_column; // cursor y within terminal 
	uint32_t cursor_column_jump; // when moving to a line that has less columns, jump to it's last char, but save the char position here for jumping back to same char position on lines that have enough columns
	uint32_t x; // position in terminal
	uint32_t y;
	uint32_t width; // number of columns 
	uint32_t height; // number of lines
};

void window_init(struct Window *window);
void window_split_horizontally(struct Window *window, struct Window *new_window);

#endif
