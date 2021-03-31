/* filebuf.h
 * author: Andrew Klinge
 */

#ifndef __FILEBUF_H__
#define __FILEBUF_H__

#include <stdio.h>
#include <stdbool.h>

typedef uint32_t index_t;

#define BUF_ID_ORIGIN 0
#define BUF_ID_MODIFY 1

enum file_event_ids {
	FILE_EVENT_DELETE,
	FILE_EVENT_DELETE_THEN_ADD,
	FILE_EVENT_ADD,
	FILE_EVENT_APPEND;
};

struct PieceTableEntry {
	struct PieceTableEntry *prev;
	struct PieceTableEntry *next;
	index_t start;
	index_t length;
	int8_t buf_id;
};

struct PieceTable {
	char *origin_buf;
	char *modify_buf;
	struct PieceTableEntry *current_entry; // most recently used entry for fast access
	struct PieceTableEntry *entries; // memory for each entry. not guaranteed to be in any order
	uint32_t entries_count;
	uint32_t entries_size;
	uint32_t modify_buf_count;
	uint32_t modify_buf_size;
	uint32_t origin_buf_size;
};

// an action performed in modifying the piece table, stored in history for undo/redo
struct FileEvent {
	struct PieceTableEntry *entry; // entry modified or created during event
	int id; // see file_event_ids enum
	index_t data; // related info to event, such as length of deleted text
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
