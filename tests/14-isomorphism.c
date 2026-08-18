/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The isomorphism check and diff test
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
	CyberiadaDocument *doc1, *doc2;
	CyberiadaNode* node;
	CyberiadaAction *a1, *a2;
	CyberiadaIsomorphismResult iso;
	int result_flags = 0, compare_flags = 0;
	size_t diff_nodes_size = 0, i;
	CyberiadaNodePair* diff_nodes = NULL;
	size_t* diff_nodes_flags = NULL;
	int found;

	doc1 = cyberiada_new_sm_document();
	TEST_ASSERT(doc1);
	TEST_ASSERT(cyberiada_read_sm_document(doc1, "diagrams/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	/* a document copy is identical to the original */
	doc2 = cyberiada_copy_sm_document(doc1);
	TEST_ASSERT(doc2);
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines,
											   doc2->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(iso.diff_nodes_size == 0);
	TEST_ASSERT(iso.diff_edges_size == 0);
	TEST_ASSERT(iso.new_nodes_size == 0);
	TEST_ASSERT(iso.missing_nodes_size == 0);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.diff_nodes == NULL);

	/* a changed title is reported in the node diff */
	node = cyberiada_graph_find_node_by_id(doc2->state_machines->nodes, "n1");
	TEST_ASSERT(node);
	free(node->title);
	TEST_ASSERT(cyberiada_copy_string(&node->title, &node->title_len,
									  "Changed") == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines,
											   doc2->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(!(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL));
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK);
	TEST_ASSERT(iso.diff_nodes_size > 0);
	TEST_ASSERT(iso.diff_nodes);
	TEST_ASSERT(iso.diff_nodes_flags);
	found = 0;
	for (i = 0; i < iso.diff_nodes_size; i++) {
		if (iso.diff_nodes_flags[i] & CYBERIADA_NODE_DIFF_TITLE) {
			TEST_ASSERT(strcmp(iso.diff_nodes[i].n2->title, "Changed") == 0);
			found = 1;
		}
	}
	TEST_ASSERT(found);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);

	/* the deprecated pointer-based form keeps working */
	TEST_ASSERT(cyberiada_check_isomorphism(doc1->state_machines,
											doc2->state_machines, 1, 0,
											&result_flags, NULL,
											&diff_nodes_size, &diff_nodes,
											&diff_nodes_flags,
											NULL, NULL, NULL, NULL,
											NULL, NULL, NULL,
											NULL, NULL, NULL, NULL) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(result_flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK);
	TEST_ASSERT(diff_nodes_size > 0);
	TEST_ASSERT(diff_nodes);
	TEST_ASSERT(diff_nodes_flags);
	TEST_ASSERT(diff_nodes_flags[0] & CYBERIADA_NODE_DIFF_TITLE);
	free(diff_nodes);
	free(diff_nodes_flags);

	/* empty state machines compare as identical */
	{
		CyberiadaDocument *e1, *e2;
		e1 = cyberiada_new_sm_document();
		TEST_ASSERT(e1);
		TEST_ASSERT(cyberiada_read_sm_document(e1, "samples/standard-minimal.graphml",
											   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
					CYBERIADA_NO_ERROR);
		e2 = cyberiada_copy_sm_document(e1);
		TEST_ASSERT(e2);
		TEST_ASSERT(cyberiada_check_sm_isomorphism(e1->state_machines,
												   e2->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
		TEST_ASSERT(iso.diff_nodes_size == 0);
		TEST_ASSERT(iso.new_nodes_size == 0);
		TEST_ASSERT(iso.missing_nodes_size == 0);
		TEST_ASSERT(iso.new_edges_size == 0);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
					CYBERIADA_NO_ERROR);

		/* the empty vs non-empty difference is reported in the arrays */
		TEST_ASSERT(cyberiada_check_sm_isomorphism(e1->state_machines,
												   doc1->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_STATES);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES);
		TEST_ASSERT(iso.new_nodes_size == 3);
		TEST_ASSERT(iso.new_edges_size == 3);
		TEST_ASSERT(iso.missing_nodes_size == 0);
		TEST_ASSERT(iso.diff_nodes_size == 0);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
					CYBERIADA_NO_ERROR);

		/* the swapped order fills the missing side */
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines,
												   e1->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_STATES);
		TEST_ASSERT(iso.missing_nodes_size == 3);
		TEST_ASSERT(iso.missing_edges_size == 3);
		TEST_ASSERT(iso.new_nodes_size == 0);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
					CYBERIADA_NO_ERROR);

		/* the deprecated form accepts the empty graphs too */
		result_flags = 0;
		TEST_ASSERT(cyberiada_check_isomorphism(e1->state_machines,
												e2->state_machines, 1, 0,
												&result_flags, NULL,
												NULL, NULL, NULL,
												NULL, NULL, NULL, NULL,
												NULL, NULL, NULL,
												NULL, NULL, NULL, NULL) ==
					CYBERIADA_NO_ERROR);
		TEST_ASSERT(result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL);

		TEST_ASSERT(cyberiada_destroy_sm_document(e1) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(e2) == CYBERIADA_NO_ERROR);
	}

	/* action comparison */
	a1 = cyberiada_new_action(cybActionTransition, "EVENT", "guard", "act();");
	TEST_ASSERT(a1);
	a2 = cyberiada_new_action(cybActionTransition, "EVENT", "guard", "act();");
	TEST_ASSERT(a2);
	TEST_ASSERT(cyberiada_compare_node_actions(a1, a2, &compare_flags) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(compare_flags == 0);
	free(a2->guard);
	TEST_ASSERT(cyberiada_copy_string(&a2->guard, &a2->guard_len, "other") ==
				CYBERIADA_NO_ERROR);
	compare_flags = 0;
	TEST_ASSERT(cyberiada_compare_node_actions(a1, a2, &compare_flags) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(compare_flags & CYBERIADA_ACTION_DIFF_GUARDS);
	TEST_ASSERT(cyberiada_destroy_action(a1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_action(a2) == CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc2) == CYBERIADA_NO_ERROR);
	return 0;
}
