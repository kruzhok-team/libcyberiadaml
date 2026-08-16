/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Cyberiada GraphML regexps
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
#include "cyb_regex.h"

#include "cyb_regexps.h"
#include "cyb_error.h"

#define CYBERIADA_ACTION_EDGE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*))?\\s*$"
#define CYBERIADA_ACTION_NODE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*)?)\\s*$"
#define CYBERIADA_ACTION_SPACES_REGEXP         "^\\s*$"
#define CYBERIADA_ACTION_LEGACY_REGEXP         "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*/"
#define CYBERIADA_ACTION_LEGACY_EDGE_REGEXP    "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*/?\\s*(\\[([^]]+)\\])?(\\s*(.*))?\\s*$"
/*#define CYBERIADA_ACTION_NEWLINE_REGEXP        "^([^\n]*(\n[ \t\r]*[^\\s])?)*\n\\s*\n(.*)?$"*/

typedef struct _CyberiadaRegexpsMisc {
	/* basic regexps */
	regex_t edge_action_regexp;
	regex_t node_action_regexp;
	regex_t node_legacy_action_regexp;
	regex_t edge_legacy_action_regexp;
	/*regex_t newline_regexp;*/
	regex_t spaces_regexp;
} CyberiadaRegexpsMics;

int cyberiada_init_action_regexps(CyberiadaRegexps* regexps, int flattened)
{
	if (!regexps) {
		return CYBERIADA_BAD_PARAMETER;
	}
	regexps->flattened_regexps = flattened;
	regexps->berloga_legacy = 0;
	regexps->arena_legacy = 0;
	regexps->r = (CyberiadaRegexpsMics*)malloc(sizeof(CyberiadaRegexpsMics));
	if(!regexps->r) {
		return CYBERIADA_MEMORY_ERROR;
	}
	if (cyb_regcomp(&(regexps->r->edge_action_regexp), CYBERIADA_ACTION_EDGE_REGEXP)) {
		ERROR("cannot compile edge action regexp\n");
		goto error_edge;
	}
	if (cyb_regcomp(&(regexps->r->node_action_regexp), CYBERIADA_ACTION_NODE_REGEXP)) {
		ERROR("cannot compile node action regexp\n");
		goto error_node;
	}
	if (cyb_regcomp(&(regexps->r->node_legacy_action_regexp), CYBERIADA_ACTION_LEGACY_REGEXP)) {
		ERROR("cannot compile legacy node action regexp\n");
		goto error_node_legacy;
	}
	if (cyb_regcomp(&(regexps->r->edge_legacy_action_regexp), CYBERIADA_ACTION_LEGACY_EDGE_REGEXP)) {
		ERROR("cannot compile legacy edge action regexp\n");
		goto error_edge_legacy;
	}
/*	if (cyb_regcomp(&(regexps->newline_regexp), CYBERIADA_ACTION_NEWLINE_REGEXP)) {
	ERROR("cannot compile new line regexp\n");
	return CYBERIADA_ASSERT;
	}*/
	if (cyb_regcomp(&(regexps->r->spaces_regexp), CYBERIADA_ACTION_SPACES_REGEXP)) {
		ERROR("cannot compile new line regexp\n");
		goto error_spaces;
	}
	return CYBERIADA_NO_ERROR;

error_spaces:
	cyb_regfree(&(regexps->r->edge_legacy_action_regexp));
error_edge_legacy:
	cyb_regfree(&(regexps->r->node_legacy_action_regexp));
error_node_legacy:
	cyb_regfree(&(regexps->r->node_action_regexp));
error_node:
	cyb_regfree(&(regexps->r->edge_action_regexp));
error_edge:
	free(regexps->r);
	regexps->r = NULL;
	return CYBERIADA_ASSERT;
}

int cyberiada_free_action_regexps(CyberiadaRegexps* regexps)
{
	if (!regexps || !regexps->r) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyb_regfree(&(regexps->r->edge_action_regexp));
	cyb_regfree(&(regexps->r->node_action_regexp));
	cyb_regfree(&(regexps->r->node_legacy_action_regexp));
	cyb_regfree(&(regexps->r->edge_legacy_action_regexp));
/*	cyb_regfree(&cyberiada_newline_regexp);*/
	cyb_regfree(&(regexps->r->spaces_regexp));
	free(regexps->r);
	return CYBERIADA_NO_ERROR;
}

int cyberiada_action_regexps_spaces(CyberiadaRegexps* regexps, const char* s)
{
	if (!regexps || !regexps->r) {
		return 0;
	}
	return cyb_regexec(&(regexps->r->spaces_regexp), s, 0, NULL, 0) == 0;
}
