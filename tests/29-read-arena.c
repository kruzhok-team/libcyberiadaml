/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Arena dialect test
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
#define SAMPLE "samples/arena-stapler.graphml"
#define SCAN   "57f705d2-4cfa-4bc9-bddd-34d813665361"
#define FIGHT  "7c1214b0-a01e-4ddd-b28e-81064e9a8317"
#define CLOSE  "f2336ae0-ab0d-437e-b6fa-16dffd4ddb0b"
#define ATTACK "3b8d2d03-7973-4db9-9f85-c57bb0ca45aa"

static int overlap(const CyberiadaRect* a, const CyberiadaRect* b)
{
	return (a->x < b->x + b->width - EPS && b->x < a->x + a->width - EPS &&
			a->y < b->y + b->height - EPS && b->y < a->y + a->height - EPS);
}

/* every state is a rect inside its container, the siblings do not overlap */
static void check_container(const CyberiadaNode* node, int bounded, double w, double h)
{
	const CyberiadaNode *a, *b;

	if (node->geometry_rect) {
		bounded = 1;
		w = node->geometry_rect->width;
		h = node->geometry_rect->height;
	}
	for (a = node->children; a; a = a->next) {
		if (a->type & (cybNodeSimpleState | cybNodeCompositeState)) {
			TEST_ASSERT(a->geometry_rect && !a->geometry_point);
		}
		if (a->geometry_rect) {
			if (bounded) {
				TEST_ASSERT(a->geometry_rect->x >= -EPS);
				TEST_ASSERT(a->geometry_rect->y >= -EPS);
				TEST_ASSERT(a->geometry_rect->x + a->geometry_rect->width <= w + EPS);
				TEST_ASSERT(a->geometry_rect->y + a->geometry_rect->height <= h + EPS);
			}
			for (b = a->next; b; b = b->next) {
				if (b->geometry_rect) {
					TEST_ASSERT(!overlap(a->geometry_rect, b->geometry_rect));
				}
			}
		}
		if (a->children) {
			check_container(a, bounded, w, h);
		}
	}
}

static CyberiadaDocument* load(const char* filename, int flags, int result)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown, flags) == result);
	if (result == CYBERIADA_NO_ERROR && !(flags & CYBERIADA_FLAG_SKIP_GEOMETRY)) {
		TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
														coordLeftTop, edgeBorder) ==
					CYBERIADA_NO_ERROR);
	}
	return doc;
}

static const CyberiadaRect* rect(CyberiadaDocument* doc, const char* id)
{
	CyberiadaNode* node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, id);
	TEST_ASSERT(node && node->geometry_rect);
	return node->geometry_rect;
}

int main(void)
{
	CyberiadaDocument *doc, *copy;
	CyberiadaNode* node;
	CyberiadaEdge* edge;
	const char* ids[] = { SCAN, FIGHT, CLOSE, ATTACK };
	size_t i;

	/* the Arena points become rects in the non-strict mode */
	doc = load(SAMPLE, CYBERIADA_FLAG_NO, CYBERIADA_NO_ERROR);
	TEST_ASSERT(strcmp(doc->format, "Cyberiada-GraphML-Arena") == 0);
	check_container(doc->state_machines->nodes, 0, 0.0, 0.0);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "initial");
	TEST_ASSERT(node && node->geometry_point && !node->geometry_rect);

	/* the layout is kept: Scan left of Fight, y turned down inside Fight */
	TEST_ASSERT(rect(doc, SCAN)->x + rect(doc, SCAN)->width < rect(doc, FIGHT)->x);
	TEST_ASSERT(rect(doc, CLOSE)->x < rect(doc, ATTACK)->x);
	TEST_ASSERT(rect(doc, CLOSE)->y < rect(doc, ATTACK)->y);

	/* the Arena label points have no frame and are dropped */
	for (edge = doc->state_machines->edges; edge; edge = edge->next) {
		TEST_ASSERT(!edge->geometry_label_point && !edge->geometry_label_rect);
	}

	/* the written document is standard */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "29-out.graphml", cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	copy = load("29-out.graphml", CYBERIADA_FLAG_STRICT, CYBERIADA_NO_ERROR);
	TEST_ASSERT(strcmp(copy->format, "Cyberiada-GraphML-1.0") == 0);
	for (i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
		TEST_ASSERT(fabs(rect(doc, ids[i])->x - rect(copy, ids[i])->x) < EPS);
		TEST_ASSERT(fabs(rect(doc, ids[i])->y - rect(copy, ids[i])->y) < EPS);
		TEST_ASSERT(fabs(rect(doc, ids[i])->width - rect(copy, ids[i])->width) < EPS);
		TEST_ASSERT(fabs(rect(doc, ids[i])->height - rect(copy, ids[i])->height) < EPS);
	}
	TEST_ASSERT(cyberiada_destroy_sm_document(copy) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the strict mode rejects the point geometry of a state */
	doc = load(SAMPLE, CYBERIADA_FLAG_STRICT, CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the geometry may be skipped */
	doc = load(SAMPLE, CYBERIADA_FLAG_SKIP_GEOMETRY, CYBERIADA_NO_ERROR);
	TEST_ASSERT(strcmp(doc->format, "Cyberiada-GraphML-Arena") == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
