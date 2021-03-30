/* filebuf.h
 * author: Andrew Klinge
 */

#ifndef __FILEBUF_H__
#define __FILEBUF_H__

#include <stdio.h>
#include <stdbool.h>

typedef uint32_t index_t;

#define BUF_ID_ORIGIN 0
#define BUF_ID_APPEND 1
#define BUF_ID_DELETE 2

enum file_event_ids {
	FILE_EVENT_DELETE,
	FILE_EVENT_DELETE_THEN_APPEND,
	FILE_EVENT_APPEND;
};

struct PieceTableEntry {
	struct PieceTableEntry *prev;
	struct PieceTableEntry *next;
	index_t start;
	index_t length;
	index_t deletion_length;
	int8_t buf_id;
	HistoryEvent event;
};

struct PieceTable {
	// keep origin before append buf. see 'start' field of PieceTableRow
	char *origin_buf;
	char *append_buf;
	struct PieceTableEntry *first_entry; // entries are linked together
	struct PieceTableEntry *current_entry; // most recently used entry for fast access
	struct PieceTableEntry *entries; // memory for each entry
	uint32_t entries_count;
	uint32_t entries_size;
	uint32_t append_buf_count;
	uint32_t append_buf_size;
	uint32_t origin_buf_size;
};

// an action performed in modifying the piece table, stored in history for undo/redo
struct FileEvent {
	struct PieceTableEntry *entry; // entry modified or created during event
	int id; // see file_event_ids enum
};

struct FileBuf {
	struct FileEvent *history; // for undo/redo
	char *path;
	struct PieceTable table;
	uint32_t history_size;	// how much memory for history
	uint32_t history_count; // how much history
	uint32_t history_index; // where to modify history
	index_t file_index;
	index_t file_length;
};

void filebuf_init(struct FileBuf *fb, char *path);
void filebuf_insert(struct FileBuf *fb, char c);
void filebuf_finish_insert(struct FileBuf *fb, index_t file_index, index_t buf_index, index_t insert_length, index_t delete_length);
void filebuf_undo(struct FileBuf *fb);
void filebuf_redo(struct FileBuf *fb);

char filebuf_char_at(struct FileBuf *fb, index_t file_index);
struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index);

bool filebuf_load(struct FileBuf *buf, char *path);
bool filebuf_write(struct FileBuf *buf);

#endif
