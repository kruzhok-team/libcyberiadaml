/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The graph metainformation
 *
 * Copyright (C) 2024-2025 Alexey Fedoseev <aleksey@fedoseev.net>
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

#ifndef __CYBERIADA_META_H
#define __CYBERIADA_META_H

#include "cyberiadaml.h"
#include "cyb_regexps.h"

#ifdef __cplusplus
extern "C" {
#endif

	CyberiadaMetainformation* cyberiada_new_meta(void);
	CyberiadaMetainformation* cyberiada_copy_meta(CyberiadaMetainformation* src);
	int cyberiada_destroy_meta(CyberiadaMetainformation* meta);
	int cyberiada_add_default_meta(CyberiadaDocument* doc, const char* sm_name);	
	int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body, size_t* meta_body_len);
	int cyberiada_decode_meta(CyberiadaDocument* doc, char* metadata, CyberiadaRegexps* regexps);
	int cyberiada_print_meta(CyberiadaMetainformation* meta);
	const char* cyberiada_find_meta_string(CyberiadaMetainformation* meta, const char* name);

#ifdef __cplusplus
}
#endif
    
#endif
