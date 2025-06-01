/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The geometry utilities
 *
 * Copyright (C) 2024-2025 Alexey Fedoseev <aleksey@fedoseev.net>
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
#include "cyb_error.h"

int cyberiada_document_no_geometry(CyberiadaDocument* doc)
{
	if (!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	doc->geometry_format = cybgeomNone;
	doc->node_coord_format = coordNone;
	doc->edge_coord_format = coordNone;
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
	
	if (!node || !(node->id)) {
		return NULL;
	}
	if (node->type == cybNodeSM) {
		type = htTree;
	} else if (node->type == cybNodeCompositeState || node->type == cybNodeRegion) {
		type = htCompositeNode;
	} else if (node->type & (cybNodeSimpleState | cybNodeChoice | cybNodeComment | cybNodeFormalComment)) {
		type = htSimpleNode;
	} else if (node->type & (cybNodeInitial | cybNodeFinal | cybNodeTerminate)) {
		type = htPoint;
	} else {
		ERROR("Cannot convert node to htree, bad type: %d\n", node->type);
		return NULL;
	}
	t_node = htree_new_node(type, node->id);
	if (node->geometry_point) {
		t_node->point = htree_copy_point(node->geometry_point);
	}
	if (node->geometry_rect) {
		t_node->rect = htree_copy_rect(node->geometry_rect);
	}
	if (node->children) {
		for (child = node->children; child; child = child->next) {
			t_child = cyberiada_node_to_htree(child);
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
	t_edge = htree_new_edge(edge->id, edge->source_id, edge->target_id);
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
		if (tree->nodes) {
			HTreeNode* n = tree->nodes;
			while (n->next) n = n->next;
			n->next = t_node;
		} else {
			tree->nodes = t_node;
		}
	}

	for (edge = sm->edges; edge; edge = edge->next) {
		HTreeEdge* t_edge = cyberiada_edge_to_htree(edge);
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

	if (file_format == cybxmlYED) {
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
	
	if (file_format == cybxmlYED) {
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

	cyberiada_clean_document_geometry(doc);
	
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

int cyberiada_check_nodes_geometry(CyberiadaNode* nodes)
{
	CyberiadaNode* n;

	for (n = nodes; n; n = n->next) {
		if (n->type == cybNodeInitial || n->type == cybNodeFinal || n->type == cybNodeTerminate) {
			if (n->geometry_rect) {
				ERROR("Point node %s has rect geometry\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;
			}
		} else if (n->type == cybNodeSM || n->type == cybNodeSimpleState || n->type == cybNodeCompositeState ||
				   n->type == cybNodeSubmachineState || n->type == cybNodeChoice) {
			if (n->geometry_point) {
				ERROR("Rect (node %s) has point geometry\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;
			}
			if (n->geometry_rect && (n->geometry_rect->width == 0.0 && n->geometry_rect->height)) {
				ERROR("Rect (node %s) has zero width & height\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;				
			}
		}
		if (n->children) {
			int res = cyberiada_check_nodes_geometry(n->children);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	
	return CYBERIADA_NO_ERROR;	
}
