/* filebuf.c
 * Stores text being edited.
 * author: Andrew Klinge
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "filebuf.h"

void filebuf_init_empty(struct FileBuf *fb, char *path) {
	fb->buf_size = 4096 * 4;
	fb->buf = malloc(sizeof(char) * init_size);
	fb->buf_count = 0;
	fb->path = path;
}

void filebuf_backspace(struct FileBuf *fb) {
	if (fb->buf_count == 0) return;
	fb->buf_count--;
}

void filebuf_insert(struct FileBuf *fb, int index, char c) {
	// TODO
}

void filebuf_undo(struct FileBuf *fb) {
 	// TODO
}

void filebuf_redo(struct FileBuf *fb) {
	// TODO
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
