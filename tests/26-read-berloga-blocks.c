/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The legacy Berloga action blocks test
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 * ----------------------------------------------------------------------------- */

#include <string.h>

#include <cyberiadaml.h>
#include "testutils.h"

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode* node;
	CyberiadaAction* action;
	size_t count = 0;

	/* the Berloga action blocks are separated by a single newline whenever the
	   previous block has no behavior, so an empty entry must not swallow the
	   exit block that follows it */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/berloga-empty-actions.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->format);
	TEST_ASSERT(strncmp(doc->format, "yEd Berloga", 11) == 0);
	TEST_ASSERT(doc->state_machines && doc->state_machines->nodes);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	for (action = node->actions; action; action = action->next) {
		TEST_ASSERT(action->behavior);
		/* the behavior of an empty block is empty, not the next block itself */
		TEST_ASSERT(strstr(action->behavior, "exit/") == NULL);
		count++;
	}
	TEST_ASSERT(count == 2);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
