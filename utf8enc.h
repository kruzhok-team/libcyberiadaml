/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The string encoder header
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

#ifndef __CYBERIADA_UTF8ENC_H
#define __CYBERIADA_UTF8ENC_H

#ifdef __cplusplus
extern "C" {
#endif

	char* utf8_encode(const char *data, size_t input_len, size_t *output_len);
	char* utf8_decode(const char *data, size_t input_len, size_t *output_len);

#ifdef __cplusplus
}
#endif

#endif
