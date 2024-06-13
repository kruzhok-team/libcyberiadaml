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

static double round_number(double num)
{
    double value = (int)(num * 1000.0 + .5);
    return (double)value / 1000.0;
}

static void cyberiada_round_point(CyberiadaPoint* p)
{
	if (!p) return ;
	p->x = round_number(p->x);
	p->y = round_number(p->y);
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

static void cyberiada_update_rect(CyberiadaRect* r, double dx, double dy)
{
	if (!r) return ;
	r->x += dx;
	r->y += dy;
}

static void cyberiada_round_rect(CyberiadaRect* r)
{
	if (!r) return ;
	r->x = round_number(r->x);
	r->y = round_number(r->y);
	r->width = round_number(r->width);
	r->height = round_number(r->height);
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

static int cyberiada_extend_rect_p(CyberiadaRect* rect,
								   double x, double y,
								   CyberiadaGeometryCoordFormat format)
{
	double delta;
	if (!rect) {
		ERROR("Cannot extend rect with point\n");
		return CYBERIADA_BAD_PARAMETER;
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
			rect->x += delta;
		}
		if (y < rect->y) {
			delta = rect->y - y;
			rect->height += delta;
			rect->y -= delta;
		}
		if (y > rect->y + rect->height) {
			delta = y - rect->y - rect->height;
			rect->height += delta;
			rect->y += delta;
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
								   CyberiadaRect* rect,
								   CyberiadaGeometryCoordFormat format)
{
	if (!result || !rect) {
		ERROR("Cannot extend rect with rect\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (format == cybCoordLocalCenter) {
		cyberiada_extend_rect_p(result,
								rect->x - rect->width / 2.0,
								rect->y - rect->height / 2.0,
								format);
		cyberiada_extend_rect_p(result,
								rect->x + rect->width / 2.0,
								rect->y + rect->height / 2.0,
								format);
	} else {
		cyberiada_extend_rect_p(result,
								rect->x,
								rect->y,
								format);
		cyberiada_extend_rect_p(result,
								rect->x + rect->width,
								rect->y + rect->height,
								format);
	}	
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
		*delta_x = rect->x - rect->x - rect->width / 2.0;
		*delta_y = rect->y - rect->y - rect->height / 2.0;
	} else if (from_format == cybCoordLeftTop && to_format == cybCoordLocalCenter) {
		/* (x, y, w, h) -> (x - pw/2, y - ph/2, w, h) */
		*delta_x = rect->x - rect->width / 2.0;
		*delta_y = rect->y - rect->height / 2.0;
	} else if (from_format == cybCoordLeftTop && to_format == cybCoordAbsolute) {
		/* (x, y, w, h) -> (x + px, y + py, w, h) */
		*delta_x = rect->x;
		*delta_y = rect->y;
	} else if (from_format == cybCoordLocalCenter && to_format == cybCoordAbsolute) {
		/* (x, y, w, h) -> (x + px + pw/2, y + py + ph/2, w, h) */
		*delta_x = rect->x + rect->x + rect->width / 2.0;
		*delta_y = rect->y + rect->y + rect->height / 2.0;
	} else if (from_format == cybCoordLocalCenter && to_format == cybCoordLeftTop) {
        /* (x, y, w, h) -> (x + pw/2, y + ph/2, w, h) */		
		*delta_x = rect->x + rect->width / 2.0;
		*delta_y = rect->y + rect->height / 2.0;
	} else {
		ERROR("bad node coordinates conversion from %d to %d\n", from_format, to_format);
		return CYBERIADA_BAD_PARAMETER;
	}	
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_coordinates(CyberiadaNode* parent,
										 double x, double y,
										 double* to_x, double* to_y,
										 CyberiadaGeometryCoordFormat from_format,
										 CyberiadaGeometryCoordFormat to_format)
{
	if (!to_x || !to_y) {
		ERROR("Cannot convert coordinates\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!parent || !parent->geometry_rect || from_format == to_format) {
		*to_x = x;
		*to_y = y;
	} else {
		CyberiadaRect* rect = parent->geometry_rect;
		double delta_x, delta_y;
		cyberiada_convert_geometry(rect, from_format, to_format, &delta_x, &delta_y);
		cyberiada_convert_coordinates(parent->parent,
									  x + delta_x, y + delta_y,
									  to_x, to_y,
									  from_format,
									  to_format);
	}
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_build_bounding_rect(CyberiadaNode* node,
										 CyberiadaRect* result,
										 CyberiadaGeometryCoordFormat format)
{
	if (!node || !result) {
		ERROR("Cannot build bounding rect\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	while (node) {
		if (node->geometry_point) {
			cyberiada_extend_rect_p(result,
									node->geometry_point->x,
									node->geometry_point->y,
									format);
		}
		if (node->geometry_rect) {
			cyberiada_extend_rect_r(result,
									node->geometry_rect,
									format);
		}
		if (node->children) {
			int res = cyberiada_build_bounding_rect(node->children, result, format);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_node_geometry(CyberiadaNode* node,
										   CyberiadaRect* parent_rect,
										   CyberiadaGeometryCoordFormat from_format,
										   CyberiadaGeometryCoordFormat to_format)
{
	double delta_x, delta_y;

	if (!node || !parent_rect) {
		ERROR("Cannot convert node geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (from_format == to_format) {
		return CYBERIADA_NO_ERROR;
	}
	
	cyberiada_convert_geometry(parent_rect, from_format, to_format, &delta_x, &delta_y);
	
	while (node) {
		if (node->geometry_point) {
			cyberiada_update_point(node->geometry_point, delta_x, delta_y);
		}
		if (node->geometry_rect) {
			cyberiada_update_rect(node->geometry_rect, delta_x, delta_y);
		}
		if (node->children) {
			int res = cyberiada_convert_node_geometry(node->children,
													  node->geometry_rect ? node->geometry_rect: parent_rect,
													  from_format,
													  to_format);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node = node->next;
	}
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
										 CyberiadaPoint* point,
										 CyberiadaGeometryCoordFormat format)
{
	int res;
	CyberiadaRect* bound;
	if (!node || !point) return 0;
	if (!node->geometry_rect) return 0;
	
	bound = cyberiada_copy_rect(node->geometry_rect);
	cyberiada_build_bounding_rect(node, bound, format);

	res = cyberiada_rect_contains_point(bound, point, format);
	free(bound);
	return res;
}

static int cyberiada_reconstruct_nodes(CyberiadaNode* node,
									   CyberiadaRect* rect,
									   CyberiadaGeometryCoordFormat format)
{
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

	CyberiadaNode* start_node = node;
	if (!node && !rect) {
		ERROR("Cannot reconstruct node geometry\n");
		return CYBERIADA_BAD_PARAMETER;	
	}

	while (node) {
		if (cyberiada_is_point_node(node) && !node->geometry_point) {
			CyberiadaPoint* new_point = cyberiada_new_point();
			CyberiadaNode* n;
			int intersects;
			do {
				if (format == cybCoordAbsolute) {
					new_point->x = rect->x + rect->width * rand();
					new_point->y = rect->y + rect->height * rand();					
				} else if (format == cybCoordLeftTop) {
					new_point->x = rect->width * rand();
					new_point->y = rect->height * rand();					
				} else if (format == cybCoordLocalCenter) {
					new_point->x = rect->width * (-0.5 + rand());
					new_point->y = rect->height * (-0.5 + rand());
				} else {
					free(new_point);
					return CYBERIADA_BAD_PARAMETER;
				}

				intersects = 0;
				for (n = start_node; n->next; n = n->next) {
					if (n == node) continue;
					if (n->geometry_rect && cyberiada_rect_contains_point(n->geometry_rect, new_point, format)) {
						intersects = 1;
						break;
					}
				}
				
			} while (intersects);
			
			node->geometry_point = new_point;
		}

		if (cyberiada_is_rect_node(node) && !node->geometry_rect) {
			CyberiadaRect* new_rect = cyberiada_new_rect();
			CyberiadaNode* n;
			int intersects;
			double node_width = DEFAULT_NODE_SIZE, node_height = DEFAULT_NODE_SIZE;
			if (node_width < rect->width) {
				node_width = rect->width / 2.0;
			}
			if (node_height < rect->height) {
				node_height = rect->height / 2.0;
			}
			do {
				new_rect->width = node_width;
				new_rect->height = node_height;
				if (format == cybCoordAbsolute) {
					new_rect->x = rect->x + (rect->width - node_width) * rand();
					new_rect->y = rect->y + (rect->height - node_height) * rand();
				} else if (format == cybCoordLeftTop) {
					new_rect->x = (rect->width - node_width) * rand();
					new_rect->y = (rect->height - node_height) * rand();					
				} else if (format == cybCoordLocalCenter) {
					new_rect->x = (rect->width - node_width) * (-0.5 + rand());
					new_rect->y = (rect->height - node_height) * (-0.5 + rand());
				} else {
					free(new_rect);
					return CYBERIADA_BAD_PARAMETER;
				}

				intersects = 0;
				for (n = start_node; n->next; n = n->next) {
					if (n == node) continue;
					if (n->geometry_rect
						&& cyberiada_rect_contains_rect(n->geometry_rect, new_rect, format)) {
						intersects = 1;
						break;
					}
				}
				
			} while (intersects);
			
			node->geometry_rect = new_rect;
		}
		
		if (node->children) {
			cyberiada_build_bounding_rect(node->children, node->geometry_rect, format);			
			int res = cyberiada_reconstruct_nodes(node->children, node->geometry_rect, format);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_reconstruct_edge(CyberiadaPoint** source_point,
									  CyberiadaNode* source,
									  CyberiadaPoint** target_point,
									  CyberiadaNode* target,
									  CyberiadaGeometryCoordFormat format,
									  CyberiadaGeometryEdgeFormat edge_format)
{
	CyberiadaPoint* point;
	
	if (!source_point || !source || !target_point || !target) {
		ERROR("Cannot reconstruct edge\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (edge_format == cybEdgeLocalCenter) {

		if (!*source_point) {
			*source_point = cyberiada_new_point();
		}

		if (!*target_point) {
			*target_point = cyberiada_new_point();
		}

		return CYBERIADA_NO_ERROR;
		
	}

	return CYBERIADA_NO_ERROR;	
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
}

static int cyberiada_reconstruct_edges(CyberiadaEdge* edge,
									   CyberiadaGeometryCoordFormat format,
									   CyberiadaGeometryEdgeFormat edge_format)
{
	while (edge) {
		if (edge->source && (edge->source->geometry_rect || edge->source->geometry_point) &&
			edge->target && (edge->target->geometry_rect || edge->target->geometry_point)) {

			if (!edge->geometry_source_point || !edge->geometry_target_point) {
				cyberiada_reconstruct_edge(&(edge->geometry_source_point),
										   edge->source,
										   &(edge->geometry_target_point),
										   edge->target,
										   format,
										   edge_format);
			}

		}
		edge = edge->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;
	if (!doc || !doc->state_machines) {
		ERROR("Cannot round SM document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_reconstruct_nodes(sm->nodes, sm->bounding_rect, doc->geometry_format);
		cyberiada_reconstruct_edges(sm->edges, doc->geometry_format, doc->edge_geom_format);
	}
	return CYBERIADA_NO_ERROR;
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
		if (sm->bounding_rect) {
			free(sm->bounding_rect);
			sm->bounding_rect = NULL;
		}
	}

	doc->geometry_format = cybCoordNone;
	doc->edge_geom_format = cybEdgeNone;
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_convert_edges_geometry(CyberiadaEdge* edges,
											CyberiadaRect* parent_rect,
											CyberiadaGeometryCoordFormat from_format,
											CyberiadaGeometryCoordFormat to_format)
{
/*	CyberiadaSM* sm;
	CyberiadaNode* node;
	CyberiadaEdge* edge;
	CyberiadaPolyline* pl;
	double from_x, from_y, to_x, to_y;
	double src_from_x, src_from_y, src_to_x, src_to_y, tgt_from_x, tgt_from_y, tgt_to_x, tgt_to_y;
				
	for (edge = edges; edge; edge = edge->next) {
		if (edge->source && (edge->source->geometry_rect || edge->source->geometry_point) &&
			edge->target && (edge->target->geometry_rect || edge->target->geometry_point) &&
			(edge->geometry_source_point ||
			 edge->geometry_target_point ||
			 edge->geometry_polyline ||
			 edge->geometry_label_point)) {

			from_x = from_y = to_x = to_y = 0.0;
			if (edge->source->geometry_point) {
				from_x += edge->source->geometry_point->x;
				from_y += edge->source->geometry_point->y;
			} else {
				from_x += edge->source->geometry_rect->x + edge->source->geometry_rect->width / 2.0;
				from_y += edge->source->geometry_rect->y + edge->source->geometry_rect->height / 2.0;
				if (edge->geometry_source_point) {
					from_x += edge->geometry_source_point->x;
					from_y += edge->geometry_source_point->y;
				}
			}				
			if (edge->target->geometry_point) {
				to_x += edge->target->geometry_point->x;
				to_y += edge->target->geometry_point->y;
			} else {
				to_x += edge->target->geometry_rect->x + edge->target->geometry_rect->width / 2.0;
				to_y += edge->target->geometry_rect->y + edge->target->geometry_rect->height / 2.0;
				if (edge->geometry_target_point) {
					to_x += edge->geometry_target_point->x;
					to_y += edge->geometry_target_point->y;
				}
			}
				
			if (edge->geometry_polyline) {
				double first_p_x = edge->geometry_polyline->point.x;
				double first_p_y = edge->geometry_polyline->point.y;
				double last_p_x;
				double last_p_y;

				pl = edge->geometry_polyline;
				while (pl->next) pl = pl->next;
				last_p_x = pl->point.x;
				last_p_y = pl->point.y;
					
				src_from_x = from_x;
				src_from_y = from_y;
				src_to_x = first_p_x;
				src_to_y = first_p_y;
				tgt_from_x = last_p_x;
				tgt_from_y = last_p_y;
				tgt_to_x = to_x;
				tgt_to_y = to_y;			
			} else {
				src_from_x = tgt_from_x = from_x;
				src_from_y = tgt_from_y = from_y;
				src_to_x = tgt_to_x = to_x;
				src_to_y = tgt_to_y = to_y;
			}

			DEBUG("edge (%f; %f) -> (%f; %f) and (%f; %f) -> (%f; %f)\n",
				  src_from_x, src_from_y, src_to_x, src_to_y,
				  tgt_from_x, tgt_from_y, tgt_to_x, tgt_to_y);
				
			if (edge->geometry_source_point) {
				if (edge->source->geometry_point) {
					if (src_from_x == src_to_x) {
						edge->geometry_source_point->x = 0.0;
						if (src_from_y >= src_to_y) {
							edge->geometry_source_point->y = -PSEUDO_NODE_SIZE / 2.0;
						} else {
							edge->geometry_source_point->y = PSEUDO_NODE_SIZE / 2.0;							
						}
					} else {
						double alpha = atan((src_from_y - src_to_y) / (src_to_x - src_from_x));
						edge->geometry_source_point->x = cos(alpha) * PSEUDO_NODE_SIZE / 2.0;
						edge->geometry_source_point->y = -sin(alpha) * PSEUDO_NODE_SIZE / 2.0;
					}
				} else {
					if (src_from_x == src_to_x) {
						edge->geometry_source_point->x = edge->source->geometry_rect->width / 2.0;
						if (src_from_y >= src_to_y) {
							edge->geometry_source_point->y = 0.0;
						} else {
							edge->geometry_source_point->y = edge->source->geometry_rect->height;
						}
					} else {
						double alpha = atan((src_from_y - src_to_y) / (src_to_x - src_from_x));
						if (src_to_x < src_from_x) alpha += M_PI;
						double alpha_g = 180.0 * alpha / M_PI;
							DEBUG("src alpha %f\n", alpha_g);
						if (alpha_g < 0.0) alpha_g += 360.0;
						if (alpha_g <= 45.0 || alpha_g > 315.0) {
							edge->geometry_source_point->x = edge->source->geometry_rect->width;
							edge->geometry_source_point->y += (-tan(alpha) * edge->source->geometry_rect->width +
															   edge->source->geometry_rect->height) / 2.0;
						} else if (alpha_g > 45.0 && alpha_g <= 135.0) {
							edge->geometry_source_point->x += ((double)tan(alpha) * edge->source->geometry_rect->height +
															   edge->source->geometry_rect->width) / 2.0;
							edge->geometry_source_point->y = 0.0;
						} else if (alpha_g > 135.0 && alpha_g <= 225.0) {
							edge->geometry_source_point->x = 0.0;
							edge->geometry_source_point->y += (-(double)tan(alpha) * edge->source->geometry_rect->width +
															   edge->source->geometry_rect->height) / 2.0;
						} else {
							edge->geometry_source_point->x += (-(double)tan(alpha) * edge->source->geometry_rect->height +
															   edge->source->geometry_rect->width) / 2.0;
							edge->geometry_source_point->y = edge->source->geometry_rect->height;
						}

						if (edge->geometry_source_point->x < 0) {
							edge->geometry_source_point->x = 0.0;
						}
						if (edge->geometry_source_point->x > edge->source->geometry_rect->width) {
							edge->geometry_source_point->x = edge->source->geometry_rect->width;
						}
						if (edge->geometry_source_point->y < 0) {
							edge->geometry_source_point->y = 0.0;
						}
						if (edge->geometry_source_point->y > edge->source->geometry_rect->height) {
							edge->geometry_source_point->y = edge->source->geometry_rect->height;
						}
					}
				}
				DEBUG("sp: (%f; %f)\n", edge->geometry_source_point->x, edge->geometry_source_point->y); 
			}

			if (edge->geometry_target_point) {
				if (edge->target->geometry_point) {
					if (tgt_from_x == tgt_to_x) {
						edge->geometry_target_point->x = 0.0;
						if (tgt_from_y >= tgt_to_y) {
							edge->geometry_target_point->y = PSEUDO_NODE_SIZE / 2.0;
						} else {
							edge->geometry_target_point->y = -PSEUDO_NODE_SIZE / 2.0;							
						}
					} else {
						double alpha = atan((tgt_from_y - tgt_to_y) / (tgt_to_x - tgt_from_x));
						edge->geometry_target_point->x = cos(alpha) * PSEUDO_NODE_SIZE / 2.0;
						edge->geometry_target_point->y = -sin(alpha) * PSEUDO_NODE_SIZE / 2.0;
					}
				} else {
					if (tgt_from_x == tgt_to_x) {
						edge->geometry_target_point->x = edge->target->geometry_rect->width / 2.0;
						if (tgt_from_y >= tgt_to_y) {
							edge->geometry_target_point->y = edge->target->geometry_rect->height;
						} else {
							edge->geometry_target_point->y = 0.0;
						}
					} else {
						double alpha = atan((tgt_from_y - tgt_to_y) / (tgt_to_x - tgt_from_x));
						if (tgt_to_x < tgt_from_x) alpha += M_PI;
						alpha += M_PI; /* target = incoming edge */
	/*double alpha_g = 180.0 * alpha / M_PI;
							DEBUG("tgt alpha %f\n", alpha_g);
						if (alpha_g < 0.0) alpha_g += 360.0;
						if (alpha_g <= 45.0 || alpha_g > 315.0) {
							edge->geometry_target_point->x = edge->target->geometry_rect->width;
							edge->geometry_target_point->y += (-tan(alpha) * edge->target->geometry_rect->width +
															   edge->target->geometry_rect->height) / 2.0;
						} else if (alpha_g > 45.0 && alpha_g <= 135.0) {
							edge->geometry_target_point->x += (tan(alpha) * edge->target->geometry_rect->height +
															   edge->target->geometry_rect->width) / 2.0;
							edge->geometry_target_point->y = 0.0;
						} else if (alpha_g > 135.0 && alpha_g <= 225.0) {
							edge->geometry_target_point->x = 0.0;
							edge->geometry_target_point->y += (-tan(alpha) * edge->target->geometry_rect->width +
															   edge->target->geometry_rect->height) / 2.0;
						} else {
							edge->geometry_target_point->x += (-tan(alpha) * edge->target->geometry_rect->height +
															   edge->target->geometry_rect->width) / 2.0;
							edge->geometry_target_point->y = edge->target->geometry_rect->height;
						}
					}

					if (edge->geometry_target_point->x < 0) {
						edge->geometry_target_point->x = 0.0;
					}
					if (edge->geometry_target_point->x > edge->target->geometry_rect->width) {
						edge->geometry_target_point->x = edge->target->geometry_rect->width;
					}
					if (edge->geometry_target_point->y < 0) {
						edge->geometry_target_point->y = 0.0;
					}
					if (edge->geometry_target_point->y > edge->target->geometry_rect->height) {
						edge->geometry_target_point->y = edge->target->geometry_rect->height;
					}
				}
				DEBUG("tp: (%f; %f)\n", edge->geometry_target_point->x, edge->geometry_target_point->y);
			}				
		}
	}*/
	return CYBERIADA_NO_ERROR;
}	

static int cyberiada_convert_edge_geometry(CyberiadaEdge* edge,
										   CyberiadaGeometryCoordFormat from_format,
										   CyberiadaGeometryCoordFormat to_format,
										   CyberiadaGeometryEdgeFormat from_edge_format,
										   CyberiadaGeometryEdgeFormat to_edge_format)
{
	while (edge) {
		if (edge->source && (edge->source->geometry_rect || edge->source->geometry_point) &&
			edge->target && (edge->target->geometry_rect || edge->target->geometry_point)) {
			/*
			if (edge->geometry_source_point) {

			}

			if (edge->geometry_target_point) {
				cyberiada_convert_edge_source_target_geometry(edge,
															  from_format,
															  to_fotmat);
			}
			
			if (edge->geometry_polyline) {
			}

			if (edge->geometry_label_point) {
			}*/
		} else {
			/* TODO: update non-trivial edges properly */
			cyberiada_clean_edge_geometry(edge);
		}
		edge = edge->next;
	}
	return CYBERIADA_NO_ERROR;
}	

int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
										CyberiadaGeometryCoordFormat new_format,
										CyberiadaGeometryEdgeFormat new_edge_format)
{
	CyberiadaSM* sm;	
	CyberiadaRect* bounding_rect;
	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (!sm->nodes || sm->nodes->next) {
			ERROR("SM should have single root node\n");
			return CYBERIADA_BAD_PARAMETER;
		}
		cyberiada_convert_node_geometry(sm->nodes, sm->bounding_rect,
										doc->geometry_format, new_format);
		if (sm->edges) {
			cyberiada_convert_edge_geometry(sm->edges,
											doc->geometry_format, new_format,
											doc->edge_geom_format, new_edge_format);
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_round_node_geometry(CyberiadaNode* node)
{
	while (node) {
		if (node->children) {
			if (node->geometry_point) {
				cyberiada_round_point(node->geometry_point);
			}
			if (node->geometry_rect) {
				cyberiada_round_rect(node->geometry_rect);
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
			cyberiada_round_point(edges->geometry_source_point);
		}
		if (edges->geometry_target_point) {
			cyberiada_round_point(edges->geometry_target_point);
		}
		if (edges->geometry_label_point) {
			cyberiada_round_point(edges->geometry_label_point);
		}
		if (edges->geometry_polyline) {
			CyberiadaPolyline* pl = edges->geometry_polyline; 
			while (pl) {
				cyberiada_round_point(&(pl->point));
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
	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_round_rect(sm->bounding_rect);
		cyberiada_round_node_geometry(sm->nodes);
		cyberiada_round_edges_geometry(sm->edges);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_import_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	CyberiadaSM* sm;
	CyberiadaGeometryCoordFormat old_format, new_format;
	CyberiadaGeometryEdgeFormat old_edge_format, new_edge_format;
	
	if (!doc) {
		ERROR("Cannot import document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (file_format == cybxmlYED) {
		old_format = cybCoordAbsolute;
		old_edge_format = cybEdgeLocalCenter;
	} else if (file_format == cybxmlCyberiada10) {
		old_format = cybCoordLeftTop;
		old_edge_format = cybEdgeLeftTopBorder;
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

	if (flags & CYBERIADA_FLAG_CENTER_BORDER_EDGE_GEOMETRY) {
		new_edge_format = cybEdgeCenterBorder;
	} else if (flags & CYBERIADA_FLAG_LEFTTOP_BORDER_EDGE_GEOMETRY) {
		new_edge_format = cybEdgeLeftTopBorder;
	} else if (flags & CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY) {
		new_edge_format = cybEdgeLocalCenter;
	} else {
		ERROR("No edge geometry flag for import\n");
		return CYBERIADA_BAD_PARAMETER;		
	}
	
	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (sm->nodes->geometry_rect) {
			sm->bounding_rect = cyberiada_copy_rect(sm->nodes->geometry_rect);
		} else {
			CyberiadaRect* bounding_rect = cyberiada_new_rect();
			bounding_rect->x = bounding_rect->y = bounding_rect->width = bounding_rect->height = 0.0;
			cyberiada_build_bounding_rect(sm->nodes, bounding_rect, old_format);
			sm->bounding_rect = bounding_rect;
		}

		cyberiada_convert_node_geometry(sm->nodes, sm->bounding_rect,
										old_format, new_format);
	}

	doc->geometry_format = new_format;
	doc->edge_geom_format = new_edge_format;

	if (flags & CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) {
		cyberiada_reconstruct_document_geometry(doc);
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (sm->edges) {
			cyberiada_convert_edge_geometry(sm->edges,
											old_format, new_format,
											old_edge_format, new_edge_format);
		}
	}
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_export_document_geometry(CyberiadaDocument* doc,
									   int flags, CyberiadaXMLFormat file_format)
{
	CyberiadaSM* sm;
	CyberiadaGeometryCoordFormat to_format;
	CyberiadaGeometryEdgeFormat to_edge_format;
	
	if (!doc) {
		ERROR("Cannot export document geometry\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (file_format == cybxmlYED) {
		to_format = cybCoordAbsolute;
		to_edge_format = cybEdgeLocalCenter;
	} else if (file_format == cybxmlCyberiada10) {
		to_format = cybCoordLeftTop;
		to_edge_format = cybEdgeLeftTopBorder;
	} else {
		ERROR("Bad XML format %d\n", file_format);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (flags & CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY) {
		cyberiada_reconstruct_document_geometry(doc);
	} else if (!cyberiada_document_has_geometry(doc)) {
		return CYBERIADA_NO_ERROR;
	}
	
	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		cyberiada_round_document_geometry(doc);
	}
	
	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (sm->bounding_rect) {
			cyberiada_convert_node_geometry(sm->nodes, sm->bounding_rect,
											doc->geometry_format, to_format);
		}
		if (sm->edges) {
			cyberiada_convert_edge_geometry(sm->edges,
											doc->geometry_format, to_format,
											doc->edge_geom_format, to_edge_format);
		}
	}

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
