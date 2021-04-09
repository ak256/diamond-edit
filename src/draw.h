/* draw.h
 * Handles drawing characters to terminal window.
 * author: Andrew Klinge
 */

#ifndef __DRAW_H__
#define __DRAW_H__

#include "window.h"
#include "filebuf.h"

void draw_chars(struct Window *window, struct PieceTableEntry *entry, index_t entry_index, index_t length);

#endif
