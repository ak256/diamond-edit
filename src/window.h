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
	int cursor_line; // cursor x within terminal
	int cursor_char; // cursor y within terminal 
	int x; // position in terminal
	int y;
	float w; // size of window in % of available space
	float h;
};

void window_init(struct Window *window);
void window_split_horizontally(struct Window *window, struct Window *new_window);

#endif
