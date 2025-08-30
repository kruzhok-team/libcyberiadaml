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

#ifndef __CYBERIADA_ACTIONS_H
#define __CYBERIADA_ACTIONS_H

#include "cyberiadaml.h"
#include "cyb_regexps.h"

#ifdef __cplusplus
extern "C" {
#endif

	int cyberiaga_matchres_action_regexps(const char* text,
										  const regmatch_t* pmatch, size_t pmatch_size,
										  char** trigger, char** guard, char** behavior,
										  size_t match_trigger, size_t match_guard, size_t match_action);
	int cyberiada_decode_edge_action(const char* text, CyberiadaAction** action, CyberiadaRegexps* regexps);
	int cyberiada_add_action(const char* trigger, const char* guard, const char* behavior,
							 CyberiadaAction** action);
	
	int cyberiada_decode_state_block_action(const char* text, CyberiadaAction** action, CyberiadaRegexps* regexps);
	int cyberiada_decode_state_actions(const char* text, CyberiadaAction** actions, CyberiadaRegexps* regexps);
	int cyberiada_decode_state_actions_yed(const char* text, CyberiadaAction** actions, CyberiadaRegexps* regexps);
	int cyberiada_print_action(CyberiadaAction* action, int level);
	
	int cyberiada_check_action_doubles(CyberiadaAction* action);
	int cyberiada_join_action_doubles(CyberiadaAction** action);
	int cyberiada_remove_empty_actions(CyberiadaAction** action);

#ifdef __cplusplus
}
#endif
    
#endif
