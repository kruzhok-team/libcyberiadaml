/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The document geometry test
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

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode* node;
	CyberiadaEdge* edge;

	/* the geometry is imported by default */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);

	/* format conversion keeps the geometry */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute,
													coordAbsolute, coordAbsolute,
													edgeCenter) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(doc->node_coord_format == coordAbsolute);

	/* reconstruction rebuilds the geometry from scratch */
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);

	/* cleaning removes the geometry */
	TEST_ASSERT(cyberiada_clean_document_geometry(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 0);

	/* reconstruction works on a cleaned document */
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the skip-geometry flag drops the geometry on import */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_SKIP_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the reconstruction flag builds geometry for a document without one */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/actions.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY |
										   CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the edge label rect is converted with the rest of the geometry
	   and extends the document bounding rect */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/label-geometry.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->bounding_rect);
	TEST_ASSERT(fabs(doc->bounding_rect->width - 1000.0) < 0.01);
	TEST_ASSERT(fabs(doc->bounding_rect->height - 760.0) < 0.01);
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute,
													coordAbsolute, coordAbsolute,
													edgeBorder) ==
				CYBERIADA_NO_ERROR);
	edge = doc->state_machines->edges;
	while (edge && strcmp(edge->id, "n0-n1") != 0) edge = edge->next;
	TEST_ASSERT(edge);
	TEST_ASSERT(edge->geometry_label_rect);
	/* the label is bound to the top left corner of the source node (50; 50) */
	TEST_ASSERT(fabs(edge->geometry_label_rect->x - 950.0) < 0.01);
	TEST_ASSERT(fabs(edge->geometry_label_rect->y - 750.0) < 0.01);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the reconstruction flag drops the label geometry of a comment edge */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/comment-edge-label.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	edge = doc->state_machines->edges;
	while (edge && strcmp(edge->id, "cX-n0") != 0) edge = edge->next;
	TEST_ASSERT(edge);
	TEST_ASSERT(edge->geometry_label_rect == NULL);
	TEST_ASSERT(edge->geometry_label_point == NULL);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the reconstruction flag repairs malformed node geometry */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/point-comment.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "cX");
	TEST_ASSERT(node);
	TEST_ASSERT(node->geometry_point == NULL);
	TEST_ASSERT(node->geometry_rect != NULL);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
