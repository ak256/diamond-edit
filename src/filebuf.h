/* filebuf.h
 * author: Andrew Klinge
 */

#ifndef __FILEBUF_H__
#define __FILEBUF_H__

#include <stdio.h>
#include <stdbool.h>

struct PieceTableRow {
	int32_t start; // sign bit indicates which buffer. 0 -> origin, 1 -> append
	uint32_t length;
};

struct PieceTable {
	// keep origin before append buf. see 'start' field of PieceTableRow
	char *origin_buf;
	char *append_buf;
	struct PieceTableRow *rows;
	uint32_t append_buf_count;
	uint32_t append_buf_size;
	uint32_t rows_count;
	uint32_t rows_size;
};

struct FileBuf {
	struct PieceTable table;
	char *path;
};

void filebuf_init_empty(struct FileBuf *fb, char *path);
void filebuf_backspace(struct FileBuf *fb);
void filebuf_insert(struct FileBuf *fb, int index, char c);
void filebuf_undo(struct FileBuf *fb);
void filebuf_redo(struct FileBuf *fb);

bool filebuf_load(struct FileBuf *buf, char *path);
bool filebuf_write(struct FileBuf *buf);

#endif
