/* filebuf.h
 * Memory for a single file being edited (file buffer).
 * Uses a Piece Table data structure to store edits (a quick overview: https://www.averylaird.com/programming/the%20text%20editor/2017/09/30/the-piece-table/).
 *
 * author: Andrew Klinge
 */

#ifndef __FILEBUF_H__
#define __FILEBUF_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t index_t; // must be an unsigned integer type

#define BUF_ID_ORIGIN false
#define BUF_ID_MODIFY true

enum file_event_ids {
	FILE_EVENT_DELETE,
	FILE_EVENT_DELETE_THEN_ADD,
	FILE_EVENT_ADD,
	FILE_EVENT_APPEND
};

struct PieceTableEntry {
	struct PieceTableEntry *prev; // prior entry in table (closer to start of file)
	struct PieceTableEntry *next; // following entry in table (closer to end of file)
	index_t start; // starting index in respective buffer identified by buf_id
	index_t length; // length in characters
	bool buf_id; // see definitions BUF_ID_*
	bool saved_to_file; // whether this entry was written to file. used to avoid rewriting already saved data
};

struct PieceTable {
	char *origin_buf;
	char *modify_buf;
	struct PieceTableEntry *first_entry; // entry at the top of the table
	struct PieceTableEntry *entries; // memory for each entry. not guaranteed to be in any order
	struct PieceTableEntry *free_entries; // pointer to head of linked list of memory in entries that has been marked freed
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
	index_t insert_length;
	index_t delete_before_length;
	index_t delete_after_length;
};

// a file buffer for editing a single file
struct FileBuf {
	struct FileEvent *history; // array for undo/redo history
	char *path; // path to the file being edited
	struct PieceTable table; // edit data
	uint32_t history_size;
	uint32_t history_count;
	uint32_t history_index; // where to modify history
	index_t length; // file length in chars
};

void filebuf_init(struct FileBuf *fb);
void filebuf_undo(struct FileBuf *fb);
void filebuf_redo(struct FileBuf *fb);
void filebuf_insert(struct FileBuf *fb, char *inserted_text, index_t insert_index, index_t insert_length, index_t delete_before_length, index_t delete_after_length);

char *filebuf_get_buffer(struct FileBuf *fb, struct PieceTableEntry *entry);
char *filebuf_get_text(struct FileBuf *fb, struct PieceTableEntry *entry);

struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index, index_t *relative_index);

char filebuf_char_at(struct FileBuf *fb, index_t file_index);

bool filebuf_index_of(struct FileBuf *fb, index_t start_index, index_t end_index, const char *string, index_t *result_index);
bool filebuf_last_index_of(struct FileBuf *fb, index_t start_index, index_t end_index, const char *string, index_t *result_index);
bool filebuf_write(struct FileBuf *buf);
bool filebuf_read(struct FileBuf *buf, char *path);

#endif
