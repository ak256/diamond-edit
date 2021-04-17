/* string_builder.c
 * author: Andrew Klinge
 */

#include "string_builder.h"

/* Resets the string builder to write at the beginning of its buffer. 
 * Should be called before initially using a string builder with this module's procedures.
 */
void string_builder_reset(struct StringBuilder *sb) {
	sb->ptr = sb->buf;
	sb->remaining = sb->size;
}

/* Appends the Writes the string's chars to the memory pointed to by buf. Writes no more than in buf_remaining.
 * Used for quickly building a string in a char buffer by appending.
 * Returns the pointer to the null-terminator of the newly written string in buf.
 *
 * buf - buffer to write chars into.
 * buf_remaining - a pointer to a counter to track how many characters are left in buf. Must not be NULL.
 * append - null-terminated string.
 */
void string_builder_append_string(struct StringBuilder *sb, char *append) {
	if (sb->remaining == 0) return;

	uint32_t remaining = sb->remaining - 1;
	uint32_t i;
	for (i = 0; i < remaining && append[i] != '\0'; i++) {
		sb->ptr[i] = append[i];
	}
	sb->ptr[i] = '\0';
	sb->remaining -= i;
	sb->ptr = sb->ptr + i;
}

/* See string_builder_append_string(). The same but writes an unsigned int. */
void string_builder_append_uint32(struct StringBuilder *sb, uint32_t append) {
	if (sb->remaining == 0) return;

	const uint32_t tmp_buf_size = 11;
	char tmp_buf[11]; // 0 .. 2^32 - 1 is at most 10 digits, +1 for null term
	snprintf(tmp_buf, tmp_buf_size, "%u", append);
	string_builder_append_string(sb, tmp_buf);
}
