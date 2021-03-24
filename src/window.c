/* window.c
 * A single file editor view.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include "window.h"

void window_init(struct Window *window) {
	window->above = NULL;
	window->below = NULL;
	window->left = NULL;
	window->right = NULL;
	window->x = 0;
	window->y = 0;
	window->cursor_line = 0;
	window->cursor_char = 0;
	window->w = 1.0f;
	window->h = 1.0f;
}

/* Splits the window evenly and adds a new window below the current window.
 * Returns pointer to the new window (which will be from window.subwindows array).
 * 
 * window - the window to split
 * new_window - pointer to where new window created by split should be stored
 */
void window_split_horizontally(struct Window *window, struct Window *new_window) {
	window_init(new_window);
	new_window->above = window;
	new_window->below = window->below;
	new_window->right = window->right;
	new_window->left = window->left;
	window->below = new_window;

	float h = window->h / (window->h + 1);
	new_window->h = h;

	struct Window *above = window;
	while (above != NULL) {
		above->h = h;
		above = above->above;
	}
	struct Window *below = new_window->below;
	while (below != NULL) {
		below->h = h;
		below = below->below;
	}
}
