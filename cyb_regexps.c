/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Cyberiada GraphML regexps
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

#include "cyb_regexps.h"

#include "cyb_error.h"

#define CYBERIADA_ACTION_EDGE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*))?\\s*$"
#define CYBERIADA_ACTION_NODE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*)?)\\s*$"
#define CYBERIADA_ACTION_SPACES_REGEXP         "^\\s*$"
#define CYBERIADA_ACTION_LEGACY_REGEXP         "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*/"
#define CYBERIADA_ACTION_LEGACY_EDGE_REGEXP    "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*/?\\s*(\\[([^]]+)\\])?(\\s*(.*))?\\s*$"
/*#define CYBERIADA_ACTION_NEWLINE_REGEXP        "^([^\n]*(\n[ \t\r]*[^\\s])?)*\n\\s*\n(.*)?$"*/

int cyberiada_init_action_regexps(CyberiadaRegexps* regexps, int flattened)
{
	if (!regexps) {
		return CYBERIADA_BAD_PARAMETER;
	}
	regexps->flattened_regexps = flattened;
	regexps->berloga_legacy = 0;
	if (regcomp(&(regexps->edge_action_regexp), CYBERIADA_ACTION_EDGE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile edge action regexp\n");
		return CYBERIADA_ASSERT;
	}
	if (regcomp(&(regexps->node_action_regexp), CYBERIADA_ACTION_NODE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile node action regexp\n");
		return CYBERIADA_ASSERT;
	}
	if (regcomp(&(regexps->node_legacy_action_regexp), CYBERIADA_ACTION_LEGACY_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile legacy node action regexp\n");
		return CYBERIADA_ASSERT;
	}
	if (regcomp(&(regexps->edge_legacy_action_regexp), CYBERIADA_ACTION_LEGACY_EDGE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile legacy edge action regexp\n");
		return CYBERIADA_ASSERT;
	}
/*	if (regcomp(&(regexps->newline_regexp), CYBERIADA_ACTION_NEWLINE_REGEXP, REG_EXTENDED)) {
	ERROR("cannot compile new line regexp\n");
	return CYBERIADA_ASSERT;
	}*/
	if (regcomp(&(regexps->spaces_regexp), CYBERIADA_ACTION_SPACES_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile new line regexp\n");
		return CYBERIADA_ASSERT;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_free_action_regexps(CyberiadaRegexps* regexps)
{
	if (!regexps) {
		return CYBERIADA_BAD_PARAMETER;
	}
	regfree(&(regexps->edge_action_regexp));
	regfree(&(regexps->node_action_regexp));
	regfree(&(regexps->node_legacy_action_regexp));
	regfree(&(regexps->edge_legacy_action_regexp));
/*	regfree(&cyberiada_newline_regexp);*/
	regfree(&(regexps->spaces_regexp));
	return CYBERIADA_NO_ERROR;
}
