/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library tests
 *
 * The revision diff of two documents
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <cyberiadaml.h>
#include "testutils.h"

static CyberiadaDocument* read_doc(const char* path)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, path, cybxmlUnknown, CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	return doc;
}

/* the flags of the node entry by the ids of its sides ("-" for none), ~0 when absent */
static unsigned int node_flags(const CyberiadaSMDiff* d, const char* id1, const char* id2)
{
	size_t i;
	for (i = 0; i < d->nodes_size; i++) {
		const CyberiadaNodeDiff* n = d->nodes + i;
		if (strcmp(n->n1 ? n->n1->id : "-", id1) == 0 && strcmp(n->n2 ? n->n2->id : "-", id2) == 0) {
			return n->flags;
		}
	}
	return ~0U;
}

static unsigned int edge_flags(const CyberiadaSMDiff* d, const char* id1, const char* id2)
{
	size_t i;
	for (i = 0; i < d->edges_size; i++) {
		const CyberiadaEdgeDiff* e = d->edges + i;
		if (strcmp(e->e1 ? e->e1->id : "-", id1) == 0 && strcmp(e->e2 ? e->e2->id : "-", id2) == 0) {
			return e->flags;
		}
	}
	return ~0U;
}

int main(void)
{
	CyberiadaDocument *base, *copy, *edited, *renumbered;
	CyberiadaDocumentDiff diff;
	CyberiadaDiffOptions options;
	const CyberiadaSMDiff* sm;
	size_t i;

	base = read_doc("diagrams/diff-light.graphml");
	edited = read_doc("diagrams/diff-light-edited.graphml");
	renumbered = read_doc("diagrams/diff-light-renumbered.graphml");
	copy = cyberiada_copy_sm_document(base);
	TEST_ASSERT(copy);

	/* a copy has no differences, every pair listed (the region of Night counts) */
	TEST_ASSERT(cyberiada_diff_documents(base, copy, NULL, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(diff.flags == 0 && diff.meta_size == 0 && diff.sms_size == 1);
	TEST_ASSERT(diff.sms[0].nodes_size == 8 && diff.sms[0].edges_size == 7);
	for (i = 0; i < diff.sms[0].nodes_size; i++) TEST_ASSERT(diff.sms[0].nodes[i].flags == 0);
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(diff.sms == NULL && diff.sms_size == 0);

	/* the edited revision */
	TEST_ASSERT(cyberiada_diff_documents(base, edited, NULL, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(diff.flags & CYBERIADA_DIFF_FLAG_META);
	TEST_ASSERT(diff.flags & CYBERIADA_DIFF_FLAG_ELEMENTS);
	TEST_ASSERT(diff.meta_size == 1 && strcmp(diff.meta[0].name, "platform") == 0);
	TEST_ASSERT(strcmp(diff.meta[0].value1, "ArduinoUno") == 0 && strcmp(diff.meta[0].value2, "ArduinoNano") == 0);
	sm = diff.sms;
	TEST_ASSERT(node_flags(sm, "init", "init") == 0);
	TEST_ASSERT(node_flags(sm, "n1", "n1") == CYBERIADA_DIFF_FLAG_COLOR);         /* moved: geometry option only */
	TEST_ASSERT(node_flags(sm, "n2", "n2") == CYBERIADA_DIFF_FLAG_PARENT);        /* Yellow into Night */
	TEST_ASSERT(node_flags(sm, "n3", "-") == CYBERIADA_DIFF_FLAG_REMOVED);        /* Green */
	TEST_ASSERT(node_flags(sm, "-", "n3") == CYBERIADA_DIFF_FLAG_ADDED);          /* Blink took its id */
	TEST_ASSERT(node_flags(sm, "n5", "n5") == 0);
	TEST_ASSERT(node_flags(sm, "c1", "c1") == CYBERIADA_DIFF_FLAG_COMMENT);
	TEST_ASSERT(edge_flags(sm, "n1-n2", "n1-n2") == CYBERIADA_DIFF_FLAG_BEHAVIOR);
	TEST_ASSERT(edge_flags(sm, "n1-n5", "n1-n5") == CYBERIADA_DIFF_FLAG_TARGET); /* re-targeted, rule a */
	TEST_ASSERT(edge_flags(sm, "n5-n1", "n5-n1#2") == CYBERIADA_DIFF_FLAG_TRIGGER); /* rule c */
	TEST_ASSERT(edge_flags(sm, "n2-n3", "-") == CYBERIADA_DIFF_FLAG_REMOVED);
	TEST_ASSERT(edge_flags(sm, "n3-n1", "-") == CYBERIADA_DIFF_FLAG_REMOVED);
	TEST_ASSERT(edge_flags(sm, "-", "n2-n1") == CYBERIADA_DIFF_FLAG_ADDED);
	TEST_ASSERT(edge_flags(sm, "c1-n1", "c1-n1") == 0);
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);

	/* the geometry option reports the moved Red */
	cyberiada_default_diff_options(&options);
	options.flags = CYBERIADA_DIFF_OPTION_GEOMETRY;
	TEST_ASSERT(cyberiada_diff_documents(base, edited, &options, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(node_flags(diff.sms, "n1", "n1") == (CYBERIADA_DIFF_FLAG_COLOR | CYBERIADA_DIFF_FLAG_GEOMETRY));
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);

	/* the comments left out */
	options.flags = CYBERIADA_DIFF_OPTION_IGNORE_COMMENTS;
	TEST_ASSERT(cyberiada_diff_documents(base, edited, &options, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(node_flags(diff.sms, "c1", "c1") == ~0U);
	TEST_ASSERT(edge_flags(diff.sms, "c1-n1", "c1-n1") == ~0U);
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);

	/* renumbered ids: no structural pass, everything removed and added */
	TEST_ASSERT(cyberiada_diff_documents(base, renumbered, NULL, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(diff.sms_size == 1 && diff.sms[0].nodes_size == 16 && diff.sms[0].edges_size == 14);
	for (i = 0; i < diff.sms[0].nodes_size; i++) {
		TEST_ASSERT(diff.sms[0].nodes[i].flags & (CYBERIADA_DIFF_FLAG_ADDED | CYBERIADA_DIFF_FLAG_REMOVED));
	}
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);

	/* a revision saved as yEd: the states and the transitions are the same (the
	   dialect keeps neither the meta, nor the SM name, nor the comment links) */
	{
		CyberiadaDocument* yed;
		TEST_ASSERT(cyberiada_write_sm_document(base, "36-out.graphml", cybxmlYEDOstranna, CYBERIADA_FLAG_NO) ==
					CYBERIADA_NO_ERROR);
		yed = read_doc("36-out.graphml");
		TEST_ASSERT(cyberiada_diff_documents(base, yed, NULL, &diff) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(diff.sms_size == 1);
		for (i = 0; i < diff.sms[0].nodes_size; i++) TEST_ASSERT(diff.sms[0].nodes[i].flags == 0);
		for (i = 0; i < diff.sms[0].edges_size; i++) {
			const CyberiadaEdgeDiff* e = diff.sms[0].edges + i;
			const CyberiadaEdge* edge = e->e1 ? e->e1 : e->e2;
			if (edge->source->type != cybNodeComment) TEST_ASSERT(e->flags == 0);
		}
		TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(yed) == CYBERIADA_NO_ERROR);
	}

	/* a removed state machine */
	{
		CyberiadaDocument *two = read_doc("diagrams/two-machines.graphml"), *one;
		CyberiadaSM* second;
		one = cyberiada_copy_sm_document(two);
		TEST_ASSERT(one && one->state_machines && one->state_machines->next);
		second = one->state_machines->next;
		one->state_machines->next = NULL;
		TEST_ASSERT(cyberiada_diff_documents(two, one, NULL, &diff) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(diff.sms_size == 2);
		TEST_ASSERT(diff.sms[0].flags == 0 && diff.sms[0].sm2 == one->state_machines);
		TEST_ASSERT(diff.sms[1].flags & CYBERIADA_DIFF_FLAG_REMOVED);
		TEST_ASSERT(diff.sms[1].sm1 == two->state_machines->next && diff.sms[1].sm2 == NULL);
		for (i = 0; i < diff.sms[1].nodes_size; i++) {
			TEST_ASSERT(diff.sms[1].nodes[i].flags == CYBERIADA_DIFF_FLAG_REMOVED);
		}
		TEST_ASSERT(diff.flags & CYBERIADA_DIFF_FLAG_REMOVED);
		TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm(second) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(one) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_destroy_sm_document(two) == CYBERIADA_NO_ERROR);
	}

	/* the state machine form */
	{
		CyberiadaSMDiff smd;
		TEST_ASSERT(cyberiada_diff_sms(base->state_machines, edited->state_machines, NULL, &smd) ==
					CYBERIADA_NO_ERROR);
		TEST_ASSERT(smd.flags & CYBERIADA_DIFF_FLAG_ELEMENTS);
		TEST_ASSERT(node_flags(&smd, "n3", "-") == CYBERIADA_DIFF_FLAG_REMOVED);
		TEST_ASSERT(cyberiada_cleanup_sm_diff(&smd) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_diff_sms(NULL, edited->state_machines, NULL, &smd) == CYBERIADA_BAD_PARAMETER);
	}

	/* the errors: no result, the geometry of different coordinate formats */
	TEST_ASSERT(cyberiada_diff_documents(base, copy, NULL, NULL) == CYBERIADA_BAD_PARAMETER);
	copy->node_coord_format = (copy->node_coord_format == coordAbsolute) ? coordLeftTop : coordAbsolute;
	options.flags = CYBERIADA_DIFF_OPTION_GEOMETRY;
	TEST_ASSERT(cyberiada_diff_documents(base, copy, &options, &diff) == CYBERIADA_BAD_PARAMETER);
	TEST_ASSERT(cyberiada_diff_documents(base, copy, NULL, &diff) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_cleanup_document_diff(&diff) == CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_destroy_sm_document(base) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(copy) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(edited) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(renumbered) == CYBERIADA_NO_ERROR);
	return 0;
}
