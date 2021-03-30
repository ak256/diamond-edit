/* filebuf.c
 * Stores and manages file text being edited.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "filebuf.h"

#define INIT_BUF_SIZE 8192 // don't go much smaller than this

/* Initializes the file buffer to empty. 
 * Should be called before using a new file buffer elsewhere.
 */
void filebuf_init(struct FileBuf *fb) {
	fb->history_count = 0;
	fb->history_index = 0;
	fb->history_size = INIT_BUF_SIZE;
	fb->history = malloc(sizeof(struct FileEvent) * fb->history_size);
	fb->file_index = 0;
	fb->file_length = 0;
	fb->path = NULL;

	struct PieceTable table;
	table.origin_buf = NULL;
	table.origin_buf_size = 0;
	table.append_buf = malloc(sizeof(char) * INIT_BUF_SIZE);
	table.append_buf_size = INIT_BUF_SIZE;
	table.append_buf_count = 0;
	table.entries_count = 1;
	table.entries_size = INIT_BUF_SIZE;
	table.entries = malloc(sizeof(struct PieceTableEntry) * table.entries_size);

	struct PieceTableEntry *first_entry = &table.entries[0];
	first_entry->prev = NULL;
	first_entry->next = NULL;
	first_entry->start = 0;
	first_entry->length = 0;
	first_entry->deletion_length = 0;
	first_entry->buf_id = BUF_ID_ORIGIN;

	table.first_entry = first_entry;
	table.current_entry = first_entry;
	fb->table = table;
}

/* Gets the next memory location for a new event in the given file buffer. */
static struct FileEvent *next_event(struct FileBuf *fb) {
	if (fb->history_index == fb->history_count) {
		if (fb->history_count == fb->history_size) {
			fb->history_size *= 2;
			fb->history = realloc(fb->history, sizeof(struct FileEvent) * fb->history_size);
		}
		fb->history_count++;
	}

	struct FileEvent *event = &fb->history[fb->history_index];
	fb->history_index++;
	return event;
}

/* Gets the next memory location for a new entry in the given table. */
static struct PieceTableEntry *next_entry(struct PieceTable *table) {
	if (table->entries_count == table->entries_size) {
		table->entries_size *= 2;
		table->entries = realloc(table->entries, sizeof(struct PieceTableEntry) * table->entries_size);
	}

	struct PieceTableEntry *entry = table->entries[table->entries_count];
	table->entries_count++;
	return entry;
}

/* Inserts the second entry into the linked list of entries after the first entry. */
static inline void link_entry_after(struct PieceTableEntry *entry, struct PieceTableEntry *insert) {
	if (entry->next != NULL) {
		entry->next->prev = insert;
		insert->next = entry->next;
	}
	entry->next = insert;
	insert->prev = entry;
}

/* Removes the entry from the linked list of entries. */
static inline void unlink_entry(struct PieceTableEntry *entry) {
	if (entry->next != NULL) {
		entry->next->prev = entry->prev;
	}
	entry->prev->next = entry->next;
}

/* Erases all current redo history. */
static void erase_redo_history(struct Filebuf *fb) {
	// erase any redo history
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

/* Retrieves the character at the actual character index in the file. */
char filebuf_char_at(struct FileBuf *fb, index_t file_index) {
	// TODO
}

/* Retrieves the piece table entry at teh given actual character index in the file. */
struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index) {
	// TODO
}

/* Adds a character to the FileBuf. */
void filebuf_insert(struct FileBuf *fb, char c) {
	if (fb->append_buf_count >= fb->append_buf_size) {
		fb->append_buf_size *= 2;
		fb->append_buf = realloc(fb->append_buf, sizeof(char) * fb->append_buf_size);
	}
	fb->append_buf[fb->append_buf_count] = c;
	fb->append_buf_count++;
}

/* Finalizes a series of character insertions by modifying the PieceTable
 * of the FileBuf.
 */
void filebuf_finish_insert(struct FileBuf *fb, index_t file_index, index_t buf_index, index_t insert_length, index_t delete_length) {
	struct FileEvent *event = next_event(fb);
	struct PieceTableEntry *at = filebuf_entry_at(fb, file_index);

	if (delete_length > 0) {
		if (insert_length > 0) {
			event->id = FILE_EVENT_DELETE_THEN_ADD;
			struct PieceTableEntry *entry = next_entry(&fb->table);
			entry->start = buf_index;
			entry->length = insert_length;
			entry->delete_length = delete_length;
			entry->buf_id = BUF_ID_APPEND;
			event->entry = entry;
			link_entry_after(at, entry);
		} else {
			event->id = FILE_EVENT_DELETE;
			event->entry = at;
		}
		at->length -= delete_length;
	} else if (at->buf_id == BUF_ID_APPEND) {
		event->id = FILE_EVENT_APPEND;
		event->entry = at;
		at->length += insert_length;
	} else {
		// TODO
	}

	erase_redo_history(fb);
}

/* Undoes the last performed action on the file. */
void filebuf_undo(struct FileBuf *fb) {
	if (fb->history_index == 0) return;
	fb->history_index--;

	struct FileEvent *undone_event = &fb->history[fb->history_index];
	switch (undone_event->id) {
	case FILE_EVENT_DELETE_THEN_APPEND:
		unlink_entry(undone_event->entry);
		// fall through to FILE_EVENT_DELETE
	case FILE_EVENT_DELETE:
		undone_event->entry->prev->length += undone_event->entry->delete_length;
		break;
	case FILE_EVENT_APPEND:
		unlink_entry(undone_event->entry);
		break;
	}
}

/* Redoes the last undone performed action on the file. */
void filebuf_redo(struct FileBuf *fb) {
	if (fb->history_index >= fb->history_count) return;
	fb->history_index++;
	// FIXME doesn't handle entries (which may be split or changed based on actions)
}

/* Attempts to load the entire file at the path into the buffer.
 * FIXME: reads entire file into memory (not good for large files e.g. 1GB). should have a limit and read in parts of the file at a time as needed.
 * Returns whether successful.
 */
bool filebuf_load(struct FileBuf *fb, char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) return false;
	fb->path = path;

	struct stat filestat;
	fstat(file, &filestat);
	fb->table.origin_buf_size = filestat.st_size * 2;
	fb->table.origin_buf = malloc(sizeof(char) * fb->origin_buf_size);

	char c;
	int i = 0;
	while ((c = getchar()) != EOF) {
		fb->buf[i] = c;
		i++;
	}
	fclose(file);
	return true;
}
