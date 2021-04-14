/* draw.h
 * Handles drawing characters to terminal window.
 * author: Andrew Klinge
 */

#ifndef __DRAW_H__
#define __DRAW_H__

#include "window.h"
#include "filebuf.h"

void draw_char(char c);
void draw_chars(struct Window *window, index_t file_index, index_t length);
void draw_line(struct Window *window, index_t file_index);
void draw_current_position(struct Window *window, uint32_t column);
void draw_set_char_color(int color);
void draw_clear_message(struct Window *window);

int draw_message(struct Window *window, const char *message);

#endif
