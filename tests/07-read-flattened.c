/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The flattened document reading test
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

#include <cyberiadaml.h>
#include "testutils.h"

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode* node;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal-flat.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->state_machines);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	TEST_ASSERT(node->actions);
	TEST_ASSERT(node->actions->type == cybActionEntry);
	TEST_ASSERT(strcmp(node->actions->behavior, "off();") == 0);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n1");
	TEST_ASSERT(node);
	TEST_ASSERT(node->actions);
	TEST_ASSERT(node->actions->type == cybActionEntry);
	TEST_ASSERT(strcmp(node->actions->behavior, "on();") == 0);

	TEST_ASSERT(doc->state_machines->edges);
	TEST_ASSERT(doc->state_machines->edges->next);
	TEST_ASSERT(doc->state_machines->edges->next->action);
	TEST_ASSERT(strcmp(doc->state_machines->edges->next->action->trigger,
					   "TURN_ON") == 0);
	TEST_ASSERT(strcmp(doc->state_machines->edges->next->action->behavior,
					   "start();") == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
