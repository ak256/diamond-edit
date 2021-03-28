/* filebuf.c
 * Stores text being edited.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "filebuf.h"

void filebuf_init(struct FileBuf *fb) {
	fb->history_count = 0;
	fb->history_index = 0;
	fb->history_size = init_buf_size;
	fb->history = malloc(sizeof(PieceTableEntry) * fb->history_size);
	fb->file_index = 0;
}

void filebuf_init_table(struct FileBuf *fb) {
	const int init_buf_size = 4096 * 2; // don't go much smaller than this

	struct PieceTable table;
	table.origin_buf = NULL;
	table.origin_buf_size = 0;
	table.append_buf = malloc(sizeof(char) * init_buf_size);
	table.append_buf_size = init_buf_size;
	table.append_buf_count = 0;
	table.entries_count = 1;

	struct PieceTableEntry first_entry;
	first_entry.prev = NULL;
	first_entry.next = NULL;
	first_entry.start = 0;
	first_entry.length = 0;
	first_entry.deletion_length = 0;
	first_entry.buf_id = BUF_ID_ORIGIN;
	table.first_entry = first_entry;
	table.current_entry = first_entry;

	fb->table = table;
}

static void erase_redo_history(struct Filebuf *fb) {
	// erase any redo history
	fb->history_index++; 
	for (int i = fb->history_index; i < fb->history_count - 1; i++) {
		struct PieceTableEntry *remove = &fb->history[i];
		remove->prev->next = remove->next;
		remove->next->prev = remove->prev;
	}
	if (fb->history_index < fb->history_count) {
		struct PieceTableEntry *remove = &fb->history[fb->history_count - 1];
		remove->prev->next = remove->next;
	}
	fb->history_count = fb->history_index;
}

char filebuf_char_at(struct FileBuf *fb, index_t file_index) {
	// TODO
}

struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index) {
	
}

void filebuf_insert(struct FileBuf *fb, char c) {
	if (fb->append_buf_count >= fb->append_buf_size) {
		fb->append_buf_size *= 2;
		fb->append_buf = realloc(fb->append_buf, sizeof(char) * fb->append_buf_size);
	}
	fb->append_buf[fb->append_buf_count] = c;
	fb->append_buf_count++;
}

void filebuf_finish_insert(struct FileBuf *fb, index_t file_index, index_t buf_index, index_t insert_length, index_t delete_length) {
	struct PieceTableEntry *at = filebuf_entry_at(fb, file_index);

	if (delete_length > 0) {
		// add hidden entry (only in history) for deletion, adjust length of current entry
		at->length -= delete_length;
		struct PieceTableEntry *deletion = &fb->history[fb->history_index];
	}

	struct PieceTableEntry *entry = &fb->history[fb->history_index];
	entry->start = buf_index;
	entry->length = insert_length;
	entry->buf_id = BUF_ID_APPEND;
	
	// insert into piece table
	struct PieceTableEntry *current = filebuf_entry_at(fb, file_index);


	while (true) {
		if (gT) {

		}
	}

	erase_redo_history(fb);
}

void filebuf_undo(struct FileBuf *fb) {
	if (fb->history_index == 0) return;
	fb->history_index--;
	// FIXME doesn't handle entries (which may be split or changed based on actions)
}

void filebuf_redo(struct FileBuf *fb) {
	if (fb->history_index >= fb->history_count) return;
	fb->history_index++;
	// FIXME doesn't handle entries (which may be split or changed based on actions)
}

/* Attempts to load the entire file at the path into the buffer.
 * WARNING: will initialize filebuf, so it should NOT be initialized prior.
 * FIXME: reads entire file into memory (not good for large files e.g. 1GB). should have a limit and read in parts of the file at a time as needed.
 * Returns whether successful.
 */
bool filebuf_load(struct FileBuf *filebuf, char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) return false;
	filebuf->path = path;

	struct stat filestat;
	fstat(file, &filestat);
	filebuf->origin_buf_size = filestat.st_size * 2;
	filebuf->origin_buf = malloc(sizeof(char) * filebuf->origin_buf_size);

	char c;
	int i = 0;
	while ((c = getchar()) != EOF) {
		filebuf->buf[i] = c;
		i++;
	}
	fclose(file);
	return true;
}
