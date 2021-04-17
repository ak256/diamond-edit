/* string_builder.h
 * Structure that acts as a char buffer that can be used to quickly build strings by appending values.
 *
 * author: Andrew Klinge
 */

#ifndef __STRING_BUILDER_H__
#define __STRING_BUILDER_H__

#include <stdint.h>

// make sure to call string_builder_reset() before first using this in any procedures
struct StringBuilder {
	char *buf; // the char buffer to build a string in. (initialized by user)
	char *ptr; // current pointer into buf, to track where to append.
	uint32_t remaining; // remaining number of characters in buf that can be written to buf.
	uint32_t size; // number of available characters in buf. (initializd by user)
};

void string_builder_reset(struct StringBuilder *sb);
void string_builder_append_string(struct StringBuilder *sb, char *append);
void string_builder_append_uint32(struct StringBuilder *sb, uint32_t append);

#endif
