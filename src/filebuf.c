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
#include <string.h>

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
	fb->path = NULL;
	fb->history_count = 0;
	fb->history_index = 0;
	fb->history_size = INIT_BUF_SIZE;
	fb->history = malloc(sizeof(struct FileEvent) * fb->history_size);
	fb->length = 0;

	struct PieceTable table;
	table.origin_buf = NULL;
	table.origin_buf_size = 0;
	table.modify_buf = malloc(sizeof(char) * INIT_BUF_SIZE);
	table.modify_buf_size = INIT_BUF_SIZE;
	table.modify_buf_count = 0;
	table.entries_count = 0;
	table.entries_size = INIT_BUF_SIZE;
	table.entries = malloc(sizeof(struct PieceTableEntry) * table.entries_size);
	table.free_entries = NULL;
	table.first_entry = NULL;
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

/* Marks the memory within the table used by the entry as free and unlinks it from the piece table.
 * entry - MUST be a pointer to memory within table->entries
 */
static void delete_entry(struct PieceTable *table, struct PieceTableEntry *entry) {
	unlink_entry(entry);
	entry->next = NULL;
	if (table->free_entries == NULL) {
		table->free_entries = entry;
		entry->prev = NULL;
	} else {
		table->free_entries->next = entry;
		entry->prev = table->free_entries;
		table->free_entries = entry;
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
 * Returns NULL if the table is empty or the index is invalid.
 *
 * file_index - must be a valid index within the file (i.e. 0 <= file_index < file length)
 * relative_index - this function stores the relative index within the entry that the file index corresponds with. if NULL, is ignored.
 */
struct PieceTableEntry *filebuf_entry_at(struct FileBuf *fb, index_t file_index, index_t *relative_index) {
	if (file_index >= fb->length) return NULL;

	struct PieceTableEntry *at = fb->table.first_entry;
	index_t i = 0;
	while (at != NULL) {
		index_t new_i = i + at->length;
		if (new_i >= file_index) {
			if (relative_index != NULL) {
				*relative_index = file_index - i;
			}
			return at;
		}
		i = new_i;
		at = at->next;
	}
	return NULL;
}

/* Splits the entry into two at the given index within the entry. 
 * Returns the new second entry that resulted from the split.
 */
static struct PieceTableEntry *split_entry(struct FileBuf *fb, struct PieceTableEntry *entry, index_t relative_split_index) {
	struct PieceTableEntry *right_entry = next_entry(&fb->table);
	right_entry->start = relative_split_index;
	right_entry->length = entry->length - (relative_split_index - entry->start);
	right_entry->buf_id = entry->buf_id;
	right_entry->saved_to_file = false;
	link_entry_after(entry, right_entry);

	// make entry the left side of the split
	entry->length = relative_split_index - entry->start;
	return right_entry;
}

/* Modifies the piece table by inserting a new entry (or by modifying existing ones).
 *
 * inserted_text - a buffer containing the text to be inserted into the file at insert_index
 * insert_index - index in the file where the user began editing
 * insert_length - number of characters in inserted_text. may be zero if delete-indices are non-zero
 * delete_before_length - number of characters before the insert_index that were deleted (via backspace)
 * delete_after_length - number of characters after the insert_index that were deleted (via delete)
 */
// TODO add undo capability. permanently deletes text currently
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

	// add change to piece table
	struct PieceTableEntry *entry = next_entry(&fb->table);
	entry->buf_id = BUF_ID_MODIFY;
	entry->start = insert_buf_index;
	entry->length = insert_length;
	entry->saved_to_file = false;
	event->entry = entry;

	index_t relative_index;
	struct PieceTableEntry *at = filebuf_entry_at(fb, insert_index, &relative_index);
	if (at == NULL) {
		fb->table.first_entry = entry;
		goto label_end_insert;
	}

	if (delete_before_length > 0) {
		index_t relative_deletion_start;
		index_t index = insert_index - delete_before_length;
		struct PieceTableEntry *deletion_end_entry = filebuf_entry_at(fb, index, &relative_deletion_start);

		// perform delete, deleting from index all the way to insert_index
		struct PieceTableEntry *current;
		if (relative_deletion_start == deletion_end_entry->start) {
			current = deletion_end_entry;
		} else {
			// save left side of split, delete right side
			current = split_entry(fb, deletion_end_entry, relative_deletion_start);
		}
		while (index < insert_index) {
			index += current->length;
			current = current->next;
			delete_entry(&fb->table, current->prev);
		}
		at = current->prev;
	} else if (relative_index == at->start) {
		at = at->prev;
	} else {
		split_entry(fb, at, relative_index);
	}

	if (delete_after_length > 0) {
		index_t relative_deletion_end;
		index_t index = insert_index + delete_after_length;
		struct PieceTableEntry *deletion_end_entry = filebuf_entry_at(fb, index, &relative_deletion_end);
		
		// perform delete, deleting from entry at insert_index all the way to index
		struct PieceTableEntry *current = at;
		if (relative_deletion_end != deletion_end_entry->start) {
			// delete left side of split, save right side
			split_entry(fb, deletion_end_entry, relative_deletion_end);
		}
		while (index > insert_index) {
			index -= current->length;
			current = current->prev;
			delete_entry(&fb->table, current->next);
		}
	}

	if (at == NULL) {
		link_entry_before(fb->table.first_entry, entry);
		fb->table.first_entry = entry;
	} else {
		link_entry_after(at, entry);
	}

label_end_insert:
	fb->length = fb->length - (delete_before_length + delete_after_length) + insert_length;
	erase_redo_history(fb);
}

/* Undoes the last performed action on the file. */
void filebuf_undo(struct FileBuf *fb) { // FIXME this doesn't work at all currently
	if (fb->history_index == 0) return;
	fb->history_index--;

	struct FileEvent *undone_event = &fb->history[fb->history_index];

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

/* Returns the character at the index in the file, or, value 0 if unable to get a character.
 * NOTE: this should not be used for iterating over the file buffer to access each character in succession and the like.
 * Doing so would be incredibly inefficient. Instead, iterate over the piece table linked list.
 */
char filebuf_char_at(struct FileBuf *fb, index_t file_index) {
	if (file_index >= fb->length) return 0;

	index_t relative_index;
	struct PieceTableEntry *at = filebuf_entry_at(fb, file_index, &relative_index);
	if (at == NULL) return (char) 0;

	char *text = filebuf_get_text(fb, at);
	return text[relative_index];
}

/* Sets 'result_index' to the file index of the first occurrence of the given character sequence,
 * search constrained between start_index (inclusive) and end_index (exclusive).
 * string - must be a valid, null-terminated string.
 * Returns whether successful. False if no occurrence found (or invalid range or empty matching string).
 */
bool filebuf_index_of(struct FileBuf *fb, index_t start_index, index_t end_index, const char *string, index_t *result_index) {
	if (end_index <= start_index || string[0] == '\0' || start_index >= fb->length) return false;

	index_t string_match_index = 0; // current position we have matched up to in the string
	index_t relative_index; // within current entry
	struct PieceTableEntry *at = filebuf_entry_at(fb, start_index, &relative_index);
	char *text = filebuf_get_text(fb, at);
	for (index_t i = start_index; i < end_index; i++) {
		if (string[string_match_index] == '\0') {
			*result_index = i;
			return true;
		}
		if (relative_index >= at->length) {
			if (at->next == NULL) return false;

			relative_index = 0;
			at = at->next;
			text = filebuf_get_text(fb, at);
		}
		if (text[relative_index] == string[string_match_index]) {
			string_match_index++;
		} else {
			string_match_index = 0;
		}
		relative_index++;
	}
	return false;
}

/* Sets 'result_index' to the file index of the last occurrence of the given character sequence,
 * search constrained between start_index (inclusive) and end_index (exclusive).
 * string - must be a valid, null-terminated string.
 * Returns whether successful. False if no occurrence found (or invalid range or empty matching string).
 */
bool filebuf_last_index_of(struct FileBuf *fb, index_t start_index, index_t end_index, const char *string, index_t *result_index) {
	if (end_index <= start_index || string[0] == '\0') return false;
	if (end_index - 1 >= fb->length) {
		end_index = fb->length;
	}

	const int string_length = strlen(string);
	index_t string_match_index = string_length - 1; // current position we have matched up to in the string
	index_t relative_index; // within current entry
	struct PieceTableEntry *at = filebuf_entry_at(fb, end_index - 1, &relative_index);
	char *text = filebuf_get_text(fb, at);
	for (index_t i = end_index - 1; i >= start_index; i--) {
		if (text[relative_index] == string[string_match_index]) {
			if (string_match_index == 0) {
				*result_index = i;
				return true;
			} else {
				string_match_index--;
			}
		} else {
			string_match_index = string_length - 1;
		}
		if (relative_index == 0) {
			if (at->prev == NULL) return false;

			at = at->prev;
			relative_index = at->length - 1;
			text = filebuf_get_text(fb, at);
		} else {
			relative_index--;
		}
	}
	return false;
}

/* Attempts to write the buffer to file at the filebuf's path.
 * Returns whether successful.
 */
bool filebuf_write(struct FileBuf *fb) {
	FILE *file = fopen(fb->path, "r+"); // update file rather than rewriting it entirely on every save
	bool write_all = false;
	if (file == NULL) {
		file = fopen(fb->path, "w"); // file doesn't exist or err? overwrite it
		write_all = true;
		if (file == NULL) return false;
	}
	
	struct PieceTableEntry *at = fb->table.first_entry;
	while (at != NULL) {
		if (write_all || !at->saved_to_file) {
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
 * fb - should be an empty, initialized FileBuf
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

/* For debugging; prints out the piece table in a readable fashion. */
void filebuf_print(struct FileBuf *fb) {
	printf("\n-----------------------\n"
		   "| ID | START | LENGTH |\n"
		   "-----------------------\n");
	struct PieceTableEntry *at = fb->table.first_entry;
	while (at != NULL) {
		printf("| %i  | %u  | %u  |\n", (int) at->buf_id, at->start, at->length);
		at = at->next;
	}
}
