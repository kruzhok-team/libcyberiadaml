/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The event handling keywords test
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
	CyberiadaDocument *doc, *reread;
	CyberiadaNode* node;
	CyberiadaAction *action, *a1, *a2;
	CyberiadaEdge* edge;
	CyberiadaIsomorphismResult iso;
	int compare_flags = 0;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/event-propagation.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	/* the internal transition carries the defer keyword, the trigger is clean */
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	action = node->actions;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionEntry);
	TEST_ASSERT(action->propagation == cybEventPropagationNone);
	action = action->next;
	TEST_ASSERT(action);
	TEST_ASSERT(action->type == cybActionTransition);
	TEST_ASSERT(strcmp(action->trigger, "TICK") == 0);
	TEST_ASSERT(action->propagation == cybEventPropagationDefer);

	/* the edge keywords: propagate, block with a guard, defer w/o behavior */
	edge = doc->state_machines->edges;
	TEST_ASSERT(edge && !edge->action);
	edge = edge->next;
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(strcmp(edge->action->trigger, "START") == 0);
	TEST_ASSERT(edge->action->propagation == cybEventPropagationPropagate);
	edge = edge->next;
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(strcmp(edge->action->trigger, "STOP") == 0);
	TEST_ASSERT(strcmp(edge->action->guard, "ready") == 0);
	TEST_ASSERT(edge->action->propagation == cybEventPropagationBlock);
	edge = edge->next;
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(strcmp(edge->action->trigger, "RESET") == 0);
	TEST_ASSERT(edge->action->propagation == cybEventPropagationDefer);

	/* the keywords survive the write/read round trip */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "19-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, "19-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	edge = reread->state_machines->edges->next->next;
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(edge->action->propagation == cybEventPropagationBlock);
	TEST_ASSERT(strcmp(edge->action->guard, "ready") == 0);
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											   reread->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the propagation difference is reported by the action comparison */
	a1 = cyberiada_new_action(cybActionTransition, "EVENT", "", "act();");
	TEST_ASSERT(a1);
	a2 = cyberiada_new_action(cybActionTransition, "EVENT", "", "act();");
	TEST_ASSERT(a2);
	a2->propagation = cybEventPropagationDefer;
	TEST_ASSERT(cyberiada_compare_node_actions(a1, a2, &compare_flags) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(compare_flags & CYBERIADA_ACTION_DIFF_PROPAGATION);
	TEST_ASSERT(cyberiada_destroy_action(a1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_action(a2) == CYBERIADA_NO_ERROR);

	return 0;
}
