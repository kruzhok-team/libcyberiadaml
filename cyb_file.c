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

#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "cyb_file.h"

#ifdef _WIN32
static wchar_t* cyberiada_utf8_to_wide(const char* s)
{
	wchar_t* w;
	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
	if (len <= 0) {
		return NULL;
	}
	w = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)len);
	if (w && MultiByteToWideChar(CP_UTF8, 0, s, -1, w, len) <= 0) {
		free(w);
		w = NULL;
	}
	return w;
}
#endif

FILE* cyberiada_fopen(const char* filename, const char* mode)
{
#ifdef _WIN32
	wchar_t* wname = cyberiada_utf8_to_wide(filename);
	wchar_t* wmode = cyberiada_utf8_to_wide(mode);
	if (wname && wmode) {
		FILE* f = _wfopen(wname, wmode);
		free(wname);
		free(wmode);
		return f;
	}
	free(wname);
	free(wmode);
	/* not UTF-8: a name in the ANSI code page */
#endif
	return fopen(filename, mode);
}
