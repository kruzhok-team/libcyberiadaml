/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library tests
 *
 * The bands of a composite: the title and the entry in the header, the exit
 * in the footer at the bottom, the region between them
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

/* the estimate of the test: 20 px a line, 8 px margins - the exit block
   ("exit/", two behavior lines) and the header (the title, "entry/", one
   behavior line) both 3 lines */
#define BAND (3 * 20.0 + 16.0)
#define EPS 0.001

static CyberiadaNode* find(CyberiadaNode* nodes, const char* id)
{
	CyberiadaNode* n;
	for (n = nodes; n; n = n->next) {
		CyberiadaNode* found;
		if (strcmp(n->id, id) == 0) return n;
		if ((found = find(n->children, id)) != NULL) return found;
	}
	return NULL;
}

static int inside(const CyberiadaRect* r, const CyberiadaRect* outer)
{
	return r->x >= outer->x - EPS && r->y >= outer->y - EPS &&
		r->x + r->width <= outer->x + outer->width + EPS &&
		r->y + r->height <= outer->y + outer->height + EPS;
}

int main(void)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	CyberiadaNode *c, *region, *a, *b;
	const CyberiadaRect *rc, *rr;

	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/composite-bands.graphml", cybxmlUnknown,
										   CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute, coordAbsolute, coordAbsolute,
													edgeBorder) == CYBERIADA_NO_ERROR);
	c = find(doc->state_machines->nodes, "c");
	TEST_ASSERT(c && c->geometry_rect && c->children && c->children->type == cybNodeRegion);
	region = c->children;
	a = find(region->children, "c::a");
	b = find(region->children, "c::b");
	TEST_ASSERT(region->geometry_rect && a && a->geometry_rect && b && b->geometry_rect);
	rc = c->geometry_rect;
	rr = region->geometry_rect;
	/* the header above the region, the footer below it */
	TEST_ASSERT(rr->y - rc->y >= BAND - EPS);
	TEST_ASSERT(rc->y + rc->height - (rr->y + rr->height) >= BAND - EPS);
	TEST_ASSERT(inside(a->geometry_rect, rr) && inside(b->geometry_rect, rr));
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
