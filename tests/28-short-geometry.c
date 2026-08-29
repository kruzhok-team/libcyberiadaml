/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The base (short) geometry format test
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
#include <cyberiadaml.h>
#include "testutils.h"

#define EPS 0.01

static int overlap(const CyberiadaRect* a, const CyberiadaRect* b)
{
	return (a->x < b->x + b->width - EPS && b->x < a->x + a->width - EPS &&
			a->y < b->y + b->height - EPS && b->y < a->y + a->height - EPS);
}

/* Every child rect is inside its container and no two siblings overlap.
   A container with no geometry of its own passes the box of the parent down. */
static void check_container(const CyberiadaNode* node, int bounded, double w, double h)
{
	const CyberiadaNode *a, *b;

	if (node->geometry_rect) {
		bounded = 1;
		w = node->geometry_rect->width;
		h = node->geometry_rect->height;
	}

	for (a = node->children; a; a = a->next) {
		if (a->geometry_rect) {
			TEST_ASSERT(a->geometry_rect->width > 0.0);
			TEST_ASSERT(a->geometry_rect->height > 0.0);
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

static CyberiadaDocument* load(const char* filename)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown,
										   CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->geometry_format == cybgeomShort);
	/* pin the coordinates to the frame of the file */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
													coordLeftTop, edgeBorder) ==
				CYBERIADA_NO_ERROR);
	return doc;
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode *node, *other;

	/* the shrunk appendix Г.4 example: the sizes are derived, the corners are kept */
	doc = load("diagrams/short-geometry.graphml");
	check_container(doc->state_machines->nodes, 0, 0.0, 0.0);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "node-0-0-1");
	TEST_ASSERT(node && node->geometry_rect);
	TEST_ASSERT(fabs(node->geometry_rect->x - 50.0) < EPS);
	TEST_ASSERT(fabs(node->geometry_rect->y - 90.0) < EPS);
	/* the label of the outgoing transition sits at x = 375 of the source */
	TEST_ASSERT(node->geometry_rect->width <= 365.0 + EPS);

	/* the composite stops short of the sibling the file placed beside it */
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "node-0-0");
	other = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "node-0-1");
	TEST_ASSERT(node && node->geometry_rect && other && other->geometry_rect);
	TEST_ASSERT(node->geometry_rect->x + node->geometry_rect->width <=
				other->geometry_rect->x + EPS);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the robot-vacuum example of the standard */
	doc = load("samples/standard-hoover.graphml");
	check_container(doc->state_machines->nodes, 0, 0.0, 0.0);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0::n2");
	TEST_ASSERT(node && node->geometry_rect);
	TEST_ASSERT(fabs(node->geometry_rect->x - 50.0) < EPS);
	TEST_ASSERT(fabs(node->geometry_rect->y - 550.0) < EPS);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
