/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The string utilities
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
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

#ifndef __CYBERIADA_STRING_H
#define __CYBERIADA_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML string utilities
 * ----------------------------------------------------------------------------- */

    #define MAX_STR_LEN							   4096
	#define CYBERIADA_SINGLE_NEWLINE               "\n"
    #define CYBERIADA_NEWLINE                      "\n\n"
    #define CYBERIADA_NEWLINE_RN                   "\r\n\r\n"
    #define EMPTY_LINE                             ""
	
	int cyberiada_copy_string(char** target, size_t* size, const char* source);
	int cyberiada_string_is_empty(const char* s);
	int cyberiada_string_trim(char* orig);
	int cyberiada_append_string(char** target, size_t* size, const char* source, const char* separator);
	
#ifdef __cplusplus
}
#endif
    
#endif
