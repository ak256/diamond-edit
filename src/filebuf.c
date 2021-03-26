/* filebuf.c
 * Stores text being edited.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "filebuf.h"

void filebuf_init_empty(struct FileBuf *fb, char *path) {
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
	first_entry.buf_id = BUF_ID_ORIGIN;
	table.first_entry = first_entry;

	fb->table = table;
	fb->path = path;

	fb->history_count = 0;
	fb->history_index = 0;
	fb->history_size = init_buf_size;
	fb->history = malloc(sizeof(PieceTableEntry*) * fb->history_size);
}

void filebuf_backspace(struct FileBuf *fb) {
	// TODO
}

void filebuf_insert(struct FileBuf *fb, index_t index, char *string, unsigned int string_length) {
	struct PieceTableRow append_row;
	added.start = index;
	added.length = string_length;
	added.buf_id = BUF_ID_APPEND;
	
	fb->table.entries_count++;
	fb->history_count = fb->history_index + 1;
	// FIXME free deleted history piece entries
}

void filebuf_undo(struct FileBuf *fb) {
	if (fb->history_index == 0) return;
	fb->history_index--;
	// FIXME update table entries to reflect undoing
	fb->table.current_entry = fb->table.current_entry->prev;
}

void filebuf_redo(struct FileBuf *fb) {
	if (fb->history_index >= fb->history_count) return;
	// FIXME update table entries to reflect redoing
	fb->history_index++;
}

/* Attempts to load the entire file at the path into the buffer.
 * WARNING: will initialize filebuf, so it should NOT be initialized prior.
 * FIXME: does not handle large files well (e.g. 1GB). should have a limit and read in parts of the file at a time
 * Returns whether successful.
 */
bool filebuf_load(struct FileBuf *filebuf, char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) return false;
	filebuf->path = path;

	struct stat filestat;
	fstat(file, &filestat);
	filebuf->buf_size = filestat.st_size * 2;
	filebuf->buf = malloc(sizeof(char) * filebuf->buf_size);

	char c;
	int i = 0;
	while ((c = getchar()) != EOF) {
		filebuf->buf[i] = c;
		i++;
	}
	filebuf->buf_count = i;
	fclose(file);
	return true;
}
