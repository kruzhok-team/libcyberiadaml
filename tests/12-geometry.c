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
#include <string.h>
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

	/* reconstruction rebuilds the geometry from scratch: the layered layout
	   runs the flow down from the initial pseudostate */
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	{
		CyberiadaNode* init = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "init");
		CyberiadaNode* off = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
		CyberiadaNode* on = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n1");
		TEST_ASSERT(init && init->geometry_point && off && off->geometry_rect && on && on->geometry_rect);
		/* the machine level of a chain runs left-to-right (the wide target) */
		TEST_ASSERT(init->geometry_point->x < off->geometry_rect->x);
		TEST_ASSERT(off->geometry_rect->x + off->geometry_rect->width <= on->geometry_rect->x);
		TEST_ASSERT(off->geometry_rect->width >= 300.0 && off->geometry_rect->height >= 200.0);
		for (edge = doc->state_machines->edges; edge; edge = edge->next) {
			if (strcmp(edge->id, "n0-n1") == 0) {
				TEST_ASSERT(edge->geometry_label_rect && edge->geometry_label_rect->width > 0.0);
			}
		}
		/* the update keeps the drawn elements and lays out the cleared one */
		{
			CyberiadaRect off_rect = *off->geometry_rect;
			CyberiadaPoint init_point = *init->geometry_point;
			htree_destroy_rect(on->geometry_rect);
			on->geometry_rect = NULL;
			TEST_ASSERT(cyberiada_update_document_geometry(doc, 1) == CYBERIADA_NO_ERROR);
			TEST_ASSERT(off->geometry_rect && on->geometry_rect && init->geometry_point);
			TEST_ASSERT(fabs(off->geometry_rect->x - off_rect.x) < 0.001 &&
						fabs(off->geometry_rect->y - off_rect.y) < 0.001);
			TEST_ASSERT(fabs(init->geometry_point->x - init_point.x) < 0.001 &&
						fabs(init->geometry_point->y - init_point.y) < 0.001);
			TEST_ASSERT(on->geometry_rect->x > off->geometry_rect->x);
		}
		/* a drawn transition without a label rect gets none in the update */
		for (edge = doc->state_machines->edges; edge; edge = edge->next) {
			if (strcmp(edge->id, "n0-n1") == 0) break;
		}
		TEST_ASSERT(edge && edge->geometry_label_rect && edge->geometry_source_point);
		htree_destroy_rect(edge->geometry_label_rect);
		edge->geometry_label_rect = NULL;
		TEST_ASSERT(cyberiada_update_document_geometry(doc, 1) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(edge->geometry_source_point && !edge->geometry_label_rect);
	}

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

	/* the metainformation node stays out of the reconstructed layout (6.9) */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY |
										   CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "nMeta");
	TEST_ASSERT(node);
	TEST_ASSERT(node->geometry_rect == NULL);
	TEST_ASSERT(node->geometry_point == NULL);
	/* the SM covers the states only, not a shelf slot for the meta node */
	node = doc->state_machines->nodes;
	TEST_ASSERT(node && node->geometry_rect);
	TEST_ASSERT(fabs(node->geometry_rect->width - 360.0) < 0.01);
	TEST_ASSERT(fabs(node->geometry_rect->height - 120.0) < 0.01);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* an edge incident to the metainformation node is left out with it */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/meta-comment-edge.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "nMeta");
	TEST_ASSERT(node && node->geometry_rect == NULL);
	/* the edges after it keep their place in the copy back */
	edge = doc->state_machines->edges;
	while (edge && strcmp(edge->id, "n0-n1") != 0) edge = edge->next;
	TEST_ASSERT(edge && edge->geometry_source_point);
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

	/* the comment sizes: the default comment box for a short note, the body
	   wrapped at 40 characters for a long one, the hidden component comment
	   left without geometry, every comment inside the machine border */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/comment-sizes.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
													coordAbsolute, edgeBorder) == CYBERIADA_NO_ERROR);
	{
		CyberiadaNode* sm = doc->state_machines->nodes;
		CyberiadaNode* short_note = cyberiada_graph_find_node_by_id(sm, "short");
		CyberiadaNode* long_note = cyberiada_graph_find_node_by_id(sm, "long");
		CyberiadaNode* component = cyberiada_graph_find_node_by_id(sm, "comp");
		TEST_ASSERT(sm && sm->geometry_rect && short_note && long_note && component);
		TEST_ASSERT(short_note->geometry_rect && short_note->geometry_rect->width == 200.0 &&
					short_note->geometry_rect->height == 100.0);
		TEST_ASSERT(long_note->geometry_rect && long_note->geometry_rect->width <= 40 * 10.0 + 16.0 &&
					long_note->geometry_rect->width > 300.0 && long_note->geometry_rect->height > 100.0);
		TEST_ASSERT(component->geometry_rect == NULL && component->geometry_point == NULL);
		TEST_ASSERT(short_note->geometry_rect->x >= sm->geometry_rect->x &&
					short_note->geometry_rect->x + short_note->geometry_rect->width <=
					sm->geometry_rect->x + sm->geometry_rect->width &&
					long_note->geometry_rect->y + long_note->geometry_rect->height <=
					sm->geometry_rect->y + sm->geometry_rect->height);
		for (edge = doc->state_machines->edges; edge; edge = edge->next) {
			if (strcmp(edge->id, "comp-n1") == 0) {
				TEST_ASSERT(edge->geometry_source_point == NULL && edge->geometry_target_point == NULL);
			}
		}
	}
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
