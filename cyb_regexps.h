/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Cyberiada GraphML actions handling
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

#ifndef __CYBERIADA_REGEXPS_H
#define __CYBERIADA_REGEXPS_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct _CyberiadaRegexpsMisc;
	typedef struct _CyberiadaRegexpsMisc CyberiadaRegexpsMics;
	
	typedef struct _CyberiadaRegexps {
		int                   berloga_legacy;
		int                   flattened_regexps;
		int                   arena_legacy;
		CyberiadaRegexpsMics* r;
	} CyberiadaRegexps;

	int cyberiada_init_action_regexps(CyberiadaRegexps* regexps, int flattened);
	int cyberiada_action_regexps_spaces(CyberiadaRegexps* regexps, const char* s);
	int cyberiada_free_action_regexps(CyberiadaRegexps* regexps);
	
#ifdef __cplusplus
}
#endif
    
#endif
