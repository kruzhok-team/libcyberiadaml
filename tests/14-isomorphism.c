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

/* append a simple state to the children of a node */
static CyberiadaNode* add_state(CyberiadaNode* parent, const char* id, const char* title)
{
	CyberiadaNode *node = cyberiada_new_node(id), *n;
	TEST_ASSERT(node);
	node->type = cybNodeSimpleState;
	TEST_ASSERT(cyberiada_copy_string(&node->title, &node->title_len, title) == CYBERIADA_NO_ERROR);
	node->parent = parent;
	if (!parent->children) {
		parent->children = node;
	} else {
		for (n = parent->children; n->next; n = n->next);
		n->next = node;
	}
	return node;
}

/* append a transition between two nodes of a state machine */
static CyberiadaEdge* add_transition(CyberiadaSM* sm, const char* id, const char* source, const char* target)
{
	CyberiadaEdge *edge = cyberiada_new_edge(id, source, target, 0), *e;
	TEST_ASSERT(edge);
	edge->source = cyberiada_graph_find_node_by_id(sm->nodes, source);
	edge->target = cyberiada_graph_find_node_by_id(sm->nodes, target);
	TEST_ASSERT(edge->source && edge->target);
	for (e = sm->edges; e->next; e = e->next);
	e->next = edge;
	return edge;
}

/* the flags of the pair of a node of the first graph, 0 when not differing */
static size_t node_pair_flags(CyberiadaIsomorphismResult* iso, const char* id)
{
	size_t i;
	for (i = 0; i < iso->diff_nodes_size; i++) {
		if (strcmp(iso->diff_nodes[i].n1->id, id) == 0) {
			return iso->diff_nodes_flags[i];
		}
	}
	return 0;
}

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

	/* different states and different edges are both reported */
	{
		CyberiadaDocument* c = cyberiada_copy_sm_document(doc1);
		TEST_ASSERT(c);
		add_state(c->state_machines->nodes, "n2", "Extra");
		add_transition(c->state_machines, "n1-n2", "n1", "n2");
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines, c->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_STATES);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES);
		TEST_ASSERT(!(iso.flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK));
		TEST_ASSERT(iso.new_nodes_size == 1 && strcmp(iso.new_nodes[0]->id, "n2") == 0);
		TEST_ASSERT(iso.new_edges_size == 1 && strcmp(iso.new_edges[0]->id, "n1-n2") == 0);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c) == CYBERIADA_NO_ERROR);
	}

	/* a re-targeted transition between kept states is a missing and a new edge */
	{
		CyberiadaDocument* c = cyberiada_copy_sm_document(doc1);
		CyberiadaEdge* e;
		TEST_ASSERT(c);
		for (e = c->state_machines->edges; e && strcmp(e->id, "n1-n0") != 0; e = e->next);
		TEST_ASSERT(e);
		e->target = e->source;
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines, c->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES);
		TEST_ASSERT(iso.missing_edges_size == 1 && strcmp(iso.missing_edges[0]->id, "n1-n0") == 0);
		TEST_ASSERT(iso.new_edges_size == 1);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c) == CYBERIADA_NO_ERROR);
	}

	/* the submachine links are compared with each other, a link on one side differs */
	{
		CyberiadaDocument *c1 = cyberiada_copy_sm_document(doc1), *c2 = cyberiada_copy_sm_document(doc1);
		CyberiadaNode *s1, *s2;
		TEST_ASSERT(c1 && c2);
		s1 = cyberiada_graph_find_node_by_id(c1->state_machines->nodes, "n1");
		s2 = cyberiada_graph_find_node_by_id(c2->state_machines->nodes, "n1");
		TEST_ASSERT(s1 && s2);
		s1->type = s2->type = cybNodeSubmachineState;
		s1->link = cyberiada_new_link("G1");
		s2->link = cyberiada_new_link("G1");
		TEST_ASSERT(cyberiada_check_sm_isomorphism(c1->state_machines, c2->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		free(s2->link->ref);
		TEST_ASSERT(cyberiada_copy_string(&s2->link->ref, &s2->link->ref_len, "G2") == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_check_sm_isomorphism(c1->state_machines, c2->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(node_pair_flags(&iso, "n1") & CYBERIADA_NODE_DIFF_SM_LINK);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		free(s2->link->ref);
		free(s2->link);
		s2->link = NULL;
		TEST_ASSERT(cyberiada_check_sm_isomorphism(c1->state_machines, c2->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(node_pair_flags(&iso, "n1") & CYBERIADA_NODE_DIFF_SM_LINK);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c1) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c2) == CYBERIADA_NO_ERROR);
	}

	/* a cleared title differs */
	{
		CyberiadaDocument* c = cyberiada_copy_sm_document(doc1);
		TEST_ASSERT(c);
		node = cyberiada_graph_find_node_by_id(c->state_machines->nodes, "n1");
		TEST_ASSERT(node);
		free(node->title);
		node->title = NULL;
		node->title_len = 0;
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines, c->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(node_pair_flags(&iso, "n1") & CYBERIADA_NODE_DIFF_TITLE);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c) == CYBERIADA_NO_ERROR);
	}

	/* a simple state turned composite differs by its children, not by its type */
	{
		CyberiadaDocument* c = cyberiada_copy_sm_document(doc1);
		size_t flags;
		TEST_ASSERT(c);
		node = cyberiada_graph_find_node_by_id(c->state_machines->nodes, "n0");
		TEST_ASSERT(node);
		node->type = cybNodeCompositeState;
		add_state(node, "n0::n3", "Inner");
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines, c->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		flags = node_pair_flags(&iso, "n0");
		TEST_ASSERT(flags & CYBERIADA_NODE_DIFF_CHILDREN);
		TEST_ASSERT(!(flags & CYBERIADA_NODE_DIFF_TYPE));
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_DIFF_STATES);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c) == CYBERIADA_NO_ERROR);
	}

	/* a state that lost two transitions is still paired, both ways */
	{
		CyberiadaDocument* c = cyberiada_copy_sm_document(doc1);
		TEST_ASSERT(c);
		add_transition(c->state_machines, "n0-n1#1", "n0", "n1");
		add_transition(c->state_machines, "n0-n1#2", "n0", "n1");
		TEST_ASSERT(cyberiada_check_sm_isomorphism(c->state_machines, doc1->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.missing_nodes_size == 0 && iso.new_nodes_size == 0);
		TEST_ASSERT(iso.missing_edges_size == 2);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_check_sm_isomorphism(doc1->state_machines, c->state_machines, 1, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.missing_nodes_size == 0 && iso.new_nodes_size == 0);
		TEST_ASSERT(iso.new_edges_size == 2);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(c) == CYBERIADA_NO_ERROR);
	}

	/* with the comments compared, a comment link to a transition is paired */
	{
		CyberiadaDocument *l1, *l2;
		l1 = cyberiada_new_sm_document();
		TEST_ASSERT(l1);
		TEST_ASSERT(cyberiada_read_sm_document(l1, "diagrams/link-name-on-transition.graphml",
											   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
					CYBERIADA_NO_ERROR);
		l2 = cyberiada_copy_sm_document(l1);
		TEST_ASSERT(l2);
		TEST_ASSERT(cyberiada_check_sm_isomorphism(l1->state_machines, l2->state_machines, 0, 0,
												   &iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
		TEST_ASSERT(iso.missing_edges_size == 0 && iso.new_edges_size == 0);
		TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(l1) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(l2) == CYBERIADA_NO_ERROR);
	}

	/* absent action strings compare as empty ones */
	a1 = cyberiada_new_action(cybActionTransition, NULL, NULL, NULL);
	TEST_ASSERT(a1);
	a2 = cyberiada_new_action(cybActionTransition, "", "", "");
	TEST_ASSERT(a2);
	compare_flags = 0;
	TEST_ASSERT(cyberiada_compare_node_actions(a1, a2, &compare_flags) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(compare_flags == 0);
	TEST_ASSERT(cyberiada_compare_node_actions(a1, a2, NULL) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_action(a1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_action(a2) == CYBERIADA_NO_ERROR);

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
