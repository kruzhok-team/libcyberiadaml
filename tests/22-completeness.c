/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The completeness extension test
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

static CyberiadaEdge* find_edge(CyberiadaSM* sm, const char* id)
{
	CyberiadaEdge* edge;
	for (edge = sm->edges; edge; edge = edge->next) {
		if (edge->id && strcmp(edge->id, id) == 0) {
			return edge;
		}
	}
	return NULL;
}

static void check_completeness(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;
	CyberiadaNode* node;
	CyberiadaEdge* edge;

	sm = doc->state_machines;
	TEST_ASSERT(sm && sm->next);
	TEST_ASSERT(strcmp(sm->nodes->title, "Completeness") == 0);
	TEST_ASSERT(strcmp(sm->next->nodes->title, "Worker") == 0);

	/* the collapsed composite state */
	node = cyberiada_graph_find_node_by_id(sm->nodes, "c0");
	TEST_ASSERT(node);
	TEST_ASSERT(node->type == cybNodeCompositeState);
	TEST_ASSERT(node->collapsed_flag);

	/* the submachine state keeps its type and link */
	node = cyberiada_graph_find_node_by_id(sm->nodes, "sub0");
	TEST_ASSERT(node);
	TEST_ASSERT(node->type == cybNodeSubmachineState);
	TEST_ASSERT(node->link && node->link->ref);
	TEST_ASSERT(strcmp(node->link->ref, "M2") == 0);

	/* the escaped brackets of the guard are kept verbatim */
	edge = find_edge(sm, "c0-sub0#1");
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(strcmp(edge->action->trigger, "FIND") == 0);
	TEST_ASSERT(strcmp(edge->action->guard, "text.Contains(\\[sample\\])") == 0);
	TEST_ASSERT(strcmp(edge->action->behavior, "mark()") == 0);

	/* the comment link points to the transition */
	edge = find_edge(sm, "cN-t#1");
	TEST_ASSERT(edge);
	TEST_ASSERT(edge->type == cybEdgeComment);
	TEST_ASSERT(edge->target == NULL);
	TEST_ASSERT(edge->target_edge);
	TEST_ASSERT(strcmp(edge->target_edge->id, "c0-sub0#1") == 0);
}

int main(void)
{
	CyberiadaDocument *doc, *reread;
	CyberiadaSM* sm;

	/* two state machines in a single document */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/two-machines.graphml",
										   cybxmlCyberiada10, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	sm = doc->state_machines;
	TEST_ASSERT(sm && sm->next && !sm->next->next);
	TEST_ASSERT(strcmp(sm->nodes->title, "First") == 0);
	TEST_ASSERT(strcmp(sm->next->nodes->title, "Second") == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the completeness features survive the write/read round trip */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/completeness.graphml",
										   cybxmlCyberiada10, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	check_completeness(doc);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "22-out.graphml", cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, "22-out.graphml",
										   cybxmlCyberiada10, CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);
	check_completeness(reread);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
