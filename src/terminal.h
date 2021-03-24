/* terminal.h
 * author: Andrew Klinge
 */

#ifndef __TERMINAL_H__
#define __TERMINAL_H__

void terminal_init();
void terminal_clear();
void terminal_cursor_home();
void terminal_cursor_set(int line, int column);
void terminal_cursor_set_line(int line);
void terminal_cursor_set_column(int column);
void terminal_cursor_up(int n);
void terminal_cursor_down(int n);
void terminal_cursor_right(int n);
void terminal_cursor_left(int n);

#endif
