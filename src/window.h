/* window.h
 * author: Andrew Klinge
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "io.h"

struct Window {
	struct Window *above; // nearby window
	struct Window *below;
	struct Window *left;
	struct Window *right;
	struct FileBuf filebuf;
	int x;
	int y;
	int cursor_line;
	int cursor_char;
	int cursor_index;
	float w; // size of window is in % of available space
	float h;
};

void window_init(struct Window *window);
void window_split_horizontally(struct Window *window, struct Window *new_window);

#endif
