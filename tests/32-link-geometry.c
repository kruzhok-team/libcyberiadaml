/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The comment link geometry frames test
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

#define EPS 0.001

static int point_is(const CyberiadaPoint* p, double x, double y)
{
	return p && fabs(p->x - x) < EPS && fabs(p->y - y) < EPS;
}

static CyberiadaEdge* find_edge(CyberiadaEdge* edges, const char* id)
{
	for (; edges; edges = edges->next) {
		if (strcmp(edges->id, id) == 0) return edges;
	}
	return NULL;
}

static void clear_points(CyberiadaEdge* e)
{
	if (e->geometry_source_point) {
		htree_destroy_point(e->geometry_source_point);
		e->geometry_source_point = NULL;
	}
	if (e->geometry_target_point) {
		htree_destroy_point(e->geometry_target_point);
		e->geometry_target_point = NULL;
	}
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaEdge *to_transition, *to_state, *chunk;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/link-geometry.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	to_transition = find_edge(doc->state_machines->edges, "c-a-b");
	to_state = find_edge(doc->state_machines->edges, "c-b");
	TEST_ASSERT(to_transition && to_transition->target_edge && to_state);

	/* the link-to-transition target point is relative to the left-top of the
	   transition's source node (PNST 1044 9.1.1): (200; 25) of a lies on the
	   transition path (100; 25) - (300; 25) */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
													coordAbsolute, edgeBorder) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(point_is(to_transition->geometry_source_point, 200.0, 150.0));
	TEST_ASSERT(point_is(to_transition->geometry_target_point, 200.0, 25.0));
	TEST_ASSERT(point_is(to_state->geometry_source_point, 250.0, 175.0));
	TEST_ASSERT(point_is(to_state->geometry_target_point, 350.0, 50.0));

	/* the centre-local frame: from the centre of a, (50; 25) */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter,
													coordLocalCenter, edgeBorder) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(point_is(to_transition->geometry_target_point, 150.0, 0.0));
	TEST_ASSERT(point_is(to_state->geometry_target_point, 0.0, 25.0));

	/* back to the file frame unchanged */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
													coordLeftTop, edgeBorder) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(point_is(to_transition->geometry_source_point, 50.0, 0.0));
	TEST_ASSERT(point_is(to_transition->geometry_target_point, 200.0, 25.0));
	TEST_ASSERT(point_is(to_state->geometry_target_point, 50.0, 50.0));

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the update fills the cleared link ends: on the borders, on the
	   transition path off its arrow, no target point on a fragment */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/link-geometry.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	to_transition = find_edge(doc->state_machines->edges, "c-a-b");
	to_state = find_edge(doc->state_machines->edges, "c-b");
	chunk = find_edge(doc->state_machines->edges, "c-b#1");
	TEST_ASSERT(to_transition && to_state && chunk);
	clear_points(to_transition);
	clear_points(to_state);
	clear_points(chunk);
	TEST_ASSERT(cyberiada_update_document_geometry(doc, 0) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
													coordAbsolute, edgeBorder) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(to_transition->geometry_source_point && to_transition->geometry_target_point);
	TEST_ASSERT(fabs(to_transition->geometry_target_point->y - 25.0) < EPS &&
				to_transition->geometry_target_point->x >= 150.0 - EPS &&
				to_transition->geometry_target_point->x <= 250.0 + EPS);
	TEST_ASSERT(fabs(to_transition->geometry_source_point->y - 150.0) < EPS);
	TEST_ASSERT(to_state->geometry_target_point &&
				fabs(to_state->geometry_target_point->y - 50.0) < EPS);
	TEST_ASSERT(chunk->geometry_source_point && !chunk->geometry_target_point);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the full reconstruction attaches every link */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/link-geometry.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 0) == CYBERIADA_NO_ERROR);
	to_transition = find_edge(doc->state_machines->edges, "c-a-b");
	to_state = find_edge(doc->state_machines->edges, "c-b");
	chunk = find_edge(doc->state_machines->edges, "c-b#1");
	TEST_ASSERT(to_transition->geometry_source_point && to_transition->geometry_target_point);
	TEST_ASSERT(to_state->geometry_source_point && to_state->geometry_target_point);
	TEST_ASSERT(chunk->geometry_source_point && !chunk->geometry_target_point);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
