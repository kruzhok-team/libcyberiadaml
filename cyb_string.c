/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The string utilities
 *
 * Copyright (C) 2024-2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cyberiadaml.h"
#include "cyb_string.h"

int cyberiada_copy_string(char** target, size_t* size, const char* source)
{
	char* target_str;
	size_t strsize;
	if (!source) {
		*target = NULL;
		*size = 0;
		return CYBERIADA_NO_ERROR;
	}
	strsize = strlen(source);  
	if (strsize > MAX_STR_LEN - 1) {
		strsize = MAX_STR_LEN - 1;
	}
	target_str = (char*)malloc(strsize + 1);
	if (!target_str) {
		return CYBERIADA_MEMORY_ERROR;
	}
	strncpy(target_str, source, strsize);
	target_str[strsize] = 0;
	*target = target_str;
	if (size) {
		*size = strsize;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_string_is_empty(const char* s)
{
	while(*s) {
		if (!isspace(*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

int cyberiada_string_trim(char* orig)
{
	char* s;
	if (!orig) return 1;
	if (!*orig) return 0;
	s = orig + strlen(orig) - 1;
	while(s >= orig) {
		if (isspace(*s)) {
			*s = 0;
			s--;
		} else {
			break;
		}
	}
	return 0;
}

int cyberiada_append_string(char** target, size_t* size, const char* source, const char* separator)
{
	char *target_str, *new_target_str;
	size_t target_size, source_size, separator_size, new_target_size;
	if (!target) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!source) {
		return CYBERIADA_NO_ERROR;
	}
	target_str = *target;
	if (!target_str) {
		return cyberiada_copy_string(target, size, source);
	}
	target_size = strlen(target_str);
	source_size = strlen(source);
	if (separator) {
		separator_size = strlen(separator);
	} else {
		separator_size = 0;
	}
	new_target_size = target_size + separator_size + source_size;
	if (new_target_size > MAX_STR_LEN - 1) {
		new_target_size = MAX_STR_LEN - 1;
	}
	new_target_str = (char*)malloc(new_target_size + 1);
	if (!new_target_str) {
		return CYBERIADA_MEMORY_ERROR;
	}
	strncpy(new_target_str, target_str, new_target_size);
	if (separator) {
		strncpy(new_target_str + target_size, separator, new_target_size - target_size);
	}
	strncpy(new_target_str + target_size + separator_size, source, new_target_size - target_size - separator_size);
	new_target_str[new_target_size] = 0;
	free(target_str);
	*target = new_target_str;
	if (size) {
		*size = new_target_size;
	}
	return CYBERIADA_NO_ERROR;
}
