/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The file access platform wrapper
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#ifndef __CYBERIADA_FILE_H
#define __CYBERIADA_FILE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* fopen of a UTF-8 file name: the narrow fopen of Windows takes the ANSI code page */
	FILE* cyberiada_fopen(const char* filename, const char* mode);

#ifdef __cplusplus
}
#endif

#endif
