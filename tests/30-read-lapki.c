/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Lapki dialect test
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

#include <math.h>
#include <string.h>
#include <cyberiadaml.h>
#include "testutils.h"

#define EPS 0.01

static CyberiadaDocument* load(const char* filename, int flags, int result)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown, flags) == result);
	if (result == CYBERIADA_NO_ERROR) {
		TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
														coordLeftTop, edgeBorder) ==
					CYBERIADA_NO_ERROR);
	}
	return doc;
}

static CyberiadaNode* node(CyberiadaDocument* doc, const char* id)
{
	CyberiadaNode* n = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, id);
	TEST_ASSERT(n);
	return n;
}

static void lapki(const char* filename, const char* written, int strict_copy)
{
	CyberiadaDocument* doc;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown,
										   CYBERIADA_FLAG_STRICT) != CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	doc = load(filename, CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	TEST_ASSERT(strcmp(doc->format, "Cyberiada-GraphML-Lapki") == 0);
	TEST_ASSERT(cyberiada_write_sm_document(doc, written, cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	doc = load(written, strict_copy ? CYBERIADA_FLAG_STRICT : CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode *n, *region;
	CyberiadaSM* sm;
	int metas;

	/* the region graph named after its parent node gets an own id */
	lapki("samples/lapki-stapler.graphml", "30-out-stapler.graphml", 0);
	doc = load("samples/lapki-stapler.graphml", CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	region = node(doc, "n0::region");
	TEST_ASSERT(region->type == cybNodeRegion && strcmp(region->parent->id, "n0") == 0);
	TEST_ASSERT(node(doc, "n0::n1")->parent == region);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the point of a comment is read as a loose rect */
	lapki("samples/lapki-blinker.graphml", "30-out-blinker.graphml", 1);
	doc = load("samples/lapki-blinker.graphml", CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	n = node(doc, "commentX");
	TEST_ASSERT(n->geometry_rect && !n->geometry_point);
	TEST_ASSERT(n->geometry_rect->width > 0.0 && n->geometry_rect->height > 0.0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the cyberiadaml-js writer: the -1 x -1 point rect, the repeated meta */
	lapki("diagrams/lapki-js.graphml", "30-out-js.graphml", 1);
	doc = load("diagrams/lapki-js.graphml", CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	n = node(doc, "init");
	TEST_ASSERT(n->geometry_point && !n->geometry_rect);
	TEST_ASSERT(fabs(n->geometry_point->x + 60.5) < EPS && fabs(n->geometry_point->y - 20.0) < EPS);
	n = node(doc, "note");
	TEST_ASSERT(n->geometry_rect && fabs(n->geometry_rect->x - 500.0) < EPS);
	TEST_ASSERT(node(doc, "n0::region")->type == cybNodeRegion);
	metas = 0;
	for (sm = doc->state_machines; sm; sm = sm->next) {
		for (n = sm->nodes->children; n; n = n->next) {
			if (n->type == cybNodeFormalComment && strcmp(n->title, "CGML_META") == 0) metas++;
		}
	}
	TEST_ASSERT(metas == 1);
	TEST_ASSERT(doc->state_machines->next && doc->state_machines->next->nodes->children &&
				strcmp(doc->state_machines->next->nodes->children->id, "m2s") == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
