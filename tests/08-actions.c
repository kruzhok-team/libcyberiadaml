/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The action text parsing test
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
	CyberiadaAction* action;
	CyberiadaEdge* edge;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/actions.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->state_machines);

	/* state actions: entry (two statements), do, exit */
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	action = node->actions;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionEntry);
	TEST_ASSERT(strstr(action->behavior, "init();"));
	TEST_ASSERT(strstr(action->behavior, "setup();"));
	action = action->next;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionDo);
	TEST_ASSERT(strstr(action->behavior, "work();"));
	action = action->next;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionExit);
	TEST_ASSERT(strstr(action->behavior, "teardown();"));
	TEST_ASSERT(!action->next);

	/* transition action with trigger, guard and behavior */
	edge = doc->state_machines->edges;
	TEST_ASSERT(edge);
	edge = edge->next;
	TEST_ASSERT(edge);
	action = edge->action;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionTransition);
	TEST_ASSERT(strcmp(action->trigger, "DONE") == 0);
	TEST_ASSERT(strcmp(action->guard, "count > 0") == 0);
	TEST_ASSERT(strstr(action->behavior, "report(count);"));

	/* transition with the empty behavior */
	edge = edge->next;
	TEST_ASSERT(edge);
	action = edge->action;
	TEST_ASSERT(action);
	TEST_ASSERT(strcmp(action->trigger, "RESTART") == 0);
	TEST_ASSERT(action->behavior == NULL || strlen(action->behavior) == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
