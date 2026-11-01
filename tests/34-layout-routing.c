/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library tests
 *
 * The layout options: the orthogonal routes of the reconstruction, the update
 * and the arrange, the polylines the default
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
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

#include <math.h>
#include <string.h>
#include <cyberiadaml.h>
#include "testutils.h"

#define ALIGNED 0.01

static int oblique_run(const CyberiadaPoint* a, const CyberiadaPoint* b)
{
	return fabs(a->x - b->x) > ALIGNED && fabs(a->y - b->y) > ALIGNED;
}

/* the segments of the transitions along no axis, in absolute coordinates */
static int oblique(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;
	int n = 0;
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute, coordAbsolute, coordAbsolute,
													edgeBorder) == CYBERIADA_NO_ERROR);
	for (sm = doc->state_machines; sm; sm = sm->next) {
		CyberiadaEdge* edge;
		for (edge = sm->edges; edge; edge = edge->next) {
			const CyberiadaPoint* prev;
			CyberiadaPolyline* pl;
			if (edge->type == cybEdgeComment || edge->source == edge->target ||
				!edge->geometry_source_point || !edge->geometry_target_point) continue;
			prev = edge->geometry_source_point;
			for (pl = edge->geometry_polyline; pl; pl = pl->next) {
				n += oblique_run(prev, &pl->point);
				prev = &pl->point;
			}
			n += oblique_run(prev, edge->geometry_target_point);
		}
	}
	return n;
}

static CyberiadaDocument* sample(void)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/ostranna-player.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	return doc;
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaLayoutOptions options;

	cyberiada_default_layout_options(&options);
	TEST_ASSERT(options.routing == htRoutingPolyline);

	/* the polylines of the default reconstruction run oblique */
	doc = sample();
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(oblique(doc) > 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the orthogonal routes of the reconstruction, the update, the arrange */
	options.routing = htRoutingOrthogonal;
	doc = sample();
	TEST_ASSERT(cyberiada_reconstruct_document_geometry_with(doc, 1, &options) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(oblique(doc) == 0);
	TEST_ASSERT(cyberiada_update_document_geometry_with(doc, 1, &options) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(oblique(doc) == 0);
	TEST_ASSERT(cyberiada_arrange_document_geometry_with(doc, 1, &options) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(oblique(doc) == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* NULL options are the defaults */
	doc = sample();
	TEST_ASSERT(cyberiada_reconstruct_document_geometry_with(doc, 1, NULL) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(oblique(doc) > 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
