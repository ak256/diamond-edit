/* draw.h
 * Handles drawing characters to terminal window.
 * author: Andrew Klinge
 */

#ifndef __DRAW_H__
#define __DRAW_H__

#include "filebuf.h"

void draw_chars(struct PieceTableEntry *entry, int line, int character, index_t n);
void draw_line(struct Window *window, index_t i);

#endif
