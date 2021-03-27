/* filebuf.h
 * author: Andrew Klinge
 */

#ifndef __FILEBUF_H__
#define __FILEBUF_H__

#include <stdio.h>
#include <stdbool.h>

#define BUF_ID_ORIGIN 0
#define BUF_ID_APPEND 1
#define BUF_ID_DELETE 2

typedef uint32_t index_t;

struct PieceTableEntry {
	struct PieceTableEntry *prev;
	struct PieceTableEntry *next;
	index_t start;
	index_t length;
	int8_t buf_id;
	bool undone;
};

struct PieceTable {
	// keep origin before append buf. see 'start' field of PieceTableRow
	char *origin_buf;
	char *append_buf;
	struct PieceTableEntry *first_entry; // entries are linked together
	struct PieceTableEntry *current_entry; // most recently used entry for fast access
	uint32_t append_buf_count;
	uint32_t append_buf_size;
	uint32_t origin_buf_size;
	uint32_t entries_count;
};

struct FileBuf {
	PieceTableEntry *history; // for undo/redo
	char *path;
	struct PieceTable table;
	uint32_t history_size;
	uint32_t history_count;
	uint32_t history_index;
};

void filebuf_init_empty(struct FileBuf *fb, char *path);
void filebuf_insert(struct FileBuf *fb, index_t index, char *string, int string_length);
void filebuf_undo(struct FileBuf *fb);
void filebuf_redo(struct FileBuf *fb);

bool filebuf_load(struct FileBuf *buf, char *path);
bool filebuf_write(struct FileBuf *buf);

#endif
