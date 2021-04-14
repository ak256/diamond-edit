/* terminal.h
 * author: Andrew Klinge
 */

#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include <stdint.h>

void terminal_init();
void terminal_clear();
void terminal_clear_line();
void terminal_clear_line_from_cursor();
void terminal_cursor_home();
void terminal_cursor_set(uint32_t line, uint32_t column);
void terminal_cursor_set_line(uint32_t line);
void terminal_cursor_set_column(uint32_t column);
void terminal_cursor_up(uint32_t n);
void terminal_cursor_down(uint32_t n);
void terminal_cursor_right(uint32_t n);
void terminal_cursor_left(uint32_t n);

bool terminal_get_size(uint32_t *cols, uint32_t *rows);

#endif
