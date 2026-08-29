/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The geometry utilities
 *
 * Copyright (C) 2024-2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "geometry.h"
#include "cyb_graph.h"
#include "cyb_error.h"

/* the base format sizes the rect by the node content (7.2.2);
   the nominal size is used until the consumer lays the diagram out */
#define CYBERIADA_LOOSE_NODE_WIDTH   300.0
#define CYBERIADA_LOOSE_NODE_HEIGHT  200.0
#define CYBERIADA_LOOSE_PADDING       10.0
#define CYBERIADA_LOOSE_MIN_SIZE      20.0

/* the metainformation node is not a displayed element (6.9), so it takes no
   part in the layout unless the document gives it geometry of its own */
static int cyberiada_geometry_skip_node(const CyberiadaNode* node)
{
	return (cyberiada_graph_node_is_meta(node) &&
			!node->geometry_rect && !node->geometry_point);
}

static int cyberiada_geometry_skip_edge(const CyberiadaEdge* edge)
{
	return (edge && (cyberiada_geometry_skip_node(edge->source) ||
					 cyberiada_geometry_skip_node(edge->target)));
}

int cyberiada_document_no_geometry(CyberiadaDocument* doc)
{
	if (!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	doc->geometry_format = cybgeomNone;
	doc->node_coord_format = coordNone;
	doc->edge_coord_format = coordNone;
	doc->edge_pl_coord_format = coordNone;
	doc->edge_geom_format = edgeNone;

	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_clean_nodes_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->geometry_point) {
			htree_destroy_point(node->geometry_point);
			node->geometry_point = NULL;
		}
		if (node->geometry_rect) {
			htree_destroy_rect(node->geometry_rect);
			node->geometry_rect = NULL;
		}
		if (node->children) {
			int res = cyberiada_clean_nodes_geometry(node->children);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_clean_edge_geometry(CyberiadaEdge* edge)
{
	if (edge->geometry_polyline) {
		htree_destroy_polyline(edge->geometry_polyline);
		edge->geometry_polyline = NULL;
	}
	if (edge->geometry_source_point) {
		htree_destroy_point(edge->geometry_source_point);
		edge->geometry_source_point = NULL;
		}
	if (edge->geometry_target_point) {
		htree_destroy_point(edge->geometry_target_point);
		edge->geometry_target_point = NULL;
	}
	if (edge->geometry_label_point) {
		htree_destroy_point(edge->geometry_label_point);
		edge->geometry_label_point = NULL;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_clean_edges_geometry(CyberiadaEdge* edge)
{
	while (edge) {
		cyberiada_clean_edge_geometry(edge);
		edge = edge->next;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_clean_document_geometry(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;

	if (!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_clean_nodes_geometry(sm->nodes);
		cyberiada_clean_edges_geometry(sm->edges);
	}

	if (doc->bounding_rect) {
		htree_destroy_rect(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}
	
	cyberiada_document_no_geometry(doc);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_round_node_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->children) {
			if (node->geometry_point) {
				htree_round_point(node->geometry_point, 0);
			}
			if (node->geometry_rect) {
				htree_round_rect(node->geometry_rect, 0);
			}
			cyberiada_round_node_geometry(node->children);
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_round_edges_geometry(CyberiadaEdge* edges)
{
	while (edges) {
		if (edges->geometry_source_point) {
			htree_round_point(edges->geometry_source_point, 0);
		}
		if (edges->geometry_target_point) {
			htree_round_point(edges->geometry_target_point, 0);
		}
		if (edges->geometry_label_point) {
			htree_round_point(edges->geometry_label_point, 0);
		}
		if (edges->geometry_polyline) {
			CyberiadaPolyline* pl = edges->geometry_polyline; 
			while (pl) {
				htree_round_point(&(pl->point), 0);
				pl = pl->next;
			}
		}
		edges = edges->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_round_document_geometry(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;
	if (!doc || !doc->state_machines) {
		ERROR("Cannot round SM document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	htree_round_rect(doc->bounding_rect, 0);
	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_round_node_geometry(sm->nodes);
		cyberiada_round_edges_geometry(sm->edges);
	}
	return CYBERIADA_NO_ERROR;
}

static HTreeNode* cyberiada_node_to_htree(CyberiadaNode* node)
{
	HTNodeType type;
	HTreeNode *t_node, *t_child, *n;
	CyberiadaNode *child;
	
	if (!node || !(node->id) || cyberiada_geometry_skip_node(node)) {
		return NULL;
	}
	if (node->type == cybNodeSM) {
		type = htTree;
	} else if (node->type == cybNodeCompositeState || node->type == cybNodeRegion) {
		type = htCompositeNode;
	} else if (node->type & (cybNodeSimpleState | cybNodeSubmachineState | cybNodeChoice |
							 cybNodeComment | cybNodeFormalComment)) {
		type = htSimpleNode;
	} else if (node->type & (cybNodeInitial | cybNodeFinal | cybNodeTerminate |
							 cybNodeEntryPoint | cybNodeExitPoint |
							 cybNodeShallowHistory | cybNodeDeepHistory |
							 cybNodeFork | cybNodeJoin)) {
		type = htPoint;
	} else {
		ERROR("Cannot convert node to htree, bad type: %d\n", node->type);
		return NULL;
	}
	t_node = htree_new_node(type, node->id);
	if (!t_node) {
		ERROR("Cannot create new node\n");
		return NULL;
	}
	if (node->geometry_point) {
		t_node->point = htree_copy_point(node->geometry_point);
		if (!t_node->point) {
			ERROR("Cannot copy a point\n");
			htree_destroy_node(t_node);
			return NULL;
		}
	}
	if (node->geometry_rect) {
		t_node->rect = htree_copy_rect(node->geometry_rect);
		if (!t_node->rect) {
			ERROR("Cannot copy a rect\n");
			htree_destroy_node(t_node);
			return NULL;
		}
	}
	if (node->children) {
		for (child = node->children; child; child = child->next) {
			t_child = cyberiada_node_to_htree(child);
			if (!t_child) continue;
			t_child->parent = t_node;
			if (t_node->children) {
				n = t_node->children;
				while (n->next) n = n->next;
				n->next = t_child;
			} else {
				t_node->children = t_child;
			}
		}
	}
	return t_node;
}

static HTreeEdge* cyberiada_edge_to_htree(CyberiadaEdge* edge)
{
	HTreeEdge* t_edge;	
	if (!edge) {
		return NULL;
	}
	t_edge = htree_new_edge(edge->id, edge->source_id, edge->target_edge ? edge->target_edge->target_id : edge->target_id);
	if (edge->geometry_polyline) {
		t_edge->polyline = htree_copy_polyline(edge->geometry_polyline);
	}
	if (edge->geometry_source_point) {
		t_edge->source_point = htree_copy_point(edge->geometry_source_point);
	}
	if (edge->geometry_target_point) {
		t_edge->target_point = htree_copy_point(edge->geometry_target_point);
	}
	if (edge->geometry_label_point) {
		t_edge->label_point = htree_copy_point(edge->geometry_label_point);
	}
	if (edge->geometry_label_rect) {
		t_edge->label_rect = htree_copy_rect(edge->geometry_label_rect);
	}
	return t_edge;
}

static HTree* cyberiada_sm_to_htree(CyberiadaSM* sm)
{
	CyberiadaNode* node;
	CyberiadaEdge* edge;
	HTree* tree;
	HTreeEdge* t_edge;

	if (!sm) {
		return NULL;
	}
	
	tree = htree_new_tree();
	
	for (node = sm->nodes; node; node = node->next) {
		HTreeNode* t_node = cyberiada_node_to_htree(node);
		if (!t_node) continue;
		if (tree->nodes) {
			HTreeNode* n = tree->nodes;
			while (n->next) n = n->next;
			n->next = t_node;
		} else {
			tree->nodes = t_node;
		}
	}

	for (edge = sm->edges; edge; edge = edge->next) {
		HTreeEdge* t_edge;
		if (cyberiada_geometry_skip_edge(edge)) continue;
		t_edge = cyberiada_edge_to_htree(edge);
		if (tree->edges) {
			HTreeEdge* e = tree->edges;
			while (e->next) e = e->next;
			e->next = t_edge;
		} else {
			tree->edges = t_edge;
		}
	}

	t_edge = tree->edges;
	while (t_edge) {
		HTreeNode* source = htree_find_node_by_id(tree->nodes, t_edge->source_id);
		HTreeNode* target = htree_find_node_by_id(tree->nodes, t_edge->target_id);
		if (!source || !target) {
			ERROR("Cannot find htree node by id ('%s', '%s')\n", t_edge->source_id, t_edge->target_id);
			htree_destroy_tree(tree);
			return NULL;
		}
		t_edge->source = source;
		t_edge->target = target;
		t_edge = t_edge->next;
	}
	
	return tree;
}

static HTDocument* cyberiada_to_htree_geometry(CyberiadaDocument* cyb_doc)
{
	HTDocument* htg_doc;
	HTree *tree, *prev = NULL; 
	CyberiadaSM* sm;
	
	if (!cyb_doc) {
		return NULL;
	}

	htg_doc = htree_new_document(cyb_doc->node_coord_format,
								 cyb_doc->edge_coord_format,
								 cyb_doc->edge_pl_coord_format,
								 cyb_doc->edge_geom_format);
	if (cyb_doc->bounding_rect) {
		htg_doc->bounding_rect = htree_copy_rect(cyb_doc->bounding_rect);
	}
	
	for (sm = cyb_doc->state_machines; sm; sm = sm->next) {
		tree = cyberiada_sm_to_htree(sm);
		if (prev) {
			prev->next = tree;
		} else {
			htg_doc->trees = tree;
		}
		prev = tree;
	}

	return htg_doc;
}

static int cyberiada_update_nodes_geometry(CyberiadaNode* nodes, HTreeNode* tree_nodes)
{
	CyberiadaNode* node;
	HTreeNode* t_node;

	if (!nodes || !tree_nodes) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (node = nodes, t_node = tree_nodes;
		 node && t_node;
		 node = node->next, t_node = t_node->next) {
		//DEBUG("update nodes %s -> %s\n", t_node->id, node->id);

		while (node && cyberiada_geometry_skip_node(node)) node = node->next;
		if (!node) break;
		
		if (node->id && t_node->id && strcmp(node->id, t_node->id) != 0) {
			ERROR("Node IDs don't match %s %s\n", node->id, t_node->id);
			return CYBERIADA_BAD_PARAMETER;
		}

		if (node->geometry_point && t_node->point) {
			htree_set_point(node->geometry_point, t_node->point);
		} else {
			if (node->geometry_point) {
				htree_destroy_point(node->geometry_point);
				node->geometry_point = NULL;
			}
			if (t_node->point) {
				node->geometry_point = htree_copy_point(t_node->point);
			}
		}

		if (node->geometry_rect && t_node->rect) {
			htree_set_rect(node->geometry_rect, t_node->rect);
		} else {
			if (node->geometry_rect) {
				htree_destroy_rect(node->geometry_rect);
				node->geometry_rect = NULL;
			}
			if (t_node->rect) {
				node->geometry_rect = htree_copy_rect(t_node->rect);
			}
		}
/*		if (node->geometry_rect) {
			DEBUG("updated node %s rect (%lf; %lf; %lf; %lf)\n",
				  node->id, node->geometry_rect->x, node->geometry_rect->y, node->geometry_rect->width, node->geometry_rect->height);
		} else {
			DEBUG("empty node %s rect\n", node->id);
		}*/
		
		if (node->children && t_node->children) {
			cyberiada_update_nodes_geometry(node->children, t_node->children);
		}	 
	}
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_update_edge_geometry(CyberiadaEdge* edge, HTreeEdge* tree_edge)
{
	if (!edge || !tree_edge) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (edge->id && tree_edge->id && strcmp(edge->id, tree_edge->id) != 0) {
		ERROR("Edge IDs don't match %s %s\n", edge->id, tree_edge->id);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (edge->geometry_polyline && tree_edge->polyline) {
		htree_set_polyline(edge->geometry_polyline, tree_edge->polyline);
	} else {
		if (edge->geometry_polyline) {
			htree_destroy_polyline(edge->geometry_polyline);
			edge->geometry_polyline = NULL;
		}
		if (tree_edge->polyline) {
			edge->geometry_polyline = htree_copy_polyline(tree_edge->polyline);
		}
	}

	if (edge->geometry_source_point && tree_edge->source_point) {
		htree_set_point(edge->geometry_source_point, tree_edge->source_point);
	} else {
		if (edge->geometry_source_point) {
			htree_destroy_point(edge->geometry_source_point);
			edge->geometry_source_point = NULL;
		}
		if (tree_edge->source_point) {
			edge->geometry_source_point = htree_copy_point(tree_edge->source_point);
		}
	}

	if (edge->geometry_target_point && tree_edge->target_point) {
		htree_set_point(edge->geometry_target_point, tree_edge->target_point);
	} else {
		if (edge->geometry_target_point) {
			htree_destroy_point(edge->geometry_target_point);
			edge->geometry_target_point = NULL;
		}
		if (tree_edge->target_point) {	
			edge->geometry_target_point = htree_copy_point(tree_edge->target_point);
		}
	}

	if (edge->geometry_label_point && tree_edge->label_point) {
		htree_set_point(edge->geometry_label_point, tree_edge->label_point);
	} else {
		if (edge->geometry_label_point) {
			htree_destroy_point(edge->geometry_label_point);
			edge->geometry_label_point = NULL;
		}
		if (tree_edge->label_point) {
			edge->geometry_label_point = htree_copy_point(tree_edge->label_point);
		}
	}

	if (edge->geometry_label_rect && tree_edge->label_rect) {
		htree_set_rect(edge->geometry_label_rect, tree_edge->label_rect);
	} else {
		if (edge->geometry_label_rect) {
			htree_destroy_rect(edge->geometry_label_rect);
			edge->geometry_label_rect = NULL;
		}
		if (tree_edge->label_rect) {
			edge->geometry_label_rect = htree_copy_rect(tree_edge->label_rect);
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_update_sm_geometry(CyberiadaSM* sm, HTree* tree)
{
	CyberiadaEdge* edge;
	HTreeEdge* t_edge;

	if (!sm || !tree) {
		return CYBERIADA_BAD_PARAMETER;		
	}

	if (sm->nodes) {
		cyberiada_update_nodes_geometry(sm->nodes, tree->nodes);	
	}
		
	for (edge = sm->edges, t_edge = tree->edges;
		 edge && t_edge;
		 edge = edge->next, t_edge = t_edge->next) {
		while (edge && cyberiada_geometry_skip_edge(edge)) edge = edge->next;
		if (!edge) break;
		cyberiada_update_edge_geometry(edge, t_edge);
	}	
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_update_geometry(CyberiadaDocument* cyb_doc, HTDocument* htg_doc)
{
	HTree *tree; 
	CyberiadaSM* sm;

	if (!cyb_doc || !htg_doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	cyb_doc->node_coord_format = htg_doc->node_coord_format;
	cyb_doc->edge_coord_format = htg_doc->edge_coord_format;
	cyb_doc->edge_pl_coord_format = htg_doc->edge_pl_coord_format;
	cyb_doc->edge_geom_format = htg_doc->edge_format;

	if (cyb_doc->bounding_rect && htg_doc->bounding_rect) {
		htree_set_rect(cyb_doc->bounding_rect, htg_doc->bounding_rect);
	} else {
		if (cyb_doc->bounding_rect) {
			htree_destroy_rect(cyb_doc->bounding_rect);
			cyb_doc->bounding_rect = NULL;
		}
		if (htg_doc->bounding_rect) {
			cyb_doc->bounding_rect = htree_copy_rect(htg_doc->bounding_rect);
		}
	}
	for (sm = cyb_doc->state_machines, tree = htg_doc->trees;
		 sm && tree;
		 sm = sm->next, tree = tree->next) {

		cyberiada_update_sm_geometry(sm, tree);
	}
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
										CyberiadaGeometryCoordFormat new_node_coord_format,
										CyberiadaGeometryCoordFormat new_edge_coord_format,
										CyberiadaGeometryCoordFormat new_edge_pl_coord_format,
										CyberiadaGeometryEdgeFormat new_edge_format)
{
	int res;
	HTDocument* htreegeom = cyberiada_to_htree_geometry(doc);
	
	if (!htreegeom) {
		ERROR("Cannot convert document geometry to htree geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if ((res = htree_convert_document_geometry(htreegeom,
											   new_node_coord_format,
											   new_edge_coord_format,
											   new_edge_pl_coord_format,
											   new_edge_format)) != HTREE_OK) {
		ERROR("Error while converting document geometry %d\n", res);
		htree_destroy_document(htreegeom);
		return CYBERIADA_BAD_PARAMETER;
	}

	cyberiada_update_geometry(doc, htreegeom);
	htree_destroy_document(htreegeom);
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_import_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	int res, geom_flags;
	CyberiadaGeometryCoordFormat old_node_coord_format, old_edge_coord_format, old_edge_pl_coord_format;
	CyberiadaGeometryCoordFormat new_node_coord_format, new_edge_coord_format, new_edge_pl_coord_format;
	CyberiadaGeometryEdgeFormat old_edge_format, new_edge_format;
	HTDocument* htreegeom;
	
	if (!doc) {
		ERROR("Cannot import document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (CYBERIADA_FORMAT_IS_YED(file_format)) {
		old_node_coord_format = coordAbsolute;
		old_edge_coord_format = coordLocalCenter;
		old_edge_pl_coord_format = coordAbsolute;		
		old_edge_format = edgeCenter;
	} else if (file_format == cybxmlCyberiada10) {
		old_node_coord_format = coordLeftTop;
		old_edge_coord_format = coordLeftTop;
		old_edge_pl_coord_format = coordLeftTop;		
		old_edge_format = edgeBorder;
	} else {
		ERROR("Bad XML format %d\n", file_format);
		return CYBERIADA_BAD_PARAMETER;
	}
	
	geom_flags = flags & CYBERIADA_FLAG_NODES_GEOMETRY;
	if (geom_flags) {
		if (geom_flags == CYBERIADA_FLAG_NODES_ABSOLUTE_GEOMETRY) {
			new_node_coord_format = coordAbsolute;
		} else if (geom_flags == CYBERIADA_FLAG_NODES_LEFTTOP_LOCAL_GEOMETRY) {
			new_node_coord_format = coordLeftTop;
		} else if (geom_flags == CYBERIADA_FLAG_NODES_CENTER_LOCAL_GEOMETRY) {
			new_node_coord_format = coordLocalCenter;
		} else {
			ERROR("More than one nodes geometry coordinates flag was used for import\n");
			return CYBERIADA_BAD_PARAMETER;		
		}
	} else {
		ERROR("No nodes geometry coordinates flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	geom_flags = flags & CYBERIADA_FLAG_EDGES_GEOMETRY;
	if (geom_flags) {
		if (geom_flags == CYBERIADA_FLAG_EDGES_ABSOLUTE_GEOMETRY) {
			new_edge_coord_format = coordAbsolute;
		} else if (geom_flags == CYBERIADA_FLAG_EDGES_LEFTTOP_LOCAL_GEOMETRY) {
			new_edge_coord_format = coordLeftTop;
		} else if (geom_flags == CYBERIADA_FLAG_EDGES_CENTER_LOCAL_GEOMETRY) {
			new_edge_coord_format = coordLocalCenter;
		} else {
			ERROR("More than one edges geometry coordinates flag was used for import\n");
			return CYBERIADA_BAD_PARAMETER;		
		}
	} else {
		ERROR("No edges geometry coordinates flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	geom_flags = flags & CYBERIADA_FLAG_EDGES_PL_GEOMETRY;
	if (geom_flags) {
		if (geom_flags == CYBERIADA_FLAG_EDGES_PL_ABSOLUTE_GEOMETRY) {
			new_edge_pl_coord_format = coordAbsolute;
		} else if (geom_flags == CYBERIADA_FLAG_EDGES_PL_LEFTTOP_LOCAL_GEOMETRY) {
			new_edge_pl_coord_format = coordLeftTop;
		} else if (geom_flags == CYBERIADA_FLAG_EDGES_PL_CENTER_LOCAL_GEOMETRY) {
			new_edge_pl_coord_format = coordLocalCenter;
		} else {
			ERROR("More than one edges polyline geometry coordinates flag was used for import\n");
			return CYBERIADA_BAD_PARAMETER;		
		}
	} else {
		ERROR("No edges polyline geometry coordinates flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	geom_flags = flags & CYBERIADA_FLAG_EDGE_TYPE_GEOMETRY; 
	if (geom_flags == CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY) {
		new_edge_format = edgeBorder;
	} else if (geom_flags == CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY) {
		new_edge_format = edgeCenter;
	} else if (geom_flags) {
		ERROR("More than one edge geometry flag was used for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	} else {
		ERROR("No edge geometry flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	doc->node_coord_format = old_node_coord_format;
	doc->edge_coord_format = old_edge_coord_format;
	doc->edge_pl_coord_format = old_edge_pl_coord_format;
	doc->edge_geom_format = old_edge_format;
	
	htreegeom = cyberiada_to_htree_geometry(doc);
	
	if (!htreegeom) {
		ERROR("Cannot convert document geometry to htree geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (flags & (CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY | CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY)) {
		if ((res = htree_reconstruct_document_geometry(htreegeom,
													   flags & CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY)) != HTREE_OK) {
			ERROR("Error while reconstructing htree geometry %d\n", res);
			htree_destroy_document(htreegeom);
			return CYBERIADA_BAD_PARAMETER;
		}
	}

	if ((res = htree_convert_document_geometry(htreegeom,
											   new_node_coord_format,
											   new_edge_coord_format,
											   new_edge_pl_coord_format,
											   new_edge_format)) != HTREE_OK) {
		ERROR("Error while converting document geometry %d\n", res);
		htree_destroy_document(htreegeom);
		return CYBERIADA_BAD_PARAMETER;
	}

	cyberiada_update_geometry(doc, htreegeom);
	htree_destroy_document(htreegeom);
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_export_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	int res;
	CyberiadaGeometryCoordFormat to_node_coord_format, to_edge_coord_format, to_edge_pl_coord_format;
	CyberiadaGeometryEdgeFormat to_edge_format;
	HTDocument* htreegeom;
	
	if (!doc) {
		ERROR("Cannot export document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (CYBERIADA_FORMAT_IS_YED(file_format)) {
		to_node_coord_format = coordAbsolute;
		to_edge_coord_format = coordLocalCenter;
		to_edge_pl_coord_format = coordAbsolute;
		to_edge_format = edgeCenter;
	} else if (file_format == cybxmlCyberiada10) {
		to_node_coord_format = to_edge_coord_format = to_edge_pl_coord_format = coordLeftTop;
		to_edge_format = edgeBorder;
	} else {
		ERROR("Bad XML format %d\n", file_format);
		return CYBERIADA_BAD_PARAMETER;
	}

	htreegeom = cyberiada_to_htree_geometry(doc);
	if (!htreegeom) {
		ERROR("Cannot convert document geometry to htree geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if ((res = htree_convert_document_geometry(htreegeom,
											   to_node_coord_format,
											   to_edge_coord_format,
											   to_edge_pl_coord_format,
											   to_edge_format)) != HTREE_OK) {
		ERROR("Error while converting document geometry %d\n", res);
		htree_destroy_document(htreegeom);
		return CYBERIADA_BAD_PARAMETER;
	}

	cyberiada_update_geometry(doc, htreegeom);
	htree_destroy_document(htreegeom);
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}
	
	return CYBERIADA_NO_ERROR;
}


int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc, int reconstruct_sm)
{
	int res;
	HTDocument* htreegeom;
	CyberiadaGeometryCoordFormat node_format, edge_format, edge_pl_format;
	CyberiadaGeometryEdgeFormat edge_geom_format;

	if (!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	/* the geometry clean resets the format fields - keep the caller's
	   formats so the reconstructed geometry comes back converted;
	   fall back to the default formats if the document was cleaned before */
	node_format = doc->node_coord_format;
	edge_format = doc->edge_coord_format;
	edge_pl_format = doc->edge_pl_coord_format;
	edge_geom_format = doc->edge_geom_format;
	if (node_format == coordNone) {
		node_format = coordLocalCenter;
	}
	if (edge_format == coordNone) {
		edge_format = coordLocalCenter;
	}
	if (edge_pl_format == coordNone) {
		edge_pl_format = coordLocalCenter;
	}
	if (edge_geom_format == edgeNone) {
		edge_geom_format = edgeBorder;
	}

	cyberiada_clean_document_geometry(doc);

	doc->node_coord_format = node_format;
	doc->edge_coord_format = edge_format;
	doc->edge_pl_coord_format = edge_pl_format;
	doc->edge_geom_format = edge_geom_format;
	
	htreegeom = cyberiada_to_htree_geometry(doc);
	if (!htreegeom) {
		ERROR("Cannot convert document geometry to htree geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if ((res = htree_reconstruct_document_geometry(htreegeom, reconstruct_sm)) != HTREE_OK) {
		ERROR("Error while reconstructing htree geometry %d\n", res);
		htree_destroy_document(htreegeom);
		return CYBERIADA_BAD_PARAMETER;
	}

	cyberiada_update_geometry(doc, htreegeom);
	htree_destroy_document(htreegeom);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_node_has_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->geometry_point || node->geometry_rect) {
			return 1;
		}
		if (node->children) {
			int found = cyberiada_node_has_geometry(node->children);
			if (found) {
				return 1;
			}
		}
		node = node->next;
	}
	return 0;
}

int cyberiada_document_has_geometry(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;
	CyberiadaEdge* edge;

	if (!doc) {
		return 0;
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (cyberiada_node_has_geometry(sm->nodes)) {
			return 1;
		}
		edge = sm->edges;
		while (edge) {
			if (edge->geometry_polyline ||
				edge->geometry_source_point ||
				edge->geometry_target_point ||
				edge->geometry_label_point ||
				edge->geometry_label_rect) {
				
				return 1;
			}
			edge = edge->next;
		}	
	}
	
	return 0;
}

int cyberiada_document_declared_geometry(CyberiadaDocument* doc, CyberiadaGeometryFormat* format)
{
	CyberiadaMetaStringList* s;

	if (!doc || !format) {
		return CYBERIADA_BAD_PARAMETER;
	}

	if (!doc->meta_info) {
		return CYBERIADA_NOT_FOUND;
	}

	for (s = doc->meta_info->strings; s; s = s->next) {
		if (!s->name || !s->value || strcmp(s->name, CYBERIADA_META_GEOMETRY) != 0) {
			continue;
		}
		if (strcmp(s->value, CYBERIADA_META_GEOM_NONE) == 0) {
			*format = cybgeomNone;
		} else if (strcmp(s->value, CYBERIADA_META_GEOM_SHORT) == 0) {
			*format = cybgeomShort;
		} else if (strcmp(s->value, CYBERIADA_META_GEOM_FULL) == 0) {
			*format = cybgeomFull;
		} else {
			break;
		}
		return CYBERIADA_NO_ERROR;
	}

	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_loose_rect(const CyberiadaRect* rect)
{
	return rect && (rect->width == 0.0 || rect->height == 0.0);
}

static void cyberiada_repair_loose_rect(CyberiadaRect* rect)
{
	if (!rect) {
		return;
	}
	if (rect->width == 0.0) {
		rect->width = CYBERIADA_LOOSE_NODE_WIDTH;
	}
	if (rect->height == 0.0) {
		rect->height = CYBERIADA_LOOSE_NODE_HEIGHT;
	}
}

/* The extent of the children in the coordinates of the node; the children
   coordinates are relative to the left top corner of the node (7.2.1) */
static int cyberiada_children_extent(const CyberiadaNode* node, double* w, double* h)
{
	const CyberiadaNode* c;
	int found = 0;

	for (c = node->children; c; c = c->next) {
		double cw, ch;
		if (c->geometry_rect) {
			cw = c->geometry_rect->x + c->geometry_rect->width;
			ch = c->geometry_rect->y + c->geometry_rect->height;
		} else if (c->geometry_point) {
			cw = c->geometry_point->x;
			ch = c->geometry_point->y;
		} else if (c->children) {
			/* a container with no geometry of its own shares the node coordinates */
			cw = ch = 0.0;
			if (!cyberiada_children_extent(c, &cw, &ch)) {
				continue;
			}
		} else {
			continue;
		}
		if (!found || cw > *w) *w = cw;
		if (!found || ch > *h) *h = ch;
		found = 1;
	}

	return found;
}

/* The room the neighbours leave for the node: a sibling anchor or an own
   transition label caps the size on the axis it lies on */
static void cyberiada_loose_bounds(const CyberiadaNode* node, const CyberiadaNode* siblings,
								   const CyberiadaEdge* edges, double* w, double* h)
{
	const CyberiadaNode* s;
	const CyberiadaEdge* e;
	double x = node->geometry_rect->x;
	double y = node->geometry_rect->y;

	for (s = siblings; s; s = s->next) {
		double dx, dy;
		if (s == node || !s->geometry_rect) {
			continue;
		}
		dx = s->geometry_rect->x - x;
		dy = s->geometry_rect->y - y;
		if (dx > 0.0 && dx >= fabs(dy) && dx - CYBERIADA_LOOSE_PADDING < *w) {
			*w = dx - CYBERIADA_LOOSE_PADDING;
		}
		if (dy > 0.0 && dy > fabs(dx) && dy - CYBERIADA_LOOSE_PADDING < *h) {
			*h = dy - CYBERIADA_LOOSE_PADDING;
		}
	}

	/* the label is placed in the coordinates of the source node (7.2.1) */
	for (e = edges; e; e = e->next) {
		double lx, ly;
		if (e->source != node) {
			continue;
		}
		if (e->geometry_label_point) {
			lx = e->geometry_label_point->x;
			ly = e->geometry_label_point->y;
		} else if (e->geometry_label_rect) {
			lx = e->geometry_label_rect->x;
			ly = e->geometry_label_rect->y;
		} else {
			continue;
		}
		if (lx <= 0.0 || ly <= 0.0) {
			continue;
		}
		if (lx >= ly) {
			if (lx - CYBERIADA_LOOSE_PADDING < *w) {
				*w = lx - CYBERIADA_LOOSE_PADDING;
			}
		} else if (ly - CYBERIADA_LOOSE_PADDING < *h) {
			*h = ly - CYBERIADA_LOOSE_PADDING;
		}
	}
}

static void cyberiada_size_loose_node(CyberiadaNode* node, double avail_w, double avail_h)
{
	CyberiadaRect* r = node->geometry_rect;
	double ew = 0.0, eh = 0.0;
	int content = cyberiada_children_extent(node, &ew, &eh);

	if (r->width == 0.0) {
		double w = content ? ew + CYBERIADA_LOOSE_PADDING : CYBERIADA_LOOSE_NODE_WIDTH;
		if (w > avail_w) w = avail_w;
		if (w < CYBERIADA_LOOSE_MIN_SIZE) w = CYBERIADA_LOOSE_MIN_SIZE;
		r->width = w;
	}
	if (r->height == 0.0) {
		double h = content ? eh + CYBERIADA_LOOSE_PADDING : CYBERIADA_LOOSE_NODE_HEIGHT;
		if (h > avail_h) h = avail_h;
		if (h < CYBERIADA_LOOSE_MIN_SIZE) h = CYBERIADA_LOOSE_MIN_SIZE;
		r->height = h;
	}
}

/* Size the loose rects top down (the room left by the neighbours) and
   bottom up (the room the content needs), see 7.2.2 */
static void cyberiada_size_loose_nodes(CyberiadaNode* nodes, const CyberiadaEdge* edges,
									   double avail_w, double avail_h)
{
	CyberiadaNode* n;

	for (n = nodes; n; n = n->next) {
		double w = avail_w, h = avail_h;

		if (n->geometry_rect) {
			w -= n->geometry_rect->x;
			h -= n->geometry_rect->y;
			cyberiada_loose_bounds(n, nodes, edges, &w, &h);
		}
		if (n->children) {
			cyberiada_size_loose_nodes(n->children, edges, w, h);
		}
		if (n->geometry_rect) {
			cyberiada_size_loose_node(n, w, h);
		}
	}
}

int cyberiada_size_loose_document_geometry(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;

	if (!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_size_loose_nodes(sm->nodes, sm->edges, HUGE_VAL, HUGE_VAL);
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_check_edges_geometry(CyberiadaEdge* edges)
{
	CyberiadaEdge* e;

	for (e = edges; e; e = e->next) {
		if (e->type == cybEdgeComment &&
			(e->geometry_label_point || e->geometry_label_rect)) {
			ERROR("Comment edge %s has label geometry\n", e->id);
			return CYBERIADA_FORMAT_ERROR;
		}
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_repair_edges_geometry(CyberiadaEdge* edges)
{
	CyberiadaEdge* e;

	for (e = edges; e; e = e->next) {
		if (e->type != cybEdgeComment) {
			continue;
		}
		if (e->geometry_label_point) {
			ERROR("warning: dropping the label point of the comment edge %s\n", e->id);
			htree_destroy_point(e->geometry_label_point);
			e->geometry_label_point = NULL;
		}
		if (e->geometry_label_rect) {
			ERROR("warning: dropping the label rect of the comment edge %s\n", e->id);
			htree_destroy_rect(e->geometry_label_rect);
			e->geometry_label_rect = NULL;
		}
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_repair_nodes_geometry(CyberiadaNode* nodes)
{
	CyberiadaNode* n;

	for (n = nodes; n; n = n->next) {
		if (n->type == cybNodeInitial || n->type == cybNodeFinal || n->type == cybNodeTerminate) {
			if (n->geometry_rect) {
				ERROR("warning: dropping the rect geometry of the point node %s\n", n->id);
				htree_destroy_rect(n->geometry_rect);
				n->geometry_rect = NULL;
			}
		} else if (n->type == cybNodeSM || n->type == cybNodeSimpleState || n->type == cybNodeCompositeState ||
				   n->type == cybNodeSubmachineState || n->type == cybNodeChoice ||
				   n->type == cybNodeComment || n->type == cybNodeFormalComment) {
			if (n->geometry_point) {
				ERROR("warning: dropping the point geometry of the rect node %s\n", n->id);
				htree_destroy_point(n->geometry_point);
				n->geometry_point = NULL;
			}
			if (cyberiada_loose_rect(n->geometry_rect)) {
				ERROR("warning: sizing the loose rect of the node %s\n", n->id);
				cyberiada_repair_loose_rect(n->geometry_rect);
			}
		}
		if (n->children) {
			cyberiada_repair_nodes_geometry(n->children);
		}
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_check_nodes_geometry(CyberiadaNode* nodes, CyberiadaGeometryFormat format)
{
	CyberiadaNode* n;

	for (n = nodes; n; n = n->next) {
		if (n->type == cybNodeInitial || n->type == cybNodeFinal || n->type == cybNodeTerminate) {
			if (n->geometry_rect) {
				ERROR("Point node %s has rect geometry\n", n->id);
				return CYBERIADA_FORMAT_ERROR;
			}
		} else if (n->type == cybNodeSM || n->type == cybNodeSimpleState || n->type == cybNodeCompositeState ||
				   n->type == cybNodeSubmachineState || n->type == cybNodeChoice ||
				   n->type == cybNodeComment || n->type == cybNodeFormalComment) {
			if (n->geometry_point) {
				ERROR("Rect (node %s) has point geometry\n", n->id);
				return CYBERIADA_FORMAT_ERROR;
			}
			if (format == cybgeomFull && cyberiada_loose_rect(n->geometry_rect)) {
				/* only the extended format sizes the rect exactly (9.1) */
				ERROR("Rect (node %s) has zero width or height\n", n->id);
				return CYBERIADA_FORMAT_ERROR;
			}
		}
		if (n->children) {
			int res = cyberiada_check_nodes_geometry(n->children, format);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	
	return CYBERIADA_NO_ERROR;	
}
