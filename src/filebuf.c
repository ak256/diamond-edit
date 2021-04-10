/* filebuf.c
 * Memory for a single file being edited (file buffer).
 * Uses a Piece Table data structure to store edits (a quick overview: https://www.averylaird.com/programming/the%20text%20editor/2017/09/30/the-piece-table/).
 *
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>

#include "filebuf.h"
#include "config.h"

#define INIT_BUF_SIZE 8192 // don't go much smaller than this

static struct FileEvent *next_event(struct FileBuf *fb);
static struct PieceTableEntry *next_entry(struct PieceTable *table);

static void delete_entry(struct PieceTable *table, struct PieceTableEntry *entry);
static void filebuf_defragment(struct FileBuf *fb);
static void erase_redo_history(struct FileBuf *fb);

static inline void link_entry_before(struct PieceTableEntry *ref, struct PieceTableEntry *entry);
static inline void link_entry_after(struct PieceTableEntry *ref, struct PieceTableEntry *entry);
static inline void unlink_entry(struct PieceTableEntry *entry);

/* Initializes the file buffer to empty. 
 * Should be called before using a new file buffer elsewhere.
 */
void filebuf_init(struct FileBuf *fb) {
	fb->history_count = 0;
	fb->history_index = 0;
	fb->history_size = INIT_BUF_SIZE;
	fb->history = malloc(sizeof(struct FileEvent) * fb->history_size);
	fb->path = NULL;

	struct PieceTable table;
	table.origin_buf = NULL;
	table.origin_buf_size = 0;
	table.modify_buf = malloc(sizeof(char) * INIT_BUF_SIZE);
	table.modify_buf_size = INIT_BUF_SIZE;
	table.modify_buf_count = 0;
	table.entries_count = 1;
	table.entries_size = INIT_BUF_SIZE;
	table.entries = malloc(sizeof(struct PieceTableEntry) * table.entries_size);

	struct PieceTableEntry *first_entry = next_entry(&table);
	first_entry->prev = NULL;
	first_entry->next = NULL;
	first_entry->start = 0;
	first_entry->length = 0;
	first_entry->buf_id = BUF_ID_ORIGIN;
	first_entry->saved_to_file = true;

	table.free_entries = NULL;
	table.first_entry = first_entry;
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

/* Gets the next memory location for a new entry in the given table.
 * WARNING: memory is not cleared, so make sure to fully initialize.
 */
static struct PieceTableEntry *next_entry(struct PieceTable *table) {
	if (table->free_entries != NULL) {
		struct PieceTableEntry *entry = table->free_entries;
		table->free_entries = entry->next;
		return entry;
	}

	if (table->entries_count == table->entries_size) {
		table->entries_size *= 2;
		table->entries = realloc(table->entries, sizeof(struct PieceTableEntry) * table->entries_size);
	}

	struct PieceTableEntry *entry = &table->entries[table->entries_count];
	table->entries_count++;
	return entry;
}

/* Marks the memory within the table used by the entry as free. 
 * entry - MUST be a pointer to memory within table->entries
 */
static void delete_entry(struct PieceTable *table, struct PieceTableEntry *entry) {
	entry->next = NULL;
	if (table->free_entries == NULL) {
		table->free_entries = entry;
	} else {
		table->free_entries->next = entry;
		entry->prev = table->free_entries;
	}
}

/* Inserts the entry (2nd arg) into the linked list of entries before the reference entry (1st arg). 
 * Entry cannot already be in the list, call unlink_entry() first before re-inserting.
 */
static inline void link_entry_before(struct PieceTableEntry *ref, struct PieceTableEntry *entry) {
	if (ref->prev != NULL) {
		ref->prev->next = entry;
	}
	entry->prev = ref->prev;
	ref->prev = entry;
	entry->next = ref;
}

/* Inserts the entry (2nd arg) into the linked list of entries after the reference entry (1st arg).
 * Entry cannot already be in the list, call unlink_entry() first before re-inserting.
 */
static inline void link_entry_after(struct PieceTableEntry *ref, struct PieceTableEntry *entry) {
	if (ref->next != NULL) {
		ref->next->prev = entry;
	}
	entry->next = ref->next;
	ref->next = entry;
	entry->prev = ref;
}

/* Removes the entry from the linked list of entries. */
static inline void unlink_entry(struct PieceTableEntry *entry) {
	if (entry->next != NULL) {
		entry->next->prev = entry->prev;
	}
	entry->prev->next = entry->next;
}

/* Attempts to compact memory used by the file buffer, merging entries where possible and removing
 * any redundancies and unnecessary memory usages.
 */
static void filebuf_defragment(struct FileBuf *fb) {
	// TODO
	// detect and merge consecutive modify_buf entries (originally from splitting an entry in two)
}

/* Erases all current redo history. */
static void erase_redo_history(struct FileBuf *fb) {
	if (fb->history_index >= fb->history_count) return; // nothing to erase

	for (int i = fb->history_index; i < fb->history_count - 1; i++) {
		delete_entry(&fb->table, fb->history[i].entry);
	}
	delete_entry(&fb->table, fb->history[fb->history_count - 1].entry);
	fb->history_count = fb->history_index;

	// clean up entries that may be fragmented unnecessarily due to undos
	filebuf_defragment(fb);
}

/* Retrieves the piece table entry at the given actual character index in the file.
 *
 * relative_index - this function stores the relative index within the entry that the file index corresponds with
 */
struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index, index_t *relative_index) {
	struct PieceTableEntry *at = fb->table.first_entry;
	index_t i = 0;
	while (i < file_index) {
		i += at->length;
		if (at->next == NULL) break;

		at = at->next;
	}
	*relative_index = i - file_index;
	return at;
}

/* Modifies the piece table by inserting a new entry (or by modifying existing ones).
 *
 * inserted_text - a buffer containing the text to be inserted into the file at insert_index
 * insert_index - index in the file where the user began editing
 * insert_length - number of characters in inserted_text. may be zero if delete-indices are non-zero
 * delete_before_length - number of characters before the insert_index that were deleted (via backspace)
 * delete_after_length - number of characters after the insert_index that were deleted (via delete)
 */
void filebuf_insert(struct FileBuf *fb, char *inserted_text, index_t insert_index, index_t insert_length, index_t delete_before_length, index_t delete_after_length) {
	// add text to modify_buf
	struct PieceTable *table = &fb->table; // alias
	const index_t insert_buf_index = table->modify_buf_count;
	table->modify_buf_count += insert_length;
	if (table->modify_buf_count >= table->modify_buf_size) {
		table->modify_buf_size = table->modify_buf_count * 2;
		table->modify_buf = realloc(table->modify_buf, sizeof(char) * table->modify_buf_size);
	}
	for (index_t i = 0; i < insert_length; i++) {
		table->modify_buf[insert_buf_index + i] = inserted_text[i];
	}

	// add change to history
	struct FileEvent *event = next_event(fb);
	event->insert_length = insert_length;
	event->delete_before_length = delete_before_length;
	event->delete_after_length = delete_after_length;
	event->entry = NULL;

	// add change to piece table
	index_t relative_index;
	struct PieceTableEntry *at = filebuf_entry_at(fb, insert_index, &relative_index);
	if (relative_index == at->start) {
		// insert before the entry, possibly modifying the previous one.
		// at may be NULL after this, but this is handled properly.
		at = at->prev;
	} else {
		// inserting in middle of the entry, split it into two.
		struct PieceTableEntry *right_entry = next_entry(&fb->table);
		right_entry->start = relative_index;
		right_entry->length = at->length - (relative_index - at->start);
		right_entry->buf_id = BUF_ID_MODIFY;
		right_entry->saved_to_file = false;
		link_entry_after(at, right_entry);

		// make this entry the left side of the split
		at->length = relative_index - at->start;

		// these won't be re-merged upon undo, which is fine in case of redos,
		// but might start taking up memory since splitting entries will likely occur
		// most of the time an insert is done. this will create unnecessary memory usage.
		// so, this issue is handled by calling filebuf_defragment() upon erase_redo_history().
	}

	// FIXME @@@ HERE @@@ // make sure to update entry->saved_to_file
	if (delete_before_length > 0 || delete_after_length > 0) {
		if (insert_length > 0) {
			event->id = FILE_EVENT_DELETE_THEN_ADD;
			struct PieceTableEntry *entry = next_entry(&fb->table);
			entry->start = insert_buf_index;
			entry->length = insert_length;
			entry->buf_id = BUF_ID_MODIFY;
			entry->saved_to_file = false;
			event->entry = entry;
			link_entry_after(at, entry);
		} else {
			event->id = FILE_EVENT_DELETE;
			event->entry = at;
		}
		at->length -= delete_length;

		if (at->length == 0) {
			// delete this entry. but how to undo this as well, if entry memory overwritten?
			// FIXME
		} else if (at->length < 0) {
			// deleting into the next left entry!
			// FIXME
		}
	} else if (at->buf_id == BUF_ID_MODIFY) {
		event->id = FILE_EVENT_APPEND;
		event->entry = at;
		event->data = insert_length;
		at->length += insert_length;
	} else {
		event->id = FILE_EVENT_ADD;
		event->data = insert_length;
		struct PieceTableEntry *entry = next_entry(&fb->table);
		entry->start = insert_buf_index;
		entry->length = insert_length;
		entry->buf_id = BUF_ID_MODIFY;
		entry->saved_to_file = false;
		event->entry = entry;
		link_entry_after(at, entry);
	}

	fb->length = fb->length - (delete_before_length + delete_after_length) + insert_length;
	erase_redo_history(fb);
}

/* Undoes the last performed action on the file. */
void filebuf_undo(struct FileBuf *fb) {
	if (fb->history_index == 0) return;
	fb->history_index--;

	struct FileEvent *undone_event = &fb->history[fb->history_index];
	switch (undone_event->id) {
	case FILE_EVENT_DELETE_THEN_ADD:
		// FIXME see above related fixme's
		unlink_entry(undone_event->entry);
		undone_event->entry->prev->length += undone_event->data;
		break;
	case FILE_EVENT_DELETE:
		// FIXME see above related fixme's
		undone_event->entry->prev->length += undone_event->data;
		break;
	case FILE_EVENT_ADD:
		// TODO
		break;
	case FILE_EVENT_APPEND:
		undone_event->entry->length -= undone_event->data;
		break;
	}

	// TODO update fb->length
	// TODO update entry->saved_to_file
}

/* Redoes the last undone performed action on the file. */
void filebuf_redo(struct FileBuf *fb) {
	if (fb->history_index >= fb->history_count) return;
	fb->history_index++;
	// TODO update fb->length
	// TODO update entry->saved_to_file
}

/* Returns the character buffer that the given entry uses. */
char *filebuf_get_buffer(struct FileBuf *fb, struct PieceTableEntry *entry) {
	switch (entry->buf_id) {
	case BUF_ID_ORIGIN: return fb->table.origin_buf;
	case BUF_ID_MODIFY: return fb->table.modify_buf;
	default:
		fprintf(stderr, "char buffer ID %i is invalid!\n", entry->buf_id);
		exit(EXIT_FAILURE);
	}
}

/* Returns a pointer to the beginning of the entry's text in the file.
 * WARNING: entry texts are NOT separated by null terms! You must bound the text by the entry's length!
 */
char *filebuf_get_text(struct FileBuf *fb, struct PieceTableEntry *entry) {
	char *buf = filebuf_get_buffer(fb, entry);
	return buf + entry->start;
}

/* fseek() from <stdio.h> but with index_t offset instead of long offset. 
 * reference - must be either SEEK_CUR or SEEK_END
 */
static inline void fseeku(FILE *stream, index_t offset, int reference) {
	#if ARCH_BITS == 32
		if (offset > LONG_MAX) {
			fseek(stream, LONG_MAX, reference);
			fseek(stream, (long) (offset - LONG_MAX), reference);
		} else {
			fseek(stream, (long) offset, reference);
		}
	#elif ARCH_BITS == 64
		fseek(stream, offset, reference);
	#else
		#error ARCH_BITS bit architecture is not supported!
	#endif
}

/* Attempts to write the buffer to file at the filebuf's path.
 * Returns whether successful.
 */
bool filebuf_write(struct FileBuf *fb) {
	FILE *file = fopen(path, "r+"); // update file rather than rewriting it entirely on every save
	if (file == NULL) {
		file = fopen(path, "w"); // file doesn't exist? try just writing to it
		if (file == NULL) return false;
	}
	
	struct PieceTableEntry *at = fb->table.first_entry;
	while (at != NULL) {
		if (!at->saved_to_file) {
			char *text = filebuf_get_text(fb, at);
			for (index_t i = 0; i < at->length; i++) {
				fputc(text[i], file);
			}
			at->saved_to_file = true;
		}
		fseeku(file, at->length, SEEK_CUR);
		at = at->next;
	}
	fclose(file);
	return true;
}

/* Attempts to load the entire file at the path into the buffer.
 * FIXME: reads entire file into memory (not good for large files e.g. 1GB). should have a limit and read in parts of the file at a time as needed.
 * Returns whether successful.
 */
bool filebuf_read(struct FileBuf *fb, char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) return false;
	fb->path = path;

	struct stat filestat;
	fstat(fileno(file), &filestat);
	fb->table.origin_buf_size = filestat.st_size * 2;
	fb->table.origin_buf = malloc(sizeof(char) * fb->table.origin_buf_size);
	char c;
	int count = 0;
	while ((c = getchar()) != EOF) {
		fb->table.origin_buf[count] = c;
		count++;
	}

	struct PieceTableEntry *first_entry = next_entry(&fb->table);
	first_entry->prev = NULL;
	first_entry->next = NULL;
	first_entry->start = 0;
	first_entry->length = count;
	first_entry->buf_id = BUF_ID_ORIGIN;
	first_entry->saved_to_file = true;

	fb->length = count;
	fb->table.free_entries = NULL;
	fb->table.first_entry = first_entry;
	fclose(file);
	return true;
}
