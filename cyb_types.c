/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The basic data structures
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cyb_types.h"
#include "cyb_actions.h"
#include "cyb_meta.h"

CyberiadaCommentData* cyberiada_new_comment_data(void)
{
	CyberiadaCommentData* cd = (CyberiadaCommentData*)malloc(sizeof(CyberiadaCommentData));
	memset(cd, 0, sizeof(CyberiadaCommentData));
	return cd;
}

static CyberiadaCommentData* cyberiada_copy_comment_data(CyberiadaCommentData* src)
{
	CyberiadaCommentData* cd;
	if (!src) {
		return NULL;
	}
	cd = cyberiada_new_comment_data();
	if (src->body) {
		cyberiada_copy_string(&(cd->body), &(cd->body_len), src->body);
	}
	if (src->markup) {
		cyberiada_copy_string(&(cd->markup), &(cd->markup_len), src->markup);
	}
	return cd;
}

CyberiadaLink* cyberiada_new_link(const char* ref)
{
	CyberiadaLink* link = (CyberiadaLink*)malloc(sizeof(CyberiadaLink));
	memset(link, 0, sizeof(CyberiadaLink));
	cyberiada_copy_string(&(link->ref), &(link->ref_len), ref);
	return link;
}

static CyberiadaLink* cyberiada_copy_link(CyberiadaLink* src)
{
	if (!src || !(src->ref)) {
		return NULL;
	}
	return cyberiada_new_link(src->ref);
}

CyberiadaAction* cyberiada_new_action(CyberiadaActionType type,
									  const char* trigger,
									  const char* guard,
									  const char* behavior)
{
	CyberiadaAction* action = (CyberiadaAction*)malloc(sizeof(CyberiadaAction));
	memset(action, 0, sizeof(CyberiadaAction));
	action->type = type;
	cyberiada_copy_string(&(action->trigger), &(action->trigger_len), trigger);
	cyberiada_copy_string(&(action->guard), &(action->guard_len), guard);
	cyberiada_copy_string(&(action->behavior), &(action->behavior_len), behavior);
	return action;
}

static CyberiadaAction* cyberiada_copy_action(CyberiadaAction* src)
{
	if (!src) {
		return NULL;
	}
	return cyberiada_new_action(src->type, src->trigger, src->guard, src->behavior);
}

CyberiadaNode* cyberiada_new_node(const char* id)
{
	CyberiadaNode* new_node = (CyberiadaNode*)malloc(sizeof(CyberiadaNode));
	memset(new_node, 0, sizeof(CyberiadaNode));
	cyberiada_copy_string(&(new_node->id), &(new_node->id_len), id);
	new_node->type = cybNodeSimpleState;
	return new_node;
}

static CyberiadaNode* cyberiada_copy_node(CyberiadaNode* src)
{
	CyberiadaNode *dst, *n, *dst_child, *src_child;
	CyberiadaAction *a, *dst_action, *src_action;
	if (!src || !(src->id)) {
		return NULL;
	}
	dst = cyberiada_new_node(src->id);
	dst->type = src->type;
	if (src->title) {
		cyberiada_copy_string(&(dst->title), &(dst->title_len), src->title);
	}
	if (src->formal_title) {
		cyberiada_copy_string(&(dst->formal_title), &(dst->formal_title_len), src->formal_title);
	}
	if (src->geometry_point) {
		dst->geometry_point = htree_copy_point(src->geometry_point);
	}
	if (src->geometry_rect) {
		dst->geometry_rect = htree_copy_rect(src->geometry_rect);
	}
	dst->collapsed_flag = src->collapsed_flag;
	if (src->color) {
		cyberiada_copy_string(&(dst->color), &(dst->color_len), src->color);
	}
	if (src->link) {
		dst->link = cyberiada_copy_link(src->link);
	}
	if (src->comment_data) {
		dst->comment_data = cyberiada_copy_comment_data(src->comment_data);
	}
	if (src->actions) {
		for (src_action = src->actions; src_action; src_action = src_action->next) {
			dst_action = cyberiada_copy_action(src_action);
			if (dst->actions) {
				a = dst->actions;
				while (a->next) a = a->next;
				a->next = dst_action;
			} else {
				dst->actions = dst_action;
			}
		}
	}
	if (src->children) {
		for (src_child = src->children; src_child; src_child = src_child->next) {
			dst_child = cyberiada_copy_node(src_child);
			dst_child->parent = dst;
			if (dst->children) {
				n = dst->children;
				while (n->next) n = n->next;
				n->next = dst_child;
			} else {
				dst->children = dst_child;
			}
		}
	}
	return dst;
}

static int cyberiada_destroy_action(CyberiadaAction* action)
{
	CyberiadaAction* a;
	if (action != NULL) {
		do {
			a = action;
			if (a->trigger) free(a->trigger);
			if (a->guard) free(a->guard);
			if (a->behavior) free(a->behavior);
			action = a->next;
			free(a);
		} while (action);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_all_nodes(CyberiadaNode* node);

static int cyberiada_destroy_node(CyberiadaNode* node)
{
	if(node != NULL) {
		if (node->id) free(node->id);
		if (node->title) free(node->title);
		if (node->formal_title) free(node->title);
		if (node->children) {
			cyberiada_destroy_all_nodes(node->children);
		}
		if (node->actions) cyberiada_destroy_action(node->actions);
		if (node->geometry_point) htree_destroy_point(node->geometry_point);
		if (node->geometry_rect) htree_destroy_rect(node->geometry_rect);
		if (node->color) free(node->color);
		if (node->link) {
			if (node->link->ref) free(node->link->ref);
			free(node->link);
		}
		if (node->comment_data) {
			if (node->comment_data->body) free(node->comment_data->body);
			if (node->comment_data->markup) free(node->comment_data->markup);
			free(node->comment_data);
		}
		free(node);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_all_nodes(CyberiadaNode* node)
{
	CyberiadaNode* n;
	if(node != NULL) {
		do {
			n = node;
			node = node->next;
			cyberiada_destroy_node(n);
		} while (node);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_update_complex_state(CyberiadaNode* node, CyberiadaNode* parent)
{
	if (parent && parent->type == cybNodeSimpleState &&
		node->type != cybNodeComment && node->type != cybNodeFormalComment) {

		parent->type = cybNodeCompositeState;
	}
	return CYBERIADA_NO_ERROR;
}

CyberiadaCommentSubject* cyberiada_new_comment_subject(CyberiadaCommentSubjectType type)
{
	CyberiadaCommentSubject* cs = (CyberiadaCommentSubject*)malloc(sizeof(CyberiadaCommentSubject));
	memset(cs, 0, sizeof(CyberiadaCommentSubject));
	cs->type = type;
	return cs;
}

static CyberiadaCommentSubject* cyberiada_copy_comment_subject(CyberiadaCommentSubject* src)
{
	CyberiadaCommentSubject* dst;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_comment_subject(src->type);
	if (src->fragment) {
		cyberiada_copy_string(&(dst->fragment), &(dst->fragment_len), src->fragment);
	}
	return dst;
}

CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target, int external)
{
	CyberiadaEdge* new_edge;
	if (!source || !target) {
		return NULL;
	}
	new_edge = (CyberiadaEdge*)malloc(sizeof(CyberiadaEdge));
	memset(new_edge, 0, sizeof(CyberiadaEdge));
	if (external) {
		new_edge->type = cybEdgeExternalTransition;
	} else {
		new_edge->type = cybEdgeLocalTransition;
	}
	cyberiada_copy_string(&(new_edge->id), &(new_edge->id_len), id);
	cyberiada_copy_string(&(new_edge->source_id), &(new_edge->source_id_len), source);
	cyberiada_copy_string(&(new_edge->target_id), &(new_edge->target_id_len), target);
	return new_edge;
}

static CyberiadaEdge* cyberiada_copy_edge(CyberiadaEdge* src)
{
	CyberiadaEdge* dst;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_edge(src->id, src->source_id, src->target_id, 1);
	dst->type = src->type;
	if (src->action) {
		dst->action = cyberiada_copy_action(src->action);
	}
	if (src->comment_subject) {
		dst->comment_subject = cyberiada_copy_comment_subject(src->comment_subject);
	}
    if (src->geometry_polyline) {
		dst->geometry_polyline = htree_copy_polyline(src->geometry_polyline);
	}
	if (src->geometry_source_point) {
		dst->geometry_source_point = htree_copy_point(src->geometry_source_point);
	}
	if (src->geometry_target_point) {
		dst->geometry_target_point = htree_copy_point(src->geometry_target_point);
	}
	if (src->geometry_label_point) {
		dst->geometry_label_point = htree_copy_point(src->geometry_label_point);
	}
	if (src->geometry_label_rect) {
		dst->geometry_label_rect = htree_copy_rect(src->geometry_label_rect);
	}
	if (src->color) {
		cyberiada_copy_string(&(dst->color), &(dst->color_len), src->color);		
	}
	return dst;
}

static int cyberiada_destroy_edge(CyberiadaEdge* e)
{
	if (!e) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (e->id) free(e->id);
	if (e->source_id) free(e->source_id);
	if (e->target_id) free(e->target_id);
	if (e->action) cyberiada_destroy_action(e->action);
	if (e->comment_subject) {
		if (e->comment_subject->fragment) free(e->comment_subject->fragment);
		free(e->comment_subject);
	}
	if (e->geometry_polyline) {
		htree_destroy_polyline(e->geometry_polyline);
	}
	if (e->geometry_source_point) htree_destroy_point(e->geometry_source_point); 
	if (e->geometry_target_point) htree_destroy_point(e->geometry_target_point);
	if (e->geometry_label_point) htree_destroy_point(e->geometry_label_point);
	if (e->geometry_label_rect) htree_destroy_rect(e->geometry_label_rect);
	if (e->color) free(e->color);
	free(e);
	return CYBERIADA_NO_ERROR;
}

CyberiadaSM* cyberiada_new_sm(void)
{
	CyberiadaSM* sm = (CyberiadaSM*)malloc(sizeof(CyberiadaSM));
	memset(sm, 0, sizeof(CyberiadaSM));
	return sm;
}

int cyberiada_destroy_sm(CyberiadaSM* sm)
{
	CyberiadaEdge *edge, *e;
	if (sm) {
		if (sm->nodes) {
			cyberiada_destroy_all_nodes(sm->nodes);
		}
		if (sm->edges) {
			edge = sm->edges;
			do {
				e = edge;
				edge = edge->next;
				cyberiada_destroy_edge(e);
			} while (edge);
		}
		free(sm);
	}
	return CYBERIADA_NO_ERROR;
}

static CyberiadaSM* cyberiada_copy_sm(CyberiadaSM* src)
{
	CyberiadaSM* dst;
	CyberiadaNode* node, *new_node, *prev_node;
	CyberiadaEdge *edge, *new_edge, *prev_edge;

	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_sm();
	if (src->nodes) {
		node = src->nodes;
		while (node) {
			new_node = cyberiada_copy_node(node);
			if (dst->nodes) {
				prev_node->next = new_node;
			} else {
				dst->nodes = new_node;
			}
			prev_node = new_node;
			node = node->next;
		}
	}
	if (src->edges) {
		edge = src->edges; 
		while (edge) {
			new_edge = cyberiada_copy_edge(edge);
			if (dst->edges) {
				prev_edge->next = new_edge;	
			} else {
				dst->edges = new_edge;
			}
			prev_edge = new_edge;
			edge = edge->next;	
		}
	}
	/* update links to the source & target nodes */
	edge = dst->edges;
	while (edge) {
		CyberiadaNode* source = cyberiada_graph_find_node_by_id(dst->nodes, edge->source_id);
		CyberiadaNode* target = cyberiada_graph_find_node_by_id(dst->nodes, edge->target_id);
		if (!source || !target) {
			cyberiada_destroy_sm(dst);
			return NULL;
		}
		edge->source = source;
		edge->target = target;
		edge = edge->next;
	}
	return dst;
}

CyberiadaDocument* cyberiada_new_sm_document(void)
{
	CyberiadaDocument* doc = (CyberiadaDocument*)malloc(sizeof(CyberiadaDocument));
	cyberiada_init_sm_document(doc);
	return doc;
}

CyberiadaDocument* cyberiada_copy_sm_document(CyberiadaDocument* src)
{
	CyberiadaDocument* dst;
	CyberiadaSM *sm, *new_sm, *prev_sm;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_sm_document();
	cyberiada_copy_string(&(dst->format), &(dst->format_len), src->format);
	if (src->meta_info) {
		dst->meta_info = cyberiada_copy_meta(src->meta_info);
	}
	if (src->state_machines) {
		sm = src->state_machines;
		while (sm) {
			new_sm = cyberiada_copy_sm(sm);
			if (dst->state_machines) {
				prev_sm->next = new_sm;
			} else {
				dst->state_machines = new_sm;
			}
			prev_sm = new_sm;
			sm = sm->next;
		}
	}
	dst->geometry_format = src->geometry_format;
	dst->node_coord_format = src->node_coord_format;
	dst->edge_coord_format = src->edge_coord_format;
	dst->edge_pl_coord_format = src->edge_pl_coord_format;
	dst->edge_geom_format = src->edge_geom_format;
	if (src->bounding_rect) {
		dst->bounding_rect = htree_copy_rect(src->bounding_rect);
	}
	return dst;
}

int cyberiada_init_sm_document(CyberiadaDocument* doc)
{
	if (doc) {
		memset(doc, 0, sizeof(CyberiadaDocument));
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_cleanup_sm_document(CyberiadaDocument* doc)
{
	CyberiadaSM *sm, *sm2;
	if (doc) {
		if (doc->format) {
			free(doc->format);
		}
		if (doc->meta_info) {
			cyberiada_destroy_meta(doc->meta_info);
		}
		if (doc->state_machines) {
			sm = doc->state_machines;
			do {
				sm2 = sm;
				sm = sm->next;
				cyberiada_destroy_sm(sm2);
			} while (sm);
		}
		if (doc->bounding_rect) {
			htree_destroy_rect(doc->bounding_rect);
		}
		cyberiada_init_sm_document(doc);
	}
	return CYBERIADA_NO_ERROR;	
}

int cyberiada_destroy_sm_document(CyberiadaDocument* doc)
{
	int res = cyberiada_cleanup_sm_document(doc);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}
	free(doc);
	return CYBERIADA_NO_ERROR;
}	

int cyberiada_print_node(CyberiadaNode* node, int level)
{
	CyberiadaNode* cur_node;
	char levelspace[16];
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}

	printf("%sNode {id: %s, title: \"%s\", type: %d",
		   levelspace, node->id, node->title, (int)node->type);
	if (node->formal_title) {
		printf(", formal title: \"%s\"", node->formal_title);
	}
	printf("}\n");
	if (node->collapsed_flag) {
		printf("%sCollapsed\n", levelspace);
	}
	if (node->color) {
		printf("%sColor: %s\n", levelspace, node->color);
	}
	if (node->type == cybNodeSM ||
		node->type == cybNodeSimpleState ||
		node->type == cybNodeCompositeState ||
		node->type == cybNodeSubmachineState ||
		node->type == cybNodeComment ||
		node->type == cybNodeFormalComment ||
		node->type == cybNodeChoice) {
		
		if (node->type == cybNodeSubmachineState && node->link && node->link->ref) {
			printf("%sLink to SM: %s\n", levelspace, node->link->ref);
		} else if (node->type == cybNodeComment && node->comment_data) {
			printf("%sComment data [markup: %s]:\n", levelspace,
				   node->comment_data->markup ? node->comment_data->markup : "");
			printf("%s%s\n", levelspace, node->comment_data->body);
		}
		if (node->geometry_rect) {
			printf("%sGeometry: (%lf, %lf, %lf, %lf)\n",
			   levelspace,
				   node->geometry_rect->x,
				   node->geometry_rect->y,
				   node->geometry_rect->width,
				   node->geometry_rect->height);
		}
	} else {
		if (node->geometry_point) {
			printf("%sGeometry: (%lf, %lf)\n",
				   levelspace,
				   node->geometry_point->x,
				   node->geometry_point->y);
		}
	}
	
	cyberiada_print_action(node->actions, level + 1);
	
	printf("%sChildren:\n", levelspace);
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, level + 1);
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_print_edge(CyberiadaEdge* edge)
{
	CyberiadaPolyline* polyline;
	printf(" Edge %s [%s %s]->[%s %s] [type %d]\n",
		   edge->id,
		   edge->source_id,
		   edge->source->type == cybNodeInitial ? "INIT" : edge->source->title,
		   edge->target_id,
		   edge->target->type == cybNodeInitial ? "INIT" : edge->target->title,
		   edge->type);
	if (edge->color) {
		printf("  Color: %s\n", edge->color);
	}
	if (edge->comment_subject) {
		printf("  Comment subject [type: %d]\n", edge->comment_subject->type);
		if (edge->comment_subject->fragment) {
			printf("   Fragment: %s\n", edge->comment_subject->fragment);
		}
	}
	printf("  Geometry: ");
	if (edge->geometry_polyline) {
		printf("   Polyline:");
		for (polyline = edge->geometry_polyline; polyline; polyline = polyline->next) {
			printf(" (%lf, %lf)", polyline->point.x, polyline->point.y);
		}
		printf("\n");
	}
	if (edge->geometry_source_point) {
		printf("   Source point: (%lf, %lf)\n",
			   edge->geometry_source_point->x,
			   edge->geometry_source_point->y);
	}
	if (edge->geometry_target_point) {
		printf("   Target point: (%lf, %lf)\n",
			   edge->geometry_target_point->x,
			   edge->geometry_target_point->y);
	}
	if (edge->geometry_label_point) {
		printf("   Label point: (%lf, %lf)\n",
			   edge->geometry_label_point->x,
			   edge->geometry_label_point->y);
	} else if (edge->geometry_label_rect) {
		printf("   Label rect: (%lf, %lf, %lf, %lf)\n",
			   edge->geometry_label_rect->x,
			   edge->geometry_label_rect->y,
			   edge->geometry_label_rect->width,
			   edge->geometry_label_rect->height);
	}
	cyberiada_print_action(edge->action, 2);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_sm(CyberiadaSM* sm)
{
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;
	size_t nodes_cnt = 0, edges_cnt = 0;
	size_t nodes_cnt_wo_cmt = 0, edges_cnt_wo_cmt = 0;

	cyberiada_sm_size(sm, &nodes_cnt_wo_cmt, &edges_cnt_wo_cmt, 1, 0);
	cyberiada_sm_size(sm, &nodes_cnt, &edges_cnt, 0, 0);
	
	printf("State Machine\n");
	
	printf("Nodes: %lu (%lu w/o comments)\n", nodes_cnt, nodes_cnt_wo_cmt);
	for (cur_node = sm->nodes; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, 0);
	}
	printf("\n");

	printf("Edges: %lu (%lu w/o comments)\n", edges_cnt, edges_cnt_wo_cmt);
	for (cur_edge = sm->edges; cur_edge; cur_edge = cur_edge->next) {
		cyberiada_print_edge(cur_edge);
	}
	printf("\n");

	/*if (sm->extensions) {
		printf("Extensions:\n");
		for (cur_ext = sm->extensions; cur_ext; cur_ext = cur_ext->next) {
			cyberiada_print_extension(cur_ext);
		}
		printf("\n");
		}*/
	
    return CYBERIADA_NO_ERROR;
}

int cyberiada_print_sm_document(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;

	printf("\nDocument:\n");
	cyberiada_print_meta(doc->meta_info);

	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_print_sm(sm);
	}

	if (doc->bounding_rect) {
		printf("\nBounding rect: (%lf, %lf, %lf, %lf)\n",
			   doc->bounding_rect->x,
			   doc->bounding_rect->y,
			   doc->bounding_rect->width,
			   doc->bounding_rect->height);
	}
	
    return CYBERIADA_NO_ERROR;
}
