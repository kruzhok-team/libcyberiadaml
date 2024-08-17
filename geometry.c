/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The geometry utilities
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
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

#define DEFAULT_NODE_SIZE 100

/* -----------------------------------------------------------------------------
 * Basic geometry functions
 * ----------------------------------------------------------------------------- */

CyberiadaPoint* cyberiada_new_point(void)
{
	CyberiadaPoint* p = (CyberiadaPoint*)malloc(sizeof(CyberiadaPoint));
	memset(p, 0, sizeof(CyberiadaPoint));
	return p;
}

CyberiadaPoint* cyberiada_copy_point(CyberiadaPoint* src)
{
	CyberiadaPoint* dst;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_point();
	dst->x = src->x;
	dst->y = src->y;
	return dst;
}

static void cyberiada_update_point(CyberiadaPoint* p, double dx, double dy)
{
	if (!p) return ;
	p->x += dx;
	p->y += dy;
}

static double round_number(double num, unsigned int signs)
{
	unsigned int i;
	double factor = 1.0;
    double value;
	for (i = 0; i < signs; i++) {
		factor *= 10.0;
	}
	value = (int)(num * factor + .5);
    return (double)value / factor;
}

static void cyberiada_round_point(CyberiadaPoint* p, unsigned int signs)
{
	if (!p) return ;
	p->x = round_number(p->x, signs);
	p->y = round_number(p->y, signs);
}

CyberiadaRect* cyberiada_new_rect(void)
{
	CyberiadaRect* r = (CyberiadaRect*)malloc(sizeof(CyberiadaRect));
	memset(r, 0, sizeof(CyberiadaRect));
	return r;
}

CyberiadaRect* cyberiada_copy_rect(CyberiadaRect* src)
{
	CyberiadaRect* dst;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_rect();
	dst->x = src->x;
	dst->y = src->y;
	dst->width = src->width;
	dst->height = src->height;
	return dst;
}

static int cyberiada_empty_rect(CyberiadaRect* r)
{
	if (!r) return 0;
	return r->x == 0.0 && r->y == 0.0 && r->width == 0.0 && r->height == 0.0;
}

int cyberiada_compare_rects(CyberiadaRect* a, CyberiadaRect* b)
{
	if (!a || !b) return -1;
	return a->x != b->x || a->y != b->y || a->width != b->width || a->height != b->height;
}

static void cyberiada_update_rect(CyberiadaRect* r, double dx, double dy)
{
	if (!r) return ;
	r->x += dx;
	r->y += dy;
}

static void cyberiada_round_rect(CyberiadaRect* r, unsigned int signs)
{
	if (!r) return ;
	r->x = round_number(r->x, signs);
	r->y = round_number(r->y, signs);
	r->width = round_number(r->width, signs);
	r->height = round_number(r->height, signs);
}

CyberiadaPolyline* cyberiada_new_polyline(void)
{
	CyberiadaPolyline* pl = (CyberiadaPolyline*)malloc(sizeof(CyberiadaPolyline));
	memset(pl, 0, sizeof(CyberiadaPolyline));
	return pl;
}

CyberiadaPolyline* cyberiada_copy_polyline(CyberiadaPolyline* src)
{
	CyberiadaPolyline *dst = NULL, *prev, *pl;
	if (!src) {
		return NULL;
	}
	do {
		pl = cyberiada_new_polyline();
		pl->point.x = src->point.x;
		pl->point.y = src->point.y;
		if (dst) {
			prev->next = pl;
		} else {
			dst = pl;
		}
		prev = pl;
		src = src->next;
	} while (src);
	return dst;
}

int cyberiada_destroy_polyline(CyberiadaPolyline* polyline)
{
	CyberiadaPolyline* pl;
	do {
		pl = polyline;
		polyline = polyline->next;
		free(pl);
	} while (polyline);
}

static void cyberiada_update_polyline(CyberiadaPolyline* pl, double dx, double dy)
{
	if (!pl) return ;
	do {
		cyberiada_update_point(&(pl->point), dx, dy);
		pl = pl->next;
	} while (pl);
}

/* -----------------------------------------------------------------------------
 * Geometry transformations
 * ----------------------------------------------------------------------------- */
typedef struct _CyberiadaCoordTree {
	CyberiadaRect               rect;
	CyberiadaNode*              node;
	struct _CyberiadaCoordTree* parent;
	struct _CyberiadaCoordTree* next;
	struct _CyberiadaCoordTree* children;
} CyberiadaCoordTree;

static CyberiadaCoordTree* cyberiada_new_coord_tree(void)
{
	CyberiadaCoordTree* tree = (CyberiadaCoordTree*)malloc(sizeof(CyberiadaCoordTree));
	memset(tree, 0, sizeof(CyberiadaCoordTree));
	return tree;
}

static int cyberiada_destroy_coord_tree(CyberiadaCoordTree* tree)
{
	if (tree) {
		if (tree->children) {
			cyberiada_destroy_coord_tree(tree->children);
		}
		if (tree->next) {
			cyberiada_destroy_coord_tree(tree->next);
		}
		free(tree);
	}
	return CYBERIADA_NO_ERROR;
}

static CyberiadaCoordTree* cyberiada_coord_tree_find_node(CyberiadaCoordTree* tree, CyberiadaNode* node)
{
	CyberiadaCoordTree* res;
	if (tree) {
		if (tree->node == node) {
			return tree;
		}
		if (tree->children) {
			if ((res = cyberiada_coord_tree_find_node(tree->children, node))) {
				return res;
			}
		}
		if (tree->next) {
			if ((res = cyberiada_coord_tree_find_node(tree->next, node))) {
				return res;
			}
		}
	}
	return NULL;
}

static int cyberiada_print_coord_tree(CyberiadaCoordTree* tree, int level)
{
	if (level == 0) {
		ERROR/*printf*/("Coord tree:\n");
		if (tree) {
			ERROR/*printf*/("%p {%lf; %lf}\n", tree, tree->rect.x, tree->rect.y);
		} else {
			ERROR/*printf*/("NULL\n");
		}
	} else {
		char levelspace[16];
		int i;
		
		memset(levelspace, 0, sizeof(levelspace));
		for(i = 0; i < level; i++) {
			if (i == 14) break;
			levelspace[i] = ' ';
		}

		if (!tree->node->geometry_rect && !tree->node->geometry_point) {
			ERROR/*printf*/("%s%p [%lf; %lf] parent %p\n", levelspace, tree, tree->rect.x, tree->rect.y, tree->parent);
		} else {
			ERROR/*printf*/("%s%p (%lf; %lf) parent %p\n", levelspace, tree, tree->rect.x, tree->rect.y, tree->parent);
		}
	}
	if (tree->children) {
		cyberiada_print_coord_tree(tree->children, level + 1);
	}
	if (tree->next) {
		cyberiada_print_coord_tree(tree->next, level);
	}
	return CYBERIADA_NO_ERROR;
}

typedef struct {
	CyberiadaGeometryCoordFormat old_format, new_format;
	CyberiadaGeometryEdgeFormat old_edge_format, new_edge_format;
	CyberiadaCoordTree *old_tree;
	CyberiadaRect new_bounding_rect;
} CyberiadaDocumentTransformContext;

static int cyberiada_build_transform_tree(CyberiadaCoordTree* tree, CyberiadaNode* node, CyberiadaCoordTree* parent_tree)
{
	tree->parent = parent_tree;
	tree->node = node;
	
	if (node->geometry_point) {
		tree->rect.x = node->geometry_point->x;
		tree->rect.y = node->geometry_point->y;
	} else if (node->geometry_rect) {
		tree->rect.x = node->geometry_rect->x;
		tree->rect.y = node->geometry_rect->y;
		tree->rect.width = node->geometry_rect->width;
		tree->rect.height = node->geometry_rect->height;
	} else {
		tree->rect.x = parent_tree->rect.x;
		tree->rect.y = parent_tree->rect.y;
		tree->rect.width = parent_tree->rect.width;
		tree->rect.height = parent_tree->rect.height;
	}

	if (node->children) {
		tree->children = cyberiada_new_coord_tree();
		cyberiada_build_transform_tree(tree->children, node->children, tree);
	}
	
	if (node->next) {
		tree->next = cyberiada_new_coord_tree();
		cyberiada_build_transform_tree(tree->next, node->next, parent_tree);
	}

	return CYBERIADA_NO_ERROR;	
}

static CyberiadaDocumentTransformContext* cyberiada_new_transform_context(CyberiadaDocument* doc,
																		  CyberiadaGeometryCoordFormat new_format,
																		  CyberiadaGeometryEdgeFormat new_edge_format)
{
	CyberiadaSM* sm;
	CyberiadaCoordTree* tree;	
	CyberiadaDocumentTransformContext* context = NULL;
	if (!doc) {
		ERROR("Cannot create geometry transformation context\n");
		return NULL;
	}
	if (!doc->bounding_rect) {
		ERROR("No bounding rect for document transformation\n");
		return NULL;
	}
	context = (CyberiadaDocumentTransformContext*)malloc(sizeof(CyberiadaDocumentTransformContext));
	context->old_format = doc->geometry_format;
	context->old_edge_format = doc->edge_geom_format;
	context->new_format = new_format;
	context->new_edge_format = new_edge_format;
	if (!doc->state_machines) {
		context->old_tree = NULL;
		memset(&(context->new_bounding_rect), 0, sizeof(CyberiadaRect)); 	
	} else {
		context->old_tree = cyberiada_new_coord_tree();
		context->old_tree->rect.x = context->new_bounding_rect.x = doc->bounding_rect->x;
		context->old_tree->rect.y = context->new_bounding_rect.y = doc->bounding_rect->y;
		context->old_tree->rect.width = context->new_bounding_rect.width = doc->bounding_rect->width;
		context->old_tree->rect.height = context->new_bounding_rect.height = doc->bounding_rect->height;
		context->old_tree->children = cyberiada_new_coord_tree();
		tree = context->old_tree->children;
		for (sm = doc->state_machines; sm; sm = sm->next) {
			cyberiada_build_transform_tree(tree, sm->nodes, context->old_tree);
			if (sm->next) {
				tree->next = cyberiada_new_coord_tree();
				tree = tree->next;
			}
		}
		/* cyberiada_print_coord_tree(context->old_tree, 0); */
	}
	return context;
}

static int cyberiada_destroy_transform_context(CyberiadaDocumentTransformContext* context)
{
	if (context == NULL) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_destroy_coord_tree(context->old_tree);
	free(context);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_rect_center(CyberiadaRect* rect,
								 CyberiadaPoint* center,
								 CyberiadaGeometryCoordFormat format)
{
	if (!rect || !center) {
		return CYBERIADA_BAD_PARAMETER;
	}

	if (format == cybCoordAbsolute) {
		center->x = rect->x + rect->width / 2.0;
		center->y = rect->y + rect->height / 2.0;
	} else if (format == cybCoordLeftTop) {
		center->x = rect->width / 2.0;
		center->y = rect->height / 2.0;
	} else {
		center->x = center->y = 0.0;
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_extend_rect_p(CyberiadaRect* rect,
								   double x, double y,
								   CyberiadaGeometryCoordFormat format)
{
	double delta;
	if (!rect) {
		ERROR("Cannot extend rect with point\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (cyberiada_empty_rect(rect)) {
		rect->x = x;
		rect->y = y;
		return CYBERIADA_NO_ERROR;
	}
	if (format == cybCoordAbsolute || format == cybCoordLeftTop) {
		if (x < rect->x) {
			delta = rect->x - x;
			rect->width += delta;
			rect->x -= delta;
		}
		if (x > rect->x + rect->width) {
			delta = x - rect->x - rect->width;
			rect->width += delta;
		}
		if (y < rect->y) {
			delta = rect->y - y;
			rect->height += delta;
			rect->y -= delta;
		}
		if (y > rect->y + rect->height) {
			delta = y - rect->y - rect->height;
			rect->height += delta;
		}
	} else if (format == cybCoordLocalCenter) {
		double r_w = rect->width / 2.0;
		double r_h = rect->height / 2.0;
		if (x < rect->x - r_w) {
			delta = rect->x - r_w - x;
			rect->width += delta;
			rect->x -= delta / 2.0;
		}
		if (x > rect->x + r_w) {
			delta = x - rect->x - r_w;
			rect->width += delta;
			rect->x += delta / 2.0;
		}
		if (y < rect->y - r_h) {
			delta = rect->y - r_h - y;
			rect->height += delta;
			rect->y -= delta / 2.0;
		}
		if (y > rect->y + r_h) {
			delta = y - rect->y - r_h;
			rect->height += delta;
			rect->y += delta / 2.0;
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_extend_rect_r(CyberiadaRect* result,
								   CyberiadaRect* parent_rect,
								   CyberiadaRect* rect,
								   CyberiadaGeometryCoordFormat format)
{
	double p_x = 0.0, p_y = 0.0;
	if (!result || !rect) {
		ERROR("Cannot extend rect with rect\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (cyberiada_empty_rect(result)) {
		result->x = rect->x;
		result->y = rect->y;
		result->width = rect->width;
		result->height = rect->height;
		return CYBERIADA_NO_ERROR;
	}
	if (parent_rect) {
		p_x = parent_rect->x;
		p_y = parent_rect->y;
	}
	if (format == cybCoordAbsolute) {
		cyberiada_extend_rect_p(result,
								rect->x,
								rect->y,
								format);
		cyberiada_extend_rect_p(result,
								rect->x + rect->width,
								rect->y + rect->height,
								format);		
	} else if (format == cybCoordLeftTop) {
		cyberiada_extend_rect_p(result,
								p_x + rect->x,
								p_y + rect->y,
								format);
		cyberiada_extend_rect_p(result,
								p_x + rect->x + rect->width,
								p_y + rect->y + rect->height,
								format);				
	} else if (format == cybCoordLocalCenter) {
		cyberiada_extend_rect_p(result,
								p_x + rect->x - rect->width / 2.0,
								p_y + rect->y - rect->height / 2.0,
								format);
		cyberiada_extend_rect_p(result,
								p_x + rect->x + rect->width / 2.0,
								p_y + rect->y + rect->height / 2.0,
								format);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_extend_rect_polyline(CyberiadaRect* result,
										  CyberiadaRect* parent_rect,
										  CyberiadaPolyline* polyline,
										  CyberiadaGeometryCoordFormat format)
{
	CyberiadaPolyline* pl = polyline;
	if (!result || !polyline) {
		ERROR("Cannot extend rect with polyline\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!parent_rect) {
		ERROR("Cannot extend rect with polyline - no parent geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	while (pl) {
		if (format == cybCoordAbsolute || !parent_rect) {
			cyberiada_extend_rect_p(result,
									pl->point.x,
									pl->point.y,
									format);
		} else if (format == cybCoordLeftTop || format == cybCoordLocalCenter) {
			cyberiada_extend_rect_p(result,
									parent_rect->x + pl->point.x,
									parent_rect->y + pl->point.y,
									format);
		}
		pl = pl->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_build_bounding_rect(CyberiadaNode* node,
										 CyberiadaEdge* edges,
										 CyberiadaRect* result,
										 CyberiadaGeometryCoordFormat format)
{
	CyberiadaNode* parent;
	CyberiadaRect local_result;
	if (!node || !result) {
		ERROR("Cannot build bounding rect\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	parent = node->parent;
	memset(&local_result, 0, sizeof(CyberiadaRect));
	/* ERROR("start build bounding rect (%lf; %lf; %lf; %lf)\n", result->x, result->y, result->width, result->height); */
	while (node) {
		/* ERROR("bbr node %s\n", node->id); */
		if (node->geometry_point) {
			/* ERROR("bbr geom point (%lf; %lf)\n", */
			/* 	  node->geometry_point->x, node->geometry_point->y); */
			cyberiada_extend_rect_p(&local_result,
									node->geometry_point->x,
									node->geometry_point->y,
									format);
		}
		if (node->geometry_rect) {
			/* ERROR("bbr geom rect (%lf; %lf; %lf; %lf) parent %d\n", */
			/* 	  node->geometry_rect->x, node->geometry_rect->y, */
			/* 	  node->geometry_rect->width, node->geometry_rect->height, */
			/* 	  parent != NULL); */
			cyberiada_extend_rect_r(&local_result,
									NULL, //parent ? parent->geometry_rect: NULL,
									node->geometry_rect,
									format);
			/* ERROR("local bounding rect before edges (%lf; %lf; %lf; %lf)\n", */
			/* 	  local_result.x, local_result.y, local_result.width, local_result.height); */
			if (edges) {
				CyberiadaEdge* e = edges;
				while (e) {
					if (strcmp(e->source_id, node->id) == 0) {
						if (e->geometry_polyline) {
							cyberiada_extend_rect_polyline(&local_result,
														   node->geometry_rect,
														   e->geometry_polyline,
														   format);						
						}
						break;
					}
					e = e->next;
				}
			}
		}
		if (node->children) {
			int res = cyberiada_build_bounding_rect(node->children, edges, &local_result, format);
			if (res != CYBERIADA_NO_ERROR) {
				ERROR("error while building bounding rect %d\n", res);
				return res;
			}
		}
		/* ERROR("local bounding rect (%lf; %lf; %lf; %lf)\n", */
		/* 	  local_result.x, local_result.y, local_result.width, local_result.height); */
		node = node->next;
	}
	cyberiada_extend_rect_r(result,
							parent ? parent->geometry_rect: NULL,
							&local_result,
							format);
	/* ERROR("finish build bounding rect (%lf; %lf; %lf; %lf)\n", result->x, result->y, result->width, result->height); */
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_geometry(CyberiadaRect* rect,
									  CyberiadaGeometryCoordFormat from_format,
									  CyberiadaGeometryCoordFormat to_format,
									  double* delta_x, double* delta_y)
{
	if (!rect || !delta_x || !delta_y) {
		ERROR("Cannot convert geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (from_format == to_format) {
		*delta_x = *delta_y = 0.0;
		return CYBERIADA_NO_ERROR;
	}

	if (from_format == cybCoordAbsolute && to_format == cybCoordLeftTop) {
		/* (x, y, w, h) -> (x - px, y - py, w, h) */
		*delta_x = -rect->x;
		*delta_y = -rect->y;
	} else if (from_format == cybCoordAbsolute && to_format == cybCoordLocalCenter) {
		/* (x, y, w, h) -> (x - px - pw/2, y - py - ph/2, w, h) */
		*delta_x = -rect->x - rect->width / 2.0;
		*delta_y = -rect->y - rect->height / 2.0;
	} else if (from_format == cybCoordLeftTop && to_format == cybCoordLocalCenter) {
		/* (x, y, w, h) -> (x - pw/2, y - ph/2, w, h) */
		*delta_x = -rect->width / 2.0; //rect->x - rect->width / 2.0;
		*delta_y = -rect->height / 2.0; //rect->y - rect->height / 2.0;
	} else if (from_format == cybCoordLeftTop && to_format == cybCoordAbsolute) {
		/* (x, y, w, h) -> (x + px, y + py, w, h) */
		*delta_x = rect->x;
		*delta_y = rect->y;
	} else if (from_format == cybCoordLocalCenter && to_format == cybCoordAbsolute) {
		/* (x, y, w, h) -> (x + px + pw/2, y + py + ph/2, w, h) */
		*delta_x = rect->x + rect->width / 2.0;
		*delta_y = rect->y + rect->height / 2.0;
	} else if (from_format == cybCoordLocalCenter && to_format == cybCoordLeftTop) {
        /* (x, y, w, h) -> (x + pw/2, y + ph/2, w, h) */
		*delta_x = rect->width / 2.0;
		*delta_y = rect->height / 2.0;
	} else {
		ERROR("bad node coordinates conversion from %d to %d\n", from_format, to_format);
		return CYBERIADA_BAD_PARAMETER;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_node_tree_geometry(CyberiadaCoordTree* tree,
												CyberiadaDocumentTransformContext* context)
{
	CyberiadaGeometryCoordFormat from_format = context->old_format;
	CyberiadaGeometryCoordFormat to_format = context->new_format;
	CyberiadaRect* bounding_rect = &(context->new_bounding_rect);
	CyberiadaRect* old_bounding_rect = &(context->old_tree->rect);
	CyberiadaCoordTree* t;
	/* if (tree->node) { */
	/* 	DEBUG("convert node tree '%s' from %d to %d\n", tree->node->title, from_format, to_format); */
	/* } else { */
	/* 	DEBUG("convert tree from %d to %d\n", from_format, to_format); */
	/* } */
	/* DEBUG("bounding rect (%lf; %lf; %lf; %lf)\n", */
	/* 	  bounding_rect->x, bounding_rect->y, bounding_rect->width, bounding_rect->height); */
	if (tree->parent == NULL) {
		tree = tree->children;
		while (tree) {
			if (tree->node->geometry_rect) {
				/* TODO: implement SM nodes conversion later */
			}
			if (tree->children) {
				cyberiada_convert_node_tree_geometry(tree->children, context);
			}
			tree = tree->next;
		}
	} else {
		int parent_is_bounding_rect = (!tree->parent->node->geometry_rect &&
									   (tree->node->geometry_rect || tree->node->geometry_point) &&
									   tree->next &&
									   cyberiada_compare_rects(old_bounding_rect, &(tree->parent->rect)) == 0);
		/* if (parent_is_bounding_rect) { */
		/* 	DEBUG("parent is equal to bounding\n"); */
		/* } */
		while (tree) {
			double delta_x = 0.0, delta_y = 0.0;
			int parent_based_delta = 0;
			/* ERROR("convert node %s geometry (%lf; %lf; %lf; %lf)\n", */
			/* 	  tree->node->title, tree->rect.x, tree->rect.y, tree->rect.width, tree->rect.height); */
			/* ERROR("parent geometry (%lf; %lf; %lf; %lf)\n", */
			/* 	  tree->parent->rect.x, tree->parent->rect.y, tree->parent->rect.width, tree->parent->rect.height); */
			cyberiada_convert_geometry(&(tree->parent->rect), from_format, to_format, &delta_x, &delta_y);
			/* ERROR("base delta x %lf y %lf\n", delta_x, delta_y); */
			if (!tree->parent->node->geometry_rect) {
				/* DEBUG("parent w/o real geometry: top level node\n"); */
				if ((((from_format == cybCoordAbsolute || from_format == cybCoordLeftTop) &&
					 to_format == cybCoordLocalCenter) ||
					((to_format == cybCoordAbsolute || to_format == cybCoordLeftTop) &&
					 from_format == cybCoordLocalCenter))) {
					if (tree->node->geometry_point) {
						if(!parent_is_bounding_rect) {
/*							ERROR("parent based delta bounding (%lf; %lf) point (%lf; %lf)\n",
								  bounding_rect->x, bounding_rect->y,
								  tree->node->geometry_point->x,
								  tree->node->geometry_point->y);*/
							delta_x = bounding_rect->x - tree->node->geometry_point->x;
							delta_y = bounding_rect->y - tree->node->geometry_point->y;
							parent_based_delta = 1;				
						}
 					}
					if (tree->node->geometry_rect && !parent_is_bounding_rect) {
/*						ERROR("parent based delta bounding (%lf; %lf) rect (%lf; %lf)\n",
							  bounding_rect->x, bounding_rect->y,
							  tree->node->geometry_rect->x,
							  tree->node->geometry_rect->y);*/
						delta_x = bounding_rect->x - tree->node->geometry_rect->x;
						delta_y = bounding_rect->y - tree->node->geometry_rect->y;
						parent_based_delta = 1;
					}
				} else {
					delta_x = delta_y = 0.0;
					parent_based_delta = 1;
				}
			}
			/* ERROR("delta x %lf y %lf node is bound %d parent based %d\n", */
			/* 	  delta_x, delta_y, parent_is_bounding_rect, parent_based_delta); */
			if (tree->children) {
				cyberiada_convert_node_tree_geometry(tree->children, context);
			}
			if (tree->node->geometry_point) {
				if (from_format == cybCoordLocalCenter && parent_is_bounding_rect) {
					tree->node->geometry_point->x += tree->parent->rect.x;
					tree->node->geometry_point->y += tree->parent->rect.y;
				} else {
					tree->node->geometry_point->x += delta_x;
					tree->node->geometry_point->y += delta_y;
				}
			}
			if (tree->node->geometry_rect) {
				if (from_format == cybCoordLocalCenter && parent_is_bounding_rect) {
					tree->node->geometry_rect->x += tree->parent->rect.x - tree->node->geometry_rect->width / 2.0;
					tree->node->geometry_rect->y += tree->parent->rect.y - tree->node->geometry_rect->height / 2.0;					
				} else if (to_format == cybCoordLocalCenter && !parent_based_delta) {
					tree->node->geometry_rect->x += delta_x + tree->node->geometry_rect->width / 2.0;
					tree->node->geometry_rect->y += delta_y + tree->node->geometry_rect->height / 2.0;
				} else if (from_format == cybCoordLocalCenter && !parent_based_delta) {
					tree->node->geometry_rect->x += delta_x - tree->node->geometry_rect->width / 2.0;
					tree->node->geometry_rect->y += delta_y - tree->node->geometry_rect->height / 2.0;
				} else {
					tree->node->geometry_rect->x += delta_x;
					tree->node->geometry_rect->y += delta_y;
				}
			}
/*			if (tree->node->geometry_point) {
				ERROR("node %s new geometry point (%lf; %lf)\n",
					  tree->node->title,
					  tree->node->geometry_point->x,
					  tree->node->geometry_point->y);
			}
			if (tree->node->geometry_rect) {
				ERROR("node %s new geometry rect (%lf; %lf; %lf; %lf)\n",
					  tree->node->title,
					  tree->node->geometry_rect->x,
					  tree->node->geometry_rect->y,
					  tree->node->geometry_rect->width,
					  tree->node->geometry_rect->height);
					  }*/
			tree = tree->next;
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_node_geometry(CyberiadaDocument* doc,
										   CyberiadaDocumentTransformContext* context)
{
	CyberiadaSM* sm;
	if (!context || !doc || !doc->state_machines) {
		ERROR("Cannot convert SM document geometry - no context/doc/state machines\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (context->old_format == context->new_format ||
		context->old_format == cybCoordNone ||
		context->new_format == cybCoordNone) {
		return CYBERIADA_NO_ERROR;
	}
	/* ERROR("\n"); */
	/* ERROR("convert node geometry from %d to %d\n", context->old_format, context->new_format); */
	/* if left-top-to-center transformation skip bounding rect & SMs, else convert SMs */
	/* ERROR("old b r (%lf; %lf; %lf; %lf)\n", */
	/* 	  context->old_tree->rect.x, */
	/* 	  context->old_tree->rect.y, */
	/* 	  context->old_tree->rect.width, */
	/* 	  context->old_tree->rect.height); */
	if ((context->old_format == cybCoordAbsolute || context->old_format == cybCoordLeftTop) &&
		context->new_format == cybCoordLocalCenter) {
		context->new_bounding_rect.x = context->old_tree->rect.x + context->old_tree->rect.width / 2.0;
		context->new_bounding_rect.y = context->old_tree->rect.y + context->old_tree->rect.height / 2.0;
	} else if ((context->new_format == cybCoordAbsolute || context->new_format == cybCoordLeftTop) &&
			   context->old_format == cybCoordLocalCenter) {
		context->new_bounding_rect.x = context->old_tree->rect.x - context->old_tree->rect.width / 2.0;
		context->new_bounding_rect.y = context->old_tree->rect.y - context->old_tree->rect.height / 2.0;
	} else {
		context->new_bounding_rect.x = context->old_tree->rect.x;
		context->new_bounding_rect.y = context->old_tree->rect.y;
	}
	/* ERROR("new b r (%lf; %lf; %lf; %lf)\n", */
	/* 	  context->new_bounding_rect.x, context->new_bounding_rect.y, */
	/* 	  context->new_bounding_rect.width, context->new_bounding_rect.height); */
	
	cyberiada_convert_node_tree_geometry(context->old_tree, context);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_is_point_node(CyberiadaNode* node)
{
	if (!node) return 0;
	return node->type & (cybNodeInitial | cybNodeFinal | cybNodeTerminate |
						 cybNodeEntryPoint | cybNodeExitPoint |
						 cybNodeShallowHistory | cybNodeDeepHistory |
						 cybNodeFork | cybNodeJoin);
}

static int cyberiada_is_rect_node(CyberiadaNode* node)
{
	if (!node) return 0;
	return !cyberiada_is_point_node(node);
}

static int cyberiada_rect_contains_point(CyberiadaRect* rect,
										 CyberiadaPoint* point,
										 CyberiadaGeometryCoordFormat format)
{
	if (!rect || !point) return 0;
	if (format == cybCoordAbsolute || format == cybCoordLeftTop) {
		return (point->x >= rect->x &&
				point->y >= rect->y &&
				point->x <= rect->x + rect->width &&
				point->y <= rect->y + rect->height);
	} else if (format == cybCoordLocalCenter) {
		return (point->x >= -rect->width / 2.0 + rect->x &&
				point->y >= -rect->height / 2.0 + rect->y &&
				point->x <= rect->width / 2.0 + rect->x &&
				point->y <= rect->height / 2.0 + rect->y);		
	} else {
		return 0;
	}
}

static int cyberiada_rect_correct_point(CyberiadaRect* rect,
										CyberiadaPoint* point,
										CyberiadaGeometryCoordFormat format)
{
	if (!rect || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (format != cybCoordAbsolute && format != cybCoordLeftTop && format != cybCoordLocalCenter) {
		return CYBERIADA_BAD_PARAMETER;
	}

	/*cyberiada_round_point(point, 4);*/

	if (format == cybCoordAbsolute) {
		if (point->x < rect->x) point->x = rect->x;
		if (point->x > rect->x + rect->width) point->x = rect->x + rect->width;
		if (point->y < rect->y) point->y = rect->y;
		if (point->y > rect->y + rect->height) point->y = rect->y + rect->height;				
	} else if (format == cybCoordLeftTop) {
		if (point->x < 0.0) point->x = 0.0;
		if (point->x > rect->width) point->x = rect->width;
		if (point->y < 0.0) point->y = 0.0;
		if (point->y > rect->height) point->y = rect->height;				
	} else {
		/*  format == cybCoordLocalCenter */
		if (point->x < -rect->width / 2.0) point->x = -rect->width / 2.0;
		if (point->x > rect->width / 2.0) point->x = rect->width / 2.0;
		if (point->y < -rect->height / 2.0) point->y = -rect->height / 2.0;
		if (point->y > rect->height / 2.0) point->y = rect->height / 2.0;				
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_rect_contains_rect(CyberiadaRect* rect,
										CyberiadaRect* r,
										CyberiadaGeometryCoordFormat format)
{
	if (!rect || !r) return 0;
	if (format == cybCoordAbsolute || format == cybCoordLeftTop) {
		return (r->x >= rect->x &&
				r->y >= rect->y &&
				r->x + r->width <= rect->x + rect->width &&
				r->y + r->height <= rect->y + rect->height);
	} else if (format == cybCoordLocalCenter) {
		return (rect->x - r->width / 2.0 >= rect->x - rect->width / 2.0 &&
				rect->y - r->height / 2.0 >=  rect->y - rect->height / 2.0 &&
				rect->x + r->width / 2.0 <=  rect->x + rect->width / 2.0 &&
				rect->y + r->height / 2.0 <= rect->y + rect->height / 2.0);		
	} else {
		return 0;
	}
}

static int cyberiada_node_contains_point(CyberiadaNode* node,
										 CyberiadaEdge* edges,
										 CyberiadaPoint* point,
										 CyberiadaGeometryCoordFormat format)
{
	int res;
	CyberiadaRect* bound;
	if (!node || !point) return 0;
	if (!node->geometry_rect) return 0;
	
	bound = cyberiada_copy_rect(node->geometry_rect);
	cyberiada_build_bounding_rect(node, edges, bound, format);
	res = cyberiada_rect_contains_point(bound, point, format);
	free(bound);
	return res;
}

/* static int cyberiada_reconstruct_nodes(CyberiadaNode* node, */
/* 									   CyberiadaRect* rect, */
/* 									   CyberiadaGeometryCoordFormat format) */
/* { */
	/* TODO reconstruction algorithm:
	   
       - init the reconstruct queue
	   - examine the children nodes 
	       - child is the parent itself and has geometry - recurcively continue the algorithm
           - child is the parent but has no geometry
			   - build the bounding rect as possible limitation
			   - save the node in the reconstruct queue
           - child is atomar but has no geometry - save the node in the reconstruct list
       - recunstruct geometry for nodes from the list inside the box [if parent has geometry]
       - go through the reconstructed children
       - if parent has no geometry, reconstruct the geometry 

       Right now:

       - determine the parent borders
       - put nodes w/o geometry somewhere in the parent
    */

/* 	CyberiadaNode* start_node = node; */
/* 	if (!node && !rect) { */
/* 		ERROR("Cannot reconstruct node geometry\n"); */
/* 		return CYBERIADA_BAD_PARAMETER;	 */
/* 	} */

/* 	while (node) { */
/* 		if (cyberiada_is_point_node(node) && !node->geometry_point) { */
/* 			CyberiadaPoint* new_point = cyberiada_new_point(); */
/* 			CyberiadaNode* n; */
/* 			int intersects; */
/* 			do { */
/* 				if (format == cybCoordAbsolute) { */
/* 					new_point->x = rect->x + rect->width * rand(); */
/* 					new_point->y = rect->y + rect->height * rand();					 */
/* 				} else if (format == cybCoordLeftTop) { */
/* 					new_point->x = rect->width * rand(); */
/* 					new_point->y = rect->height * rand();					 */
/* 				} else if (format == cybCoordLocalCenter) { */
/* 					new_point->x = rect->width * (-0.5 + rand()); */
/* 					new_point->y = rect->height * (-0.5 + rand()); */
/* 				} else { */
/* 					free(new_point); */
/* 					return CYBERIADA_BAD_PARAMETER; */
/* 				} */

/* 				intersects = 0; */
/* 				for (n = start_node; n->next; n = n->next) { */
/* 					if (n == node) continue; */
/* 					if (n->geometry_rect && cyberiada_rect_contains_point(n->geometry_rect, new_point, format)) { */
/* 						intersects = 1; */
/* 						break; */
/* 					} */
/* 				} */
				
/* 			} while (intersects); */
			
/* 			node->geometry_point = new_point; */
/* 		} */

/* 		if (cyberiada_is_rect_node(node) && !node->geometry_rect) { */
/* 			CyberiadaRect* new_rect = cyberiada_new_rect(); */
/* 			CyberiadaNode* n; */
/* 			int intersects; */
/* 			double node_width = DEFAULT_NODE_SIZE, node_height = DEFAULT_NODE_SIZE; */
/* 			if (node_width < rect->width) { */
/* 				node_width = rect->width / 2.0; */
/* 			} */
/* 			if (node_height < rect->height) { */
/* 				node_height = rect->height / 2.0; */
/* 			} */
/* 			do { */
/* 				new_rect->width = node_width; */
/* 				new_rect->height = node_height; */
/* 				if (format == cybCoordAbsolute) { */
/* 					new_rect->x = rect->x + (rect->width - node_width) * rand(); */
/* 					new_rect->y = rect->y + (rect->height - node_height) * rand(); */
/* 				} else if (format == cybCoordLeftTop) { */
/* 					new_rect->x = (rect->width - node_width) * rand(); */
/* 					new_rect->y = (rect->height - node_height) * rand();					 */
/* 				} else if (format == cybCoordLocalCenter) { */
/* 					new_rect->x = (rect->width - node_width) * (-0.5 + rand()); */
/* 					new_rect->y = (rect->height - node_height) * (-0.5 + rand()); */
/* 				} else { */
/* 					free(new_rect); */
/* 					return CYBERIADA_BAD_PARAMETER; */
/* 				} */

/* 				intersects = 0; */
/* 				for (n = start_node; n->next; n = n->next) { */
/* 					if (n == node) continue; */
/* 					if (n->geometry_rect */
/* 						&& cyberiada_rect_contains_rect(n->geometry_rect, new_rect, format)) { */
/* 						intersects = 1; */
/* 						break; */
/* 					} */
/* 				} */
				
/* 			} while (intersects); */
			
/* 			node->geometry_rect = new_rect; */
/* 		} */
		
/* 		if (node->children) { */
/* 			cyberiada_build_bounding_rect(node->children, node->geometry_rect, format);			 */
/* 			int res = cyberiada_reconstruct_nodes(node->children, node->geometry_rect, format); */
/* 			if (res != CYBERIADA_NO_ERROR) { */
/* 				return res; */
/* 			} */
/* 		} */
/* 		node = node->next; */
/* 	} */
/* 	return CYBERIADA_NO_ERROR; */
/* } */

/* static int cyberiada_reconstruct_edge(CyberiadaPoint** source_point, */
/* 									  CyberiadaNode* source, */
/* 									  CyberiadaPoint** target_point, */
/* 									  CyberiadaNode* target, */
/* 									  CyberiadaGeometryCoordFormat format, */
/* 									  CyberiadaGeometryEdgeFormat edge_format) */
/* { */
/* 	CyberiadaPoint* point; */
	
/* 	if (!source_point || !source || !target_point || !target) { */
/* 		ERROR("Cannot reconstruct edge\n"); */
/* 		return CYBERIADA_BAD_PARAMETER; */
/* 	} */
	
/* 	if (edge_format == cybEdgeCenter) { */

/* 		if (!*source_point) { */
/* 			*source_point = cyberiada_new_point(); */
/* 		} */

/* 		if (!*target_point) { */
/* 			*target_point = cyberiada_new_point(); */
/* 		} */

/* 		return CYBERIADA_NO_ERROR; */
		
/* 	} */

/* 	return CYBERIADA_NO_ERROR;	 */
	/*
		
		if (source->geometry_point && !*source_point) {
			*source_point = cyberiada_new_point();
		}
		if (target->geometry_point && !*target_point) {
			*target_point = cyberiada_new_point();
		}

	}

	if (*source_point && *target_point) {
		return CYBERIADA_NO_ERROR;
	}

	if (!*source_point) {
		CyberiadaPoint* to_point;
		if (*target_point) {
			to_point = *target_point;
		} else if (target->geometry_point) {
			to_point = target->geometry_point;
		} else {
		}
		cyberiada_edge_build_border_point(source_point,
										  source->geometry_rect,
										  to_point,
										  edge_format);
		if (source->geometry_rect) {
	}
	
	
	if (edge_format == cybEdgeLeftTopBorder) {
		source->geometry_rect->width / 2.0;
		
		
	} else if (edge_format == cybEdgeCenterBorder) {
		
	} else {
		ERROR("Wrong edge format %d\n", edge_format);
		return CYBERIADA_BAD_PARAMETER;
	}
	
	return CYBERIADA_NO_ERROR;*/
/* } */

/* static int cyberiada_reconstruct_edges(CyberiadaEdge* edge, */
/* 									   CyberiadaGeometryCoordFormat format, */
/* 									   CyberiadaGeometryEdgeFormat edge_format) */
/* { */
/* 	while (edge) { */
/* 		if (edge->source && (edge->source->geometry_rect || edge->source->geometry_point) && */
/* 			edge->target && (edge->target->geometry_rect || edge->target->geometry_point)) { */

/* 			if (!edge->geometry_source_point || !edge->geometry_target_point) { */
/* 				cyberiada_reconstruct_edge(&(edge->geometry_source_point), */
/* 										   edge->source, */
/* 										   &(edge->geometry_target_point), */
/* 										   edge->target, */
/* 										   format, */
/* 										   edge_format); */
/* 			} */

/* 		} */
/* 		edge = edge->next; */
/* 	} */
/* 	return CYBERIADA_NO_ERROR; */
/* } */

static int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc)
{
	/* NOT SUPPORTED YET */
	CyberiadaSM* sm;
	if (!doc || !doc->state_machines) {
		ERROR("Cannot round SM document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
/*	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_reconstruct_nodes(sm->nodes, doc->bounding_rect, doc->geometry_format);
		cyberiada_reconstruct_edges(sm->edges, doc->geometry_format, doc->edge_geom_format);
		}
		return CYBERIADA_NO_ERROR;*/
	return CYBERIADA_NOT_IMPLEMENTED;
}

static int cyberiada_clean_nodes_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->geometry_point) {
			free(node->geometry_point);
			node->geometry_point = NULL;
		}
		if (node->geometry_rect) {
			free(node->geometry_rect);
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
		cyberiada_destroy_polyline(edge->geometry_polyline);
		edge->geometry_polyline = NULL;
	}
	if (edge->geometry_source_point) {
		free(edge->geometry_source_point);
		edge->geometry_source_point = NULL;
		}
	if (edge->geometry_target_point) {
		free(edge->geometry_target_point);
		edge->geometry_target_point = NULL;
	}
	if (edge->geometry_label_point) {
		free(edge->geometry_label_point);
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
		free(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}
	
	doc->geometry_format = cybCoordNone;
	doc->edge_geom_format = cybEdgeNone;
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_recalculate_point(CyberiadaPoint* point,
									   CyberiadaCoordTree* source_tree,
									   CyberiadaCoordTree* target_tree,
									   CyberiadaGeometryCoordFormat old_format)
{
	if (!point || !source_tree || !target_tree) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (old_format == cybCoordAbsolute || old_format == cybCoordNone) {
		return CYBERIADA_NO_ERROR;
	} else if (old_format == cybCoordLeftTop) {
		point->x += source_tree->rect.x;
		point->y += source_tree->rect.y;
	} else if (old_format == cybCoordLocalCenter) {
		point->x += source_tree->rect.x + source_tree->rect.width / 2.0;
		point->y += source_tree->rect.y + source_tree->rect.height / 2.0;
	} else {
		ERROR("Cannot recalculate point: bad format %d", old_format);
		return CYBERIADA_BAD_PARAMETER;		
	}
	if (source_tree->parent) {
		cyberiada_recalculate_point(point, source_tree->parent, target_tree, old_format);
	} else {
		if (old_format == cybCoordLeftTop) {
			point->x -= target_tree->rect.x;
			point->y -= target_tree->rect.y;
		} else if (old_format == cybCoordLocalCenter) {
			point->x -= target_tree->rect.x + target_tree->rect.width / 2.0;
			point->y -= target_tree->rect.y + target_tree->rect.height / 2.0;
		}
		if (target_tree->parent) {
			cyberiada_recalculate_point(point, source_tree, target_tree->parent, old_format);
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_find_border_point(CyberiadaCoordTree* tree,
									   CyberiadaPoint* local_center,
									   CyberiadaPoint* distant_center,
									   CyberiadaPoint* point,
									   CyberiadaGeometryCoordFormat old_format)
{
	if (!tree || !local_center || !distant_center || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (old_format != cybCoordAbsolute && old_format != cybCoordLeftTop && old_format != cybCoordLocalCenter) {
		return CYBERIADA_BAD_PARAMETER;
	}

	CyberiadaRect* rect = &(tree->rect);
	double delta_center_x, delta_center_y;

	if (old_format == cybCoordAbsolute) {
		delta_center_x = local_center->x - rect->x - rect->width / 2.0;
		delta_center_y = local_center->y - rect->y - rect->height / 2.0;
	} else if (old_format == cybCoordLeftTop) {
		delta_center_x = local_center->x - rect->width / 2.0;
		delta_center_y = local_center->y - rect->height / 2.0;
	} else {
		/*  old_format == cybCoordLocalCenter */
		delta_center_x = 0.0;
		delta_center_y = 0.0;
	}
	/* ERROR("find border local (%lf; %lf) distant (%lf; %lf) source %d\n", */
	/* 	  local_center->x, local_center->y, */
	/* 	  distant_center->x, distant_center->y, */
	/* 	  source); */
	/* ERROR("delta center (%lf; %lf)\n", delta_center_x, delta_center_y); */

	if (local_center->x == distant_center->x) {
		point->x = local_center->x;
		if (local_center->y >= distant_center->y) {
			/* DEBUG("top\n"); */
			if (old_format == cybCoordAbsolute) {
				point->y = rect->y;
			} else if (old_format == cybCoordLeftTop) {
				point->y = 0.0;
			} else {
				point->y = -rect->height / 2.0;
			}
		} else {
			/* DEBUG("bottom\n"); */
			if (old_format == cybCoordAbsolute) {
				point->y = rect->y + rect->height;
			} else if (old_format == cybCoordLeftTop) {
				point->y = rect->height;
			} else {
				/*  old_format == cybCoordLocalCenter */
				point->y = rect->height / 2.0;
			}
		}
		/* ERROR("vertical (%lf; %lf)\n", point->x, point->y); */
	} else {
		double alpha = atan(-(local_center->y - distant_center->y) /
							(local_center->x - distant_center->x));
		if (distant_center->x < local_center->x) alpha += M_PI;
		double alpha_g = 180.0 * alpha / M_PI;
		/* ERROR("board src alpha %f\n", alpha_g); */
		if (alpha_g < 0.0) alpha_g += 360.0;
		if (alpha_g <= 45.0 || alpha_g > 315.0) {
			if (old_format == cybCoordAbsolute) {
				point->x = rect->x + rect->width;
				point->y = rect->y - tan(alpha) * rect->width / 2.0 + rect->height / 2.0;
			} else if (old_format == cybCoordLeftTop) {
				point->x = rect->width;
				point->y = (-tan(alpha) * rect->width + rect->height) / 2.0;
			} else {
				/*  old_format == cybCoordLocalCenter */
				point->x = rect->width / 2.0;
				point->y = -tan(alpha) * rect->width / 2.0;
			}
			point->y += delta_center_y;
		} else if (alpha_g > 45.0 && alpha_g <= 135.0) {
			if (old_format == cybCoordAbsolute) {
				point->x = rect->x + tan(alpha) * rect->height / 2.0 + rect->width / 2.0;
				point->y = rect->y;
			} else if (old_format == cybCoordLeftTop) {
				point->x = (tan(alpha) * rect->height + rect->width) / 2.0;
				point->y = 0.0;					
			} else {
				/*  old_format == cybCoordLocalCenter */
				point->x = tan(alpha) * rect->height / 2.0;
				point->y = -rect->height / 2.0;
			}
			point->x += delta_center_x;
		} else if (alpha_g > 135.0 && alpha_g <= 225.0) {
			if (old_format == cybCoordAbsolute) {
				point->x = rect->x;
				point->y = rect->y + tan(alpha) * rect->width / 2.0 + rect->height / 2.0;
			} else if (old_format == cybCoordLeftTop) {
				point->x = 0.0;
				point->y = (-tan(alpha) * rect->width + rect->height) / 2.0;
			} else {
				/*  old_format == cybCoordLocalCenter */
				point->x = -rect->width / 2.0;
				point->y = -tan(alpha) * rect->width / 2.0;
			}
			point->y += delta_center_y;
		} else {
			if (old_format == cybCoordAbsolute) {
				point->x = rect->x - tan(alpha) * rect->height / 2.0 + rect->width / 2.0;
				point->y = rect->y + rect->height;
			} else if (old_format == cybCoordLeftTop) {
				point->x = (-tan(alpha) * rect->height + rect->width) / 2.0;
				point->y = rect->height;
			} else {
				/*  old_format == cybCoordLocalCenter */
				point->x = -tan(alpha) * rect->height / 2.0;
				point->y = rect->height / 2.0;
			}
			point->x += delta_center_x;
		}

		cyberiada_rect_correct_point(rect, point, old_format);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_calculate_border_source_point(CyberiadaCoordTree* source_tree,
												   CyberiadaCoordTree* target_tree,
												   CyberiadaPoint* source_center,
												   CyberiadaPoint* target_center,
												   CyberiadaPoint* point,
												   CyberiadaGeometryCoordFormat old_format)
{
	CyberiadaPoint new_target_center;
	if (!source_tree || !target_tree || !source_center || !target_center || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (old_format != cybCoordAbsolute && old_format != cybCoordLeftTop && old_format != cybCoordLocalCenter) {
		ERROR("Cannot calculate border source. Bad old geometry format %d\n", old_format);
		return CYBERIADA_BAD_PARAMETER;
	}
	new_target_center.x = target_center->x;
	new_target_center.y = target_center->y;
	cyberiada_recalculate_point(&new_target_center, target_tree, source_tree, old_format);

	if (source_tree->node->geometry_point) {
		if (old_format == cybCoordAbsolute) {
			point->x = source_center->x;
			point->y = source_center->y;
		} else {
			/* old_format == cybCoordLeftTop || old_format == cybCoordLocalCenter */
			point->x = point->y = 0.0;
		}
	} else {
		cyberiada_find_border_point(source_tree, source_center, &new_target_center, point, old_format);
	}

	/* ERROR("calc board sp source (%.0lf; %.0lf) target (%.0lf; %.0lf) source rect (%.0lf; %.0lf; %.0lf; %.0lf) -> (%.0lf; %.0lf)\n", */
	/* 	  source_center->x, source_center->y, new_target_center.x, new_target_center.y, */
	/* 	  source_tree->rect.x, source_tree->rect.y, source_tree->rect.width, source_tree->rect.height, */
	/* 	  point->x, point->y); */
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_calculate_border_target_point(CyberiadaCoordTree* source_tree,
												   CyberiadaCoordTree* target_tree,
												   CyberiadaPoint* source_center,
												   CyberiadaPoint* target_center,
												   CyberiadaPoint* point,
												   CyberiadaGeometryCoordFormat old_format)
{
	CyberiadaPoint new_source_center;
	if (!source_tree || !target_tree || !source_center || !target_center || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_source_center.x = source_center->x;
	new_source_center.y = source_center->y;
	cyberiada_recalculate_point(&new_source_center, source_tree, target_tree, old_format);

	if (target_tree->node->geometry_point) {
		if (old_format == cybCoordAbsolute) {
			point->x = target_center->x;
			point->y = target_center->y;
		} else {
			/* old_format == cybCoordLeftTop || old_format == cybCoordLocalCenter */
			point->x = point->y = 0.0;
		}
	} else {
		cyberiada_find_border_point(target_tree, target_center, &new_source_center, point, old_format);
	}
	
	/* ERROR("calc board tp source (%.0lf; %.0lf) target (%.0lf; %.0lf) target rect (%.0lf; %.0lf; %.0lf; %.0lf) -> (%.0lf; %.0lf)\n", */
	/* 	  new_source_center.x, new_source_center.y, target_center->x, target_center->y, */
	/* 	  target_tree->rect.x, target_tree->rect.y, target_tree->rect.width, target_tree->rect.height, */
	/* 	  point->x, point->y); */
		
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_calculate_center_source_point(CyberiadaCoordTree* source_tree,
												   CyberiadaCoordTree* target_tree,
												   CyberiadaPoint* source_center,
												   CyberiadaPoint* target_center,
												   CyberiadaPoint* point,
												   CyberiadaGeometryCoordFormat old_format,
												   CyberiadaGeometryCoordFormat new_format)
{
	if (!source_tree || !target_tree || !source_center || !target_center || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	point->x = source_center->x;
	point->y = source_center->y;
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_calculate_center_target_point(CyberiadaCoordTree* source_tree,
												   CyberiadaCoordTree* target_tree,
												   CyberiadaPoint* source_center,
												   CyberiadaPoint* target_center,
												   CyberiadaPoint* point,
												   CyberiadaGeometryCoordFormat old_format,
												   CyberiadaGeometryCoordFormat new_format)
{
	if (!source_tree || !target_tree || !source_center || !target_center || !point) {
		return CYBERIADA_BAD_PARAMETER;
	}
	point->x = target_center->x;
	point->y = target_center->y;
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_convert_edge_geometry(CyberiadaDocument* doc, CyberiadaDocumentTransformContext* context)
{
	CyberiadaSM* sm;
	CyberiadaEdge* edge;
	CyberiadaCoordTree *source_tree, *target_tree;
	CyberiadaPoint source_center, target_center;
	double delta_x, delta_y;
	CyberiadaPolyline* pl;

	if ((context->old_format == context->new_format &&
		 context->old_edge_format == context->new_edge_format) ||
		context->old_edge_format == cybEdgeNone ||
		context->new_edge_format == cybEdgeNone) {
		return CYBERIADA_NO_ERROR;
	}
	
	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (sm->edges) {
			edge = sm->edges;
			while (edge) {
				if (edge->source && (edge->source->geometry_rect || edge->source->geometry_point) &&
					edge->target && (edge->target->geometry_rect || edge->target->geometry_point)) {
					
					source_tree = cyberiada_coord_tree_find_node(context->old_tree, edge->source);
					target_tree = cyberiada_coord_tree_find_node(context->old_tree, edge->target);
					if (!source_tree || !target_tree || !source_tree->parent || !target_tree->parent) {
						ERROR("Problem while searching coord tree\n");
						return CYBERIADA_BAD_PARAMETER;
					}
					
					/* DEBUG("edge %s %s sp %d tp %d pl %d lab %d\n", */
					/* 	  edge->source->title, edge->target->title, */
					/* 	  edge->geometry_source_point != NULL, */
					/* 	  edge->geometry_target_point != NULL, */
					/* 	  edge->geometry_polyline != NULL, */
					/* 	  edge->geometry_label_point != NULL); */

					cyberiada_rect_center(&(source_tree->rect), &source_center, context->old_format);
					cyberiada_rect_center(&(target_tree->rect), &target_center, context->old_format);

					if (!edge->geometry_source_point) {
						edge->geometry_source_point = cyberiada_new_point();
					}
					if (!edge->geometry_target_point) {
						edge->geometry_target_point = cyberiada_new_point();
					}

					/* ERROR("source rect (%lf; %lf; %lf; %lf) target rect (%lf; %lf; %lf; %lf)\n", */
					/* 	  source_tree->rect.x, source_tree->rect.y, source_tree->rect.width, source_tree->rect.height, */
					/* 	  target_tree->rect.x, target_tree->rect.y, target_tree->rect.width, target_tree->rect.height); */
					/* ERROR("source center (%lf; %lf) target center (%lf; %lf)\n", */
					/* 	  source_center.x, source_center.y, target_center.x, target_center.y); */
					/* ERROR("old sp (%lf; %lf) tp (%lf; %lf)\n", */
					/* 	  edge->geometry_source_point->x, */
					/* 	  edge->geometry_source_point->y, */
					/* 	  edge->geometry_target_point->x, */
					/* 	  edge->geometry_target_point->y); */
					
					if (context->old_edge_format != context->new_edge_format) {
						if (context->old_edge_format == cybEdgeCenter) {
							/* from center to border */

							source_center.x += edge->geometry_source_point->x;
							source_center.y += edge->geometry_source_point->y;
							target_center.x += edge->geometry_target_point->x;
							target_center.y += edge->geometry_target_point->y;
						
							if (edge->geometry_polyline) {
								CyberiadaPolyline* pl;
								CyberiadaPoint source_pl, target_pl;
								source_pl.x = edge->geometry_polyline->point.x;
								source_pl.y = edge->geometry_polyline->point.y;
								pl = edge->geometry_polyline;
								while (pl->next) pl = pl->next;
								target_pl.x = pl->point.x;
								target_pl.y = pl->point.y;

								cyberiada_recalculate_point(&source_pl, source_tree, target_tree, context->old_format);

								cyberiada_calculate_border_source_point(source_tree,
																		target_tree,
																		&source_center,
																		&source_pl,
																		edge->geometry_source_point,
																		context->old_format);
								cyberiada_calculate_border_target_point(source_tree,
																		target_tree,
																		&target_pl,
																		&target_center,
																		edge->geometry_target_point,
																		context->old_format);
							} else {
								cyberiada_calculate_border_source_point(source_tree,
																		target_tree,
																		&source_center,
																		&target_center,
																		edge->geometry_source_point,
																		context->old_format);
								cyberiada_calculate_border_target_point(source_tree,
																		target_tree,
																		&source_center,
																		&target_center,
																		edge->geometry_target_point,
																		context->old_format);
							}

						} else {
							/* from border to center */
							cyberiada_calculate_border_source_point(source_tree,
																	target_tree,
																	&source_center,
																	&target_center,
																	edge->geometry_source_point,
																	context->old_format);							
							cyberiada_calculate_border_target_point(source_tree,
																	target_tree,
																	&source_center,
																	&target_center,
																	edge->geometry_target_point,
																	context->old_format);
						
							cyberiada_calculate_center_source_point(source_tree,
																	target_tree,
																	&source_center,
																	&target_center,
																	edge->geometry_source_point,
																	context->old_format,
																	context->new_format);
							cyberiada_calculate_center_target_point(source_tree,
																	target_tree,
																	&source_center,
																	&target_center,
																	edge->geometry_target_point,
																	context->old_format,
																	context->new_format);
						
							edge->geometry_source_point->x -= source_center.x;
							edge->geometry_source_point->y -= source_center.y;
							edge->geometry_target_point->x -= target_center.x;
							edge->geometry_target_point->y -= target_center.y;
						}
					}

					if (context->old_format != context->new_format) {
						cyberiada_convert_geometry(&(source_tree->rect),
												   context->old_format,
												   context->new_format,
												   &delta_x, &delta_y);
						edge->geometry_source_point->x += delta_x;
						edge->geometry_source_point->y += delta_y;
						
						cyberiada_convert_geometry(&(target_tree->rect),
												   context->old_format,
												   context->new_format,
												   &delta_x, &delta_y);
						edge->geometry_target_point->x += delta_x;
						edge->geometry_target_point->y += delta_y;							
					}

					/* ERROR("new sp (%lf; %lf) tp (%lf; %lf)\n", */
					/* 	  edge->geometry_source_point->x, */
					/* 	  edge->geometry_source_point->y, */
					/* 	  edge->geometry_target_point->x, */
					/* 	  edge->geometry_target_point->y); */
					
					if (edge->geometry_polyline) {
						cyberiada_convert_geometry(&(source_tree->rect),
												   context->old_format,
												   context->new_format,
												   &delta_x, &delta_y);
						pl = edge->geometry_polyline; 
						while (pl) {
							pl->point.x += delta_x;
							pl->point.y += delta_y;
							pl = pl->next;
						}
					}
					  
					if (edge->geometry_label_point) {
						if (context->old_edge_format != context->new_edge_format) {
							if (context->old_edge_format == cybEdgeCenter) {
								edge->geometry_label_point->x += edge->geometry_source_point->x;
								edge->geometry_label_point->y += edge->geometry_source_point->y;
							} else {
								edge->geometry_label_point->x -= edge->geometry_source_point->x;
								edge->geometry_label_point->y -= edge->geometry_source_point->y;
							}
						} else {
							cyberiada_convert_geometry(&(source_tree->rect),
													   context->old_format,
													   context->new_format,
													   &delta_x, &delta_y);
							edge->geometry_label_point->x += delta_x;
							edge->geometry_label_point->y += delta_y;
						}
					}
					
				} else {
					/* TODO: update non-trivial edges properly. But node reconstruction will help here! */
					cyberiada_clean_edge_geometry(edge);
				}
				edge = edge->next;
			}
		}
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
										CyberiadaGeometryCoordFormat new_format,
										CyberiadaGeometryEdgeFormat new_edge_format)
{
	/* NOT SUPPORTED YET */
	
	/* CyberiadaSM* sm;	 */
	/* CyberiadaRect* bounding_rect; */
	/* for (sm = doc->state_machines; sm; sm = sm->next) { */
	/* 	if (!sm->nodes || sm->nodes->next) { */
	/* 		ERROR("SM should have single root node\n"); */
	/* 		return CYBERIADA_BAD_PARAMETER; */
	/* 	} */
	/* 	cyberiada_convert_node_geometry(sm->nodes, sm->bounding_rect, */
	/* 									doc->geometry_format, new_format); */
	/* 	if (sm->edges) { */
	/* 		cyberiada_convert_edge_geometry(sm->edges, */
	/* 										doc->geometry_format, new_format, */
	/* 										doc->edge_geom_format, new_edge_format); */
	/* 	} */
	/* } */
	/* return CYBERIADA_NO_ERROR; */
	return CYBERIADA_NOT_IMPLEMENTED;
}

static int cyberiada_round_node_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->children) {
			if (node->geometry_point) {
				cyberiada_round_point(node->geometry_point, 0);
			}
			if (node->geometry_rect) {
				cyberiada_round_rect(node->geometry_rect, 0);
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
			cyberiada_round_point(edges->geometry_source_point, 0);
		}
		if (edges->geometry_target_point) {
			cyberiada_round_point(edges->geometry_target_point, 0);
		}
		if (edges->geometry_label_point) {
			cyberiada_round_point(edges->geometry_label_point, 0);
		}
		if (edges->geometry_polyline) {
			CyberiadaPolyline* pl = edges->geometry_polyline; 
			while (pl) {
				cyberiada_round_point(&(pl->point), 0);
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
	cyberiada_round_rect(doc->bounding_rect, 0);
	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_round_node_geometry(sm->nodes);
		cyberiada_round_edges_geometry(sm->edges);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_reconstruct_sm_document_bounding_rect(CyberiadaDocument* doc)
{
	CyberiadaRect* rect;
	CyberiadaSM* sm;
	CyberiadaEdge* edge;

	if (doc->bounding_rect) {
		ERROR("SM document bounding rect already available\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	/*ERROR("reconstruct sm bounding rect\n");*/
	
	rect = cyberiada_new_rect();

	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_build_bounding_rect(sm->nodes, sm->edges, rect, doc->geometry_format);
	}
	
	doc->bounding_rect = rect;

	/*ERROR("bounding rect (%lf; %lf; %lf; %lf)\n",
	  rect->x, rect->y, rect->width, rect->height);*/
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_import_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	CyberiadaGeometryCoordFormat old_format, new_format;
	CyberiadaGeometryEdgeFormat old_edge_format, new_edge_format;
	CyberiadaDocumentTransformContext* context;
	
	if (!doc) {
		ERROR("Cannot import document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (flags & CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) {
		/* NOT SUPPORTED YET */
		return CYBERIADA_NOT_IMPLEMENTED;
	}
	
	if (file_format == cybxmlYED) {
		old_format = cybCoordAbsolute;
		old_edge_format = cybEdgeCenter;
	} else if (file_format == cybxmlCyberiada10) {
		old_format = cybCoordLeftTop;
		old_edge_format = cybEdgeBorder;
	} else {
		ERROR("Bad XML format %d\n", file_format);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (flags & CYBERIADA_FLAG_ABSOLUTE_GEOMETRY) {
		new_format = cybCoordAbsolute;
	} else if (flags & CYBERIADA_FLAG_LEFTTOP_LOCAL_GEOMETRY) {
		new_format = cybCoordLeftTop;
	} else if (flags & CYBERIADA_FLAG_CENTER_LOCAL_GEOMETRY) {
		new_format = cybCoordLocalCenter;
	} else {
		ERROR("No geometry coordinates flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	if (flags & CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY) {
		new_edge_format = cybEdgeBorder;
	} else if (flags & CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY) {
		new_edge_format = cybEdgeCenter;
	} else {
		ERROR("No edge geometry flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	doc->geometry_format = old_format;
	doc->edge_geom_format = old_edge_format;
	
	if (!doc->bounding_rect) {
		cyberiada_reconstruct_sm_document_bounding_rect(doc);
	}
	
	context = cyberiada_new_transform_context(doc, new_format, new_edge_format);
	if (!context) {
		ERROR("Cannot create transformation context on import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	cyberiada_convert_node_geometry(doc, context);
	
	doc->geometry_format = new_format;
	doc->edge_geom_format = new_edge_format;

	if (flags & CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) {
		cyberiada_reconstruct_document_geometry(doc);
	}

	cyberiada_convert_edge_geometry(doc, context);
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}

	doc->bounding_rect->x = context->new_bounding_rect.x;
	doc->bounding_rect->y = context->new_bounding_rect.y;
	
	cyberiada_destroy_transform_context(context);
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_export_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	CyberiadaRect* rect;
	CyberiadaSM* sm;
	CyberiadaDocumentTransformContext* context;
	CyberiadaGeometryCoordFormat to_format;
	CyberiadaGeometryEdgeFormat to_edge_format;
	
	if (!doc) {
		ERROR("Cannot export document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (file_format == cybxmlYED) {
		to_format = cybCoordAbsolute;
		to_edge_format = cybEdgeCenter;
	} else if (file_format == cybxmlCyberiada10) {
		to_format = cybCoordLeftTop;
		to_edge_format = cybEdgeBorder;
	} else {
		ERROR("Bad XML format %d\n", file_format);
		return CYBERIADA_BAD_PARAMETER;
	}
	
	/*ERROR("export document geometry form %d to %d\n", doc->geometry_format, to_format);*/
	
	if (!doc->bounding_rect) {
		cyberiada_reconstruct_sm_document_bounding_rect(doc);
	}

	context = cyberiada_new_transform_context(doc, to_format, to_edge_format);
	if (!context) {
		ERROR("Cannot create transformation context on import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}

	cyberiada_convert_node_geometry(doc, context);
	
	if (flags & CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) {
		cyberiada_reconstruct_document_geometry(doc);
	} else if (!cyberiada_document_has_geometry(doc)) {
		cyberiada_destroy_transform_context(context);
		return CYBERIADA_NO_ERROR;
	}
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}

	cyberiada_convert_edge_geometry(doc, context);
	
	cyberiada_destroy_transform_context(context);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_node_has_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->geometry_point) {
			return 1;
		}
		if (node->geometry_rect) {
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
			if (edge->geometry_polyline) {
				return 1;
			}
			if (edge->geometry_source_point) {
				return 1;
			}
			if (edge->geometry_target_point) {
				return 1;
			}
			if (edge->geometry_label_point) {
				return 1;
			}
			edge = edge->next;
		}	
	}
	return 0;
}
