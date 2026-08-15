/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The document lifecycle test
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
	CyberiadaDocument stack_doc;
	CyberiadaDocument *doc, *copy;
	CyberiadaSM* sm;
	CyberiadaNode *node, *child;
	CyberiadaEdge* edge;

	/* stack allocation */
	TEST_ASSERT(cyberiada_init_sm_document(&stack_doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(stack_doc.state_machines == NULL);
	TEST_ASSERT(cyberiada_cleanup_sm_document(&stack_doc) == CYBERIADA_NO_ERROR);

	/* heap allocation with programmatic content */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	doc->meta_info = cyberiada_new_meta();
	TEST_ASSERT(doc->meta_info);
	doc->meta_info->strings = cyberiada_new_meta_string("name", "lifecycle");
	TEST_ASSERT(doc->meta_info->strings);

	sm = cyberiada_new_sm();
	TEST_ASSERT(sm);
	doc->state_machines = sm;

	node = cyberiada_new_node("root");
	TEST_ASSERT(node);
	node->type = cybNodeSM;
	TEST_ASSERT(cyberiada_copy_string(&node->title, &node->title_len, "SM") ==
				CYBERIADA_NO_ERROR);
	sm->nodes = node;

	child = cyberiada_new_node("n0");
	TEST_ASSERT(child);
	child->type = cybNodeSimpleState;
	child->parent = node;
	child->actions = cyberiada_new_action(cybActionEntry, "", "", "start();");
	TEST_ASSERT(child->actions);
	node->children = child;

	edge = cyberiada_new_edge("e0", "n0", "n0", 1);
	TEST_ASSERT(edge);
	edge->action = cyberiada_new_action(cybActionTransition, "EVENT", "guard", "act();");
	TEST_ASSERT(edge->action);
	sm->edges = edge;

	TEST_ASSERT(cyberiada_graph_find_node_by_id(sm->nodes, "n0") == child);
	TEST_ASSERT(cyberiada_graph_find_node_by_id(sm->nodes, "unknown") == NULL);
	TEST_ASSERT(cyberiada_graph_find_node_by_type(sm->nodes, cybNodeSimpleState) == child);

	/* deep copy and destroy both documents */
	copy = cyberiada_copy_sm_document(doc);
	TEST_ASSERT(copy);
	TEST_ASSERT(copy != doc);
	TEST_ASSERT(copy->state_machines);
	TEST_ASSERT(copy->state_machines->nodes);
	TEST_ASSERT(copy->state_machines->nodes != doc->state_machines->nodes);
	TEST_ASSERT(strcmp(copy->state_machines->nodes->id, "root") == 0);
	node = cyberiada_graph_find_node_by_id(copy->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	TEST_ASSERT(node->actions);
	TEST_ASSERT(strcmp(node->actions->behavior, "start();") == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(copy) == CYBERIADA_NO_ERROR);

	return 0;
}
