/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The revision diff
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include <stdlib.h>
#include <string.h>

#include "cyberiadaml.h"
#include "cyberiadaml_diff.h"
#include "cyb_graph.h"
#include "cyb_string.h"
#include "cyb_error.h"

/* the evidence that two nodes of the same id are one node: the strong signs
   alone pair them, the weak ones need a second sign */
#define DIFF_EVIDENCE_STRONG   2     /* an equal non-empty title, action list or comment text */
#define DIFF_EVIDENCE_WEAK     1     /* the paired parents, the same place, the only node of its
										type, a paired neighbour by the same trigger */
#define DIFF_EVIDENCE_TO_PAIR  2

#define DIFF_PLACE_DISTANCE    20.0  /* two points nearer than this are at the same place */
#define DIFF_NO_PAIR           ((size_t)-1)

/* the edge rules, applied in this order (docs/API.md, the revision diff) */
#define DIFF_EDGE_RULE_ID      0     /* the same id with the ends paired */
#define DIFF_EDGE_RULE_TRIGGER 1     /* the paired ends and the same trigger */
#define DIFF_EDGE_RULE_ENDS    2     /* the paired ends */
#define DIFF_EDGE_RULES        3

/* the edge passes: the transitions, then the comment links to the transitions */
#define DIFF_EDGE_PASS_EDGES   0
#define DIFF_EDGE_PASS_LINKS   1
#define DIFF_EDGE_PASSES       2

typedef struct {
	CyberiadaNode*   node;
	size_t           pair;           /* the index of the paired vertex of the other side */
} DiffVertex;

typedef struct {
	CyberiadaSM*     sm;
	DiffVertex*      vertexes;       /* the nodes in the pre-order */
	size_t           size;
	DiffVertex**     by_id;          /* the vertexes sorted by the node ids */
	CyberiadaEdge**  edges;          /* the compared edges in the list order */
	size_t*          edge_pairs;     /* the index of the paired edge of the other side */
	size_t           edges_size;
} DiffSide;

typedef struct {
	DiffSide         s1;
	DiffSide         s2;
	int              flags;          /* CYBERIADA_DIFF_OPTION_* */
	double           tolerance;
	int              same_format;    /* the coordinates are comparable */
} DiffContext;

/* -----------------------------------------------------------------------------
 The element helpers
 ------------------------------------------------------------------------------*/

static int diff_string_equal(const char* a, const char* b)
{
	return cyberiada_string_compare(a, b) == 0;
}

static int diff_string_set(const char* s)
{
	return s && *s;
}

/* the type class: a simple and a composite state are one node, a comment and a
   formal comment can turn into each other */
static CyberiadaNodeType diff_type_class(CyberiadaNodeType type)
{
	if (type == cybNodeCompositeState) return cybNodeSimpleState;
	if (type == cybNodeFormalComment) return cybNodeComment;
	return type;
}

static int diff_skip_node(const CyberiadaNode* node, int flags)
{
	if (cyberiada_graph_node_is_meta(node)) return 1;
	if ((flags & CYBERIADA_DIFF_OPTION_IGNORE_COMMENTS) &&
		(node->type == cybNodeComment || node->type == cybNodeFormalComment)) return 1;
	return 0;
}

static int diff_actions_equal(const CyberiadaAction* a1, const CyberiadaAction* a2)
{
	while (a1 && a2) {
		if (a1->type != a2->type ||
			a1->propagation != a2->propagation ||
			!diff_string_equal(a1->trigger, a2->trigger) ||
			!diff_string_equal(a1->guard, a2->guard) ||
			!diff_string_equal(a1->behavior, a2->behavior)) {
			return 0;
		}
		a1 = a1->next;
		a2 = a2->next;
	}
	return a1 == NULL && a2 == NULL;
}

static const char* diff_comment_body(const CyberiadaNode* node)
{
	return node->comment_data ? node->comment_data->body : NULL;
}

static const char* diff_comment_markup(const CyberiadaNode* node)
{
	return node->comment_data ? node->comment_data->markup : NULL;
}

static const char* diff_edge_trigger(const CyberiadaEdge* edge)
{
	return edge->action ? edge->action->trigger : NULL;
}

static const char* diff_edge_guard(const CyberiadaEdge* edge)
{
	return edge->action ? edge->action->guard : NULL;
}

static double diff_abs(double d)
{
	return d < 0 ? -d : d;
}

static int diff_point_equal(const CyberiadaPoint* a, const CyberiadaPoint* b, double tolerance)
{
	if (!a || !b) return a == b;
	return diff_abs(a->x - b->x) <= tolerance && diff_abs(a->y - b->y) <= tolerance;
}

static int diff_rect_equal(const CyberiadaRect* a, const CyberiadaRect* b, double tolerance)
{
	if (!a || !b) return a == b;
	return (diff_abs(a->x - b->x) <= tolerance && diff_abs(a->y - b->y) <= tolerance &&
			diff_abs(a->width - b->width) <= tolerance && diff_abs(a->height - b->height) <= tolerance);
}

static int diff_polyline_equal(const CyberiadaPolyline* a, const CyberiadaPolyline* b, double tolerance)
{
	while (a && b) {
		if (!diff_point_equal(&(a->point), &(b->point), tolerance)) return 0;
		a = a->next;
		b = b->next;
	}
	return a == NULL && b == NULL;
}

/* the rects overlap or the points are near, in the coordinates of their parents */
static int diff_same_place(const CyberiadaNode* n1, const CyberiadaNode* n2)
{
	const CyberiadaRect *r1 = n1->geometry_rect, *r2 = n2->geometry_rect;
	const CyberiadaPoint *p1 = n1->geometry_point, *p2 = n2->geometry_point;
	if (r1 && r2) {
		double w = r1->width > r2->width ? r1->width : r2->width;
		double h = r1->height > r2->height ? r1->height : r2->height;
		return diff_abs(r1->x - r2->x) < w && diff_abs(r1->y - r2->y) < h;
	}
	if (p1 && p2) {
		return diff_abs(p1->x - p2->x) < DIFF_PLACE_DISTANCE && diff_abs(p1->y - p2->y) < DIFF_PLACE_DISTANCE;
	}
	return 0;
}

/* -----------------------------------------------------------------------------
 The sides of the diff
 ------------------------------------------------------------------------------*/

static size_t diff_count_nodes(const CyberiadaNode* nodes, int flags)
{
	size_t count = 0;
	const CyberiadaNode* n;
	for (n = nodes; n; n = n->next) {
		if (diff_skip_node(n, flags)) continue;
		count += 1 + diff_count_nodes(n->children, flags);
	}
	return count;
}

static void diff_enumerate_nodes(CyberiadaNode* nodes, int flags, DiffVertex* vertexes, size_t* index)
{
	CyberiadaNode* n;
	for (n = nodes; n; n = n->next) {
		if (diff_skip_node(n, flags)) continue;
		vertexes[*index].node = n;
		vertexes[*index].pair = DIFF_NO_PAIR;
		(*index)++;
		diff_enumerate_nodes(n->children, flags, vertexes, index);
	}
}

static int diff_compare_vertex_ids(const void* a, const void* b)
{
	const DiffVertex* va = *(const DiffVertex* const*)a;
	const DiffVertex* vb = *(const DiffVertex* const*)b;
	return cyberiada_string_compare(va->node->id, vb->node->id);
}

static int diff_compare_key_id(const void* key, const void* elem)
{
	const DiffVertex* v = *(const DiffVertex* const*)elem;
	return cyberiada_string_compare((const char*)key, v->node->id);
}

/* the index of the vertex of the id, DIFF_NO_PAIR when there is none */
static size_t diff_find_id(const DiffSide* side, const char* id)
{
	DiffVertex** found;
	if (!side->size) return DIFF_NO_PAIR;
	found = (DiffVertex**)bsearch(id ? id : EMPTY_LINE, side->by_id, side->size, sizeof(DiffVertex*),
								  diff_compare_key_id);
	return found ? (size_t)(*found - side->vertexes) : DIFF_NO_PAIR;
}

static size_t diff_vertex_of(const DiffSide* side, const CyberiadaNode* node)
{
	size_t i;
	if (!node) return DIFF_NO_PAIR;
	i = diff_find_id(side, node->id);
	return (i != DIFF_NO_PAIR && side->vertexes[i].node == node) ? i : DIFF_NO_PAIR;
}

static size_t diff_edge_index(const DiffSide* side, const CyberiadaEdge* edge)
{
	size_t i;
	for (i = 0; i < side->edges_size; i++) {
		if (side->edges[i] == edge) return i;
	}
	return DIFF_NO_PAIR;
}

static int diff_edge_compared(const DiffSide* side, const CyberiadaEdge* edge, int flags)
{
	if ((flags & CYBERIADA_DIFF_OPTION_IGNORE_COMMENTS) && edge->type == cybEdgeComment) return 0;
	if (diff_vertex_of(side, edge->source) == DIFF_NO_PAIR) return 0;
	if (edge->target_edge) return 1;
	return diff_vertex_of(side, edge->target) != DIFF_NO_PAIR;
}

static void diff_side_free(DiffSide* side)
{
	free(side->vertexes);
	free(side->by_id);
	free(side->edges);
	free(side->edge_pairs);
	memset(side, 0, sizeof(DiffSide));
}

static int diff_side_init(DiffSide* side, CyberiadaSM* sm, int flags)
{
	size_t i, index = 0;
	CyberiadaEdge* e;

	memset(side, 0, sizeof(DiffSide));
	side->sm = sm;
	if (!sm || !sm->nodes) {
		return CYBERIADA_NO_ERROR;
	}

	side->size = diff_count_nodes(sm->nodes->children, flags);
	if (side->size) {
		side->vertexes = (DiffVertex*)malloc(sizeof(DiffVertex) * side->size);
		side->by_id = (DiffVertex**)malloc(sizeof(DiffVertex*) * side->size);
		if (!side->vertexes || !side->by_id) {
			diff_side_free(side);
			return CYBERIADA_MEMORY_ERROR;
		}
		diff_enumerate_nodes(sm->nodes->children, flags, side->vertexes, &index);
		for (i = 0; i < side->size; i++) {
			side->by_id[i] = side->vertexes + i;
		}
		qsort(side->by_id, side->size, sizeof(DiffVertex*), diff_compare_vertex_ids);
	}

	for (e = sm->edges; e; e = e->next) {
		side->edges_size++;
	}
	if (side->edges_size) {
		side->edges = (CyberiadaEdge**)malloc(sizeof(CyberiadaEdge*) * side->edges_size);
		side->edge_pairs = (size_t*)malloc(sizeof(size_t) * side->edges_size);
		if (!side->edges || !side->edge_pairs) {
			diff_side_free(side);
			return CYBERIADA_MEMORY_ERROR;
		}
		side->edges_size = 0;
		for (e = sm->edges; e; e = e->next) {
			if (diff_edge_compared(side, e, flags)) {
				side->edge_pairs[side->edges_size] = DIFF_NO_PAIR;
				side->edges[side->edges_size++] = e;
			}
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 The node pairing
 ------------------------------------------------------------------------------*/

static int diff_top_level(const DiffSide* side, const CyberiadaNode* parent)
{
	return parent == NULL || parent == side->sm->nodes;
}

/* the parents of two nodes are paired with each other, or both are the SMs */
static int diff_parents_paired(const DiffContext* ctx, const CyberiadaNode* n1, const CyberiadaNode* n2)
{
	size_t i1, i2;
	if (diff_top_level(&(ctx->s1), n1->parent) || diff_top_level(&(ctx->s2), n2->parent)) {
		return diff_top_level(&(ctx->s1), n1->parent) && diff_top_level(&(ctx->s2), n2->parent);
	}
	i1 = diff_vertex_of(&(ctx->s1), n1->parent);
	i2 = diff_vertex_of(&(ctx->s2), n2->parent);
	return i1 != DIFF_NO_PAIR && i2 != DIFF_NO_PAIR && ctx->s1.vertexes[i1].pair == i2;
}

static int diff_only_of_type(const CyberiadaNode* node, int flags)
{
	const CyberiadaNode* n;
	size_t count = 0;
	if (!node->parent) return 0;
	for (n = node->parent->children; n; n = n->next) {
		if (!diff_skip_node(n, flags) && diff_type_class(n->type) == diff_type_class(node->type)) {
			count++;
		}
	}
	return count == 1;
}

static const CyberiadaNode* diff_paired_node(const DiffContext* ctx, const CyberiadaNode* n1)
{
	size_t i = diff_vertex_of(&(ctx->s1), n1);
	if (i == DIFF_NO_PAIR || ctx->s1.vertexes[i].pair == DIFF_NO_PAIR) return NULL;
	return ctx->s2.vertexes[ctx->s1.vertexes[i].pair].node;
}

/* a transition of the first node to or from a paired neighbour has its
   counterpart at the second node with the same trigger */
static int diff_paired_neighbour(const DiffContext* ctx, const CyberiadaNode* n1, const CyberiadaNode* n2)
{
	size_t i, j;
	for (i = 0; i < ctx->s1.edges_size; i++) {
		const CyberiadaEdge* e1 = ctx->s1.edges[i];
		const CyberiadaNode *other1, *other2;
		int outgoing;
		if (e1->target_edge || e1->source == e1->target) continue;
		if (e1->source == n1) {
			outgoing = 1;
			other1 = e1->target;
		} else if (e1->target == n1) {
			outgoing = 0;
			other1 = e1->source;
		} else {
			continue;
		}
		other2 = diff_paired_node(ctx, other1);
		if (!other2) continue;
		for (j = 0; j < ctx->s2.edges_size; j++) {
			const CyberiadaEdge* e2 = ctx->s2.edges[j];
			if (e2->target_edge) continue;
			if ((outgoing ? (e2->source == n2 && e2->target == other2) :
				 (e2->target == n2 && e2->source == other2)) &&
				diff_string_equal(diff_edge_trigger(e1), diff_edge_trigger(e2))) {
				return 1;
			}
		}
	}
	return 0;
}

static int diff_node_evidence(const DiffContext* ctx, const CyberiadaNode* n1, const CyberiadaNode* n2)
{
	int evidence = 0;
	if (diff_string_set(n1->title) && diff_string_equal(n1->title, n2->title)) {
		evidence += DIFF_EVIDENCE_STRONG;
	}
	if (n1->actions && diff_actions_equal(n1->actions, n2->actions)) {
		evidence += DIFF_EVIDENCE_STRONG;
	}
	if (diff_string_set(diff_comment_body(n1)) &&
		diff_string_equal(diff_comment_body(n1), diff_comment_body(n2))) {
		evidence += DIFF_EVIDENCE_STRONG;
	}
	if (diff_parents_paired(ctx, n1, n2)) {
		evidence += DIFF_EVIDENCE_WEAK;
		if (ctx->same_format && diff_same_place(n1, n2)) {
			evidence += DIFF_EVIDENCE_WEAK;
		}
		if (diff_only_of_type(n1, ctx->flags) && diff_only_of_type(n2, ctx->flags)) {
			evidence += DIFF_EVIDENCE_WEAK;
		}
	}
	if (diff_paired_neighbour(ctx, n1, n2)) {
		evidence += DIFF_EVIDENCE_WEAK;
	}
	return evidence;
}

/* the sweeps over the first side, parents before children, until no new pair:
   a pair made late may give the evidence to another one */
static void diff_pair_nodes(DiffContext* ctx)
{
	size_t i, j;
	int paired;
	do {
		paired = 0;
		for (i = 0; i < ctx->s1.size; i++) {
			DiffVertex* v1 = ctx->s1.vertexes + i;
			DiffVertex* v2;
			if (v1->pair != DIFF_NO_PAIR) continue;
			j = diff_find_id(&(ctx->s2), v1->node->id);
			if (j == DIFF_NO_PAIR) continue;
			v2 = ctx->s2.vertexes + j;
			if (v2->pair != DIFF_NO_PAIR ||
				diff_type_class(v1->node->type) != diff_type_class(v2->node->type)) continue;
			if (diff_node_evidence(ctx, v1->node, v2->node) >= DIFF_EVIDENCE_TO_PAIR) {
				v1->pair = j;
				v2->pair = i;
				paired = 1;
			}
		}
	} while (paired);
}

/* -----------------------------------------------------------------------------
 The edge pairing
 ------------------------------------------------------------------------------*/

static const CyberiadaEdge* diff_paired_edge(const DiffContext* ctx, const CyberiadaEdge* e1)
{
	size_t i = diff_edge_index(&(ctx->s1), e1);
	if (i == DIFF_NO_PAIR || ctx->s1.edge_pairs[i] == DIFF_NO_PAIR) return NULL;
	return ctx->s2.edges[ctx->s1.edge_pairs[i]];
}

static int diff_source_paired(const DiffContext* ctx, const CyberiadaEdge* e1, const CyberiadaEdge* e2)
{
	const CyberiadaNode* n = diff_paired_node(ctx, e1->source);
	return n && n == e2->source;
}

static int diff_target_paired(const DiffContext* ctx, const CyberiadaEdge* e1, const CyberiadaEdge* e2)
{
	if (e1->target_edge || e2->target_edge) {
		return e1->target_edge && e2->target_edge && diff_paired_edge(ctx, e1->target_edge) == e2->target_edge;
	} else {
		const CyberiadaNode* n = diff_paired_node(ctx, e1->target);
		return n && n == e2->target;
	}
}

static int diff_edge_rule(const DiffContext* ctx, const CyberiadaEdge* e1, const CyberiadaEdge* e2, int rule)
{
	int source = diff_source_paired(ctx, e1, e2);
	int target = diff_target_paired(ctx, e1, e2);
	int trigger = diff_string_equal(diff_edge_trigger(e1), diff_edge_trigger(e2));
	switch (rule) {
	case DIFF_EDGE_RULE_ID:
		return (diff_string_equal(e1->id, e2->id) &&
				((source && target) ||
				 ((source || target) && trigger && diff_string_equal(diff_edge_guard(e1), diff_edge_guard(e2)))));
	case DIFF_EDGE_RULE_TRIGGER:
		return source && target && trigger;
	case DIFF_EDGE_RULE_ENDS:
		return source && target;
	default:
		return 0;
	}
}

static void diff_pair_edges(DiffContext* ctx)
{
	size_t i, j;
	int pass, rule;
	for (pass = 0; pass < DIFF_EDGE_PASSES; pass++) {
		for (rule = 0; rule < DIFF_EDGE_RULES; rule++) {
			for (i = 0; i < ctx->s1.edges_size; i++) {
				const CyberiadaEdge* e1 = ctx->s1.edges[i];
				if (ctx->s1.edge_pairs[i] != DIFF_NO_PAIR) continue;
				if ((e1->target_edge != NULL) != (pass == DIFF_EDGE_PASS_LINKS)) continue;
				for (j = 0; j < ctx->s2.edges_size; j++) {
					const CyberiadaEdge* e2 = ctx->s2.edges[j];
					if (ctx->s2.edge_pairs[j] != DIFF_NO_PAIR) continue;
					if ((e1->type == cybEdgeComment) != (e2->type == cybEdgeComment)) continue;
					if (diff_edge_rule(ctx, e1, e2, rule)) {
						ctx->s1.edge_pairs[i] = j;
						ctx->s2.edge_pairs[j] = i;
						break;
					}
				}
			}
		}
	}
}

/* -----------------------------------------------------------------------------
 The differences of the pairs
 ------------------------------------------------------------------------------*/

static int diff_simple_composite(CyberiadaNodeType t1, CyberiadaNodeType t2)
{
	return ((t1 == cybNodeSimpleState && t2 == cybNodeCompositeState) ||
			(t1 == cybNodeCompositeState && t2 == cybNodeSimpleState));
}

static unsigned int diff_node_flags(const DiffContext* ctx, CyberiadaNode* n1, CyberiadaNode* n2,
									int* action_flags)
{
	unsigned int flags = 0;
	*action_flags = 0;
	if (n1->type != n2->type && !diff_simple_composite(n1->type, n2->type)) {
		flags |= CYBERIADA_DIFF_FLAG_TYPE;
	}
	if (!diff_string_equal(n1->title, n2->title) || !diff_string_equal(n1->formal_title, n2->formal_title)) {
		flags |= CYBERIADA_DIFF_FLAG_TITLE;
	}
	if (!diff_actions_equal(n1->actions, n2->actions)) {
		flags |= CYBERIADA_DIFF_FLAG_ACTIONS;
		cyberiada_compare_node_actions(n1->actions, n2->actions, action_flags);
	}
	if (!diff_parents_paired(ctx, n1, n2)) {
		flags |= CYBERIADA_DIFF_FLAG_PARENT;
	}
	if ((n1->link == NULL) != (n2->link == NULL) ||
		(n1->link && n2->link && !diff_string_equal(n1->link->ref, n2->link->ref))) {
		flags |= CYBERIADA_DIFF_FLAG_LINK;
	}
	if (!diff_string_equal(diff_comment_body(n1), diff_comment_body(n2)) ||
		!diff_string_equal(diff_comment_markup(n1), diff_comment_markup(n2))) {
		flags |= CYBERIADA_DIFF_FLAG_COMMENT;
	}
	if (!diff_string_equal(n1->color, n2->color)) {
		flags |= CYBERIADA_DIFF_FLAG_COLOR;
	}
	if (ctx->flags & CYBERIADA_DIFF_OPTION_GEOMETRY) {
		if (!diff_rect_equal(n1->geometry_rect, n2->geometry_rect, ctx->tolerance) ||
			!diff_point_equal(n1->geometry_point, n2->geometry_point, ctx->tolerance)) {
			flags |= CYBERIADA_DIFF_FLAG_GEOMETRY;
		}
		if (n1->collapsed_flag != n2->collapsed_flag) {
			flags |= CYBERIADA_DIFF_FLAG_COLLAPSED;
		}
	}
	return flags;
}

static int diff_subject_equal(const CyberiadaCommentSubject* s1, const CyberiadaCommentSubject* s2)
{
	if (!s1 || !s2) return s1 == s2;
	return s1->type == s2->type && diff_string_equal(s1->fragment, s2->fragment);
}

static unsigned int diff_edge_flags(const DiffContext* ctx, const CyberiadaEdge* e1, const CyberiadaEdge* e2)
{
	unsigned int flags = 0;
	const CyberiadaAction *a1 = e1->action, *a2 = e2->action;
	if (e1->type != e2->type) {
		flags |= CYBERIADA_DIFF_FLAG_TYPE;
	}
	if (!diff_source_paired(ctx, e1, e2)) {
		flags |= CYBERIADA_DIFF_FLAG_SOURCE;
	}
	if (!diff_target_paired(ctx, e1, e2)) {
		flags |= CYBERIADA_DIFF_FLAG_TARGET;
	}
	if (!diff_string_equal(a1 ? a1->trigger : NULL, a2 ? a2->trigger : NULL)) {
		flags |= CYBERIADA_DIFF_FLAG_TRIGGER;
	}
	if (!diff_string_equal(a1 ? a1->guard : NULL, a2 ? a2->guard : NULL)) {
		flags |= CYBERIADA_DIFF_FLAG_GUARD;
	}
	if (!diff_string_equal(a1 ? a1->behavior : NULL, a2 ? a2->behavior : NULL)) {
		flags |= CYBERIADA_DIFF_FLAG_BEHAVIOR;
	}
	if ((a1 ? a1->propagation : cybEventPropagationNone) != (a2 ? a2->propagation : cybEventPropagationNone)) {
		flags |= CYBERIADA_DIFF_FLAG_PROPAGATION;
	}
	if (!diff_string_equal(e1->color, e2->color)) {
		flags |= CYBERIADA_DIFF_FLAG_COLOR;
	}
	if (!diff_subject_equal(e1->comment_subject, e2->comment_subject)) {
		flags |= CYBERIADA_DIFF_FLAG_SUBJECT;
	}
	if ((ctx->flags & CYBERIADA_DIFF_OPTION_GEOMETRY) &&
		(!diff_point_equal(e1->geometry_label_point, e2->geometry_label_point, ctx->tolerance) ||
		 !diff_rect_equal(e1->geometry_label_rect, e2->geometry_label_rect, ctx->tolerance) ||
		 !diff_polyline_equal(e1->geometry_polyline, e2->geometry_polyline, ctx->tolerance) ||
		 !diff_point_equal(e1->geometry_source_point, e2->geometry_source_point, ctx->tolerance) ||
		 !diff_point_equal(e1->geometry_target_point, e2->geometry_target_point, ctx->tolerance))) {
		flags |= CYBERIADA_DIFF_FLAG_GEOMETRY;
	}
	return flags;
}

static unsigned int diff_sm_node_flags(const DiffContext* ctx, const CyberiadaNode* r1, const CyberiadaNode* r2)
{
	unsigned int flags = 0;
	if (!r1 || !r2) return 0;
	if (!diff_string_equal(r1->title, r2->title)) {
		flags |= CYBERIADA_DIFF_FLAG_TITLE;
	}
	if (!diff_string_equal(r1->color, r2->color)) {
		flags |= CYBERIADA_DIFF_FLAG_COLOR;
	}
	if ((ctx->flags & CYBERIADA_DIFF_OPTION_GEOMETRY) &&
		!diff_rect_equal(r1->geometry_rect, r2->geometry_rect, ctx->tolerance)) {
		flags |= CYBERIADA_DIFF_FLAG_GEOMETRY;
	}
	return flags;
}

/* -----------------------------------------------------------------------------
 The state machine diff
 ------------------------------------------------------------------------------*/

static int diff_sm_pair(CyberiadaSM* sm1, CyberiadaSM* sm2, const CyberiadaDiffOptions* options,
						int same_format, CyberiadaSMDiff* result)
{
	DiffContext ctx;
	size_t i, n;
	int res;

	memset(result, 0, sizeof(CyberiadaSMDiff));
	memset(&ctx, 0, sizeof(DiffContext));
	ctx.flags = options->flags;
	ctx.tolerance = options->geometry_tolerance;
	ctx.same_format = same_format;
	if ((res = diff_side_init(&(ctx.s1), sm1, ctx.flags)) != CYBERIADA_NO_ERROR) {
		return res;
	}
	if ((res = diff_side_init(&(ctx.s2), sm2, ctx.flags)) != CYBERIADA_NO_ERROR) {
		diff_side_free(&(ctx.s1));
		return res;
	}

	result->sm1 = sm1;
	result->sm2 = sm2;
	if (!sm1) {
		result->flags = CYBERIADA_DIFF_FLAG_ADDED;
	} else if (!sm2) {
		result->flags = CYBERIADA_DIFF_FLAG_REMOVED;
	} else {
		diff_pair_nodes(&ctx);
		diff_pair_edges(&ctx);
		result->flags = diff_sm_node_flags(&ctx, sm1->nodes, sm2->nodes);
	}

	n = ctx.s1.size + ctx.s2.size;
	if (n) {
		result->nodes = (CyberiadaNodeDiff*)malloc(sizeof(CyberiadaNodeDiff) * n);
	}
	n = ctx.s1.edges_size + ctx.s2.edges_size;
	if (n) {
		result->edges = (CyberiadaEdgeDiff*)malloc(sizeof(CyberiadaEdgeDiff) * n);
	}
	if ((ctx.s1.size + ctx.s2.size && !result->nodes) ||
		(ctx.s1.edges_size + ctx.s2.edges_size && !result->edges)) {
		diff_side_free(&(ctx.s1));
		diff_side_free(&(ctx.s2));
		cyberiada_cleanup_sm_diff(result);
		return CYBERIADA_MEMORY_ERROR;
	}

	for (i = 0; i < ctx.s2.size; i++) {
		CyberiadaNodeDiff* d = result->nodes + result->nodes_size++;
		DiffVertex* v2 = ctx.s2.vertexes + i;
		d->n2 = v2->node;
		d->action_flags = 0;
		if (v2->pair == DIFF_NO_PAIR) {
			d->n1 = NULL;
			d->flags = CYBERIADA_DIFF_FLAG_ADDED;
		} else {
			d->n1 = ctx.s1.vertexes[v2->pair].node;
			d->flags = diff_node_flags(&ctx, d->n1, d->n2, &(d->action_flags));
		}
	}
	for (i = 0; i < ctx.s1.size; i++) {
		if (ctx.s1.vertexes[i].pair == DIFF_NO_PAIR) {
			CyberiadaNodeDiff* d = result->nodes + result->nodes_size++;
			d->n1 = ctx.s1.vertexes[i].node;
			d->n2 = NULL;
			d->flags = CYBERIADA_DIFF_FLAG_REMOVED;
			d->action_flags = 0;
		}
	}
	for (i = 0; i < ctx.s2.edges_size; i++) {
		CyberiadaEdgeDiff* d = result->edges + result->edges_size++;
		d->e2 = ctx.s2.edges[i];
		if (ctx.s2.edge_pairs[i] == DIFF_NO_PAIR) {
			d->e1 = NULL;
			d->flags = CYBERIADA_DIFF_FLAG_ADDED;
		} else {
			d->e1 = ctx.s1.edges[ctx.s2.edge_pairs[i]];
			d->flags = diff_edge_flags(&ctx, d->e1, d->e2);
		}
	}
	for (i = 0; i < ctx.s1.edges_size; i++) {
		if (ctx.s1.edge_pairs[i] == DIFF_NO_PAIR) {
			CyberiadaEdgeDiff* d = result->edges + result->edges_size++;
			d->e1 = ctx.s1.edges[i];
			d->e2 = NULL;
			d->flags = CYBERIADA_DIFF_FLAG_REMOVED;
		}
	}

	for (i = 0; i < result->nodes_size; i++) {
		if (result->nodes[i].flags) {
			result->flags |= CYBERIADA_DIFF_FLAG_ELEMENTS;
			break;
		}
	}
	for (i = 0; i < result->edges_size; i++) {
		if (result->edges[i].flags) {
			result->flags |= CYBERIADA_DIFF_FLAG_ELEMENTS;
			break;
		}
	}

	diff_side_free(&(ctx.s1));
	diff_side_free(&(ctx.s2));
	return CYBERIADA_NO_ERROR;
}

void cyberiada_default_diff_options(CyberiadaDiffOptions* options)
{
	if (!options) return;
	options->flags = 0;
	options->geometry_tolerance = CYBERIADA_DIFF_GEOMETRY_TOLERANCE;
}

int cyberiada_diff_sms(CyberiadaSM* sm1, CyberiadaSM* sm2,
					   const CyberiadaDiffOptions* options, CyberiadaSMDiff* result)
{
	CyberiadaDiffOptions defaults;
	if (!sm1 || !sm2 || !result) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!options) {
		cyberiada_default_diff_options(&defaults);
		options = &defaults;
	}
	return diff_sm_pair(sm1, sm2, options, 1, result);
}

int cyberiada_cleanup_sm_diff(CyberiadaSMDiff* result)
{
	if (!result) {
		return CYBERIADA_BAD_PARAMETER;
	}
	free(result->nodes);
	free(result->edges);
	memset(result, 0, sizeof(CyberiadaSMDiff));
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 The document diff
 ------------------------------------------------------------------------------*/

static const char* diff_transition_order(const CyberiadaMetainformation* meta)
{
	if (!meta || !meta->transition_order_flag) return NULL;
	return meta->transition_order_flag == 1 ? CYBERIADA_META_AO_ACTION : CYBERIADA_META_AO_EXIT;
}

static const char* diff_event_propagation(const CyberiadaMetainformation* meta)
{
	if (!meta || !meta->event_propagation_flag) return NULL;
	return meta->event_propagation_flag == 1 ? CYBERIADA_META_EP_BLOCK : CYBERIADA_META_EP_PROPAGATE;
}

/* the first value of a parameter: a document repeating one keeps its first */
static const CyberiadaMetaStringList* diff_meta_find(const CyberiadaMetainformation* meta, const char* name)
{
	const CyberiadaMetaStringList* s;
	for (s = meta ? meta->strings : NULL; s; s = s->next) {
		if (diff_string_equal(s->name, name)) return s;
	}
	return NULL;
}

static int diff_meta_add(CyberiadaDocumentDiff* result, const char* name, const char* value1, const char* value2)
{
	CyberiadaMetaDiff* m;
	if ((value1 == NULL) == (value2 == NULL) && diff_string_equal(value1, value2)) {
		return CYBERIADA_NO_ERROR;
	}
	m = result->meta + result->meta_size++;
	memset(m, 0, sizeof(CyberiadaMetaDiff));
	if (cyberiada_copy_string(&(m->name), NULL, name) != CYBERIADA_NO_ERROR ||
		cyberiada_copy_string(&(m->value1), NULL, value1) != CYBERIADA_NO_ERROR ||
		cyberiada_copy_string(&(m->value2), NULL, value2) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_MEMORY_ERROR;
	}
	return CYBERIADA_NO_ERROR;
}

static size_t diff_meta_strings(const CyberiadaMetainformation* meta)
{
	size_t n = 0;
	const CyberiadaMetaStringList* s;
	for (s = meta ? meta->strings : NULL; s; s = s->next) {
		n++;
	}
	return n;
}

#define DIFF_META_FIXED 3   /* standardVersion, transitionOrder, eventPropagation */

static int diff_meta(const CyberiadaDocument* doc1, const CyberiadaDocument* doc2, CyberiadaDocumentDiff* result)
{
	const CyberiadaMetainformation *m1 = doc1->meta_info, *m2 = doc2->meta_info;
	const CyberiadaMetaStringList* s;
	size_t n = DIFF_META_FIXED + diff_meta_strings(m1) + diff_meta_strings(m2);
	int res;

	result->meta = (CyberiadaMetaDiff*)malloc(sizeof(CyberiadaMetaDiff) * n);
	if (!result->meta) {
		return CYBERIADA_MEMORY_ERROR;
	}
	if ((res = diff_meta_add(result, CYBERIADA_META_STANDARD_VERSION,
							 m1 ? m1->standard_version : NULL, m2 ? m2->standard_version : NULL)) != CYBERIADA_NO_ERROR ||
		(res = diff_meta_add(result, CYBERIADA_META_TRANSITION_ORDER,
							 diff_transition_order(m1), diff_transition_order(m2))) != CYBERIADA_NO_ERROR ||
		(res = diff_meta_add(result, CYBERIADA_META_EVENT_PROPAGATION,
							 diff_event_propagation(m1), diff_event_propagation(m2))) != CYBERIADA_NO_ERROR) {
		return res;
	}
	for (s = m1 ? m1->strings : NULL; s; s = s->next) {
		const CyberiadaMetaStringList* s2;
		if (diff_meta_find(m1, s->name) != s) continue;
		s2 = diff_meta_find(m2, s->name);
		if ((res = diff_meta_add(result, s->name, s->value, s2 ? s2->value : NULL)) != CYBERIADA_NO_ERROR) {
			return res;
		}
	}
	for (s = m2 ? m2->strings : NULL; s; s = s->next) {
		if (diff_meta_find(m2, s->name) != s || diff_meta_find(m1, s->name)) continue;
		if ((res = diff_meta_add(result, s->name, NULL, s->value)) != CYBERIADA_NO_ERROR) {
			return res;
		}
	}
	return CYBERIADA_NO_ERROR;
}

static size_t diff_sm_count(const CyberiadaDocument* doc)
{
	size_t n = 0;
	const CyberiadaSM* sm;
	for (sm = doc->state_machines; sm; sm = sm->next) {
		n++;
	}
	return n;
}

static int diff_formats_equal(const CyberiadaDocument* doc1, const CyberiadaDocument* doc2)
{
	return (doc1->node_coord_format == doc2->node_coord_format &&
			doc1->edge_coord_format == doc2->edge_coord_format &&
			doc1->edge_pl_coord_format == doc2->edge_pl_coord_format &&
			doc1->edge_geom_format == doc2->edge_geom_format);
}

/* the SM passes: by the id of the SM node, by its title, in the document order */
#define DIFF_SM_PASS_ID     0
#define DIFF_SM_PASS_TITLE  1
#define DIFF_SM_PASS_ORDER  2
#define DIFF_SM_PASSES      3

static int diff_sm_match(const CyberiadaSM* sm1, const CyberiadaSM* sm2, int pass)
{
	const CyberiadaNode *r1 = sm1->nodes, *r2 = sm2->nodes;
	switch (pass) {
	case DIFF_SM_PASS_ID:
		return r1 && r2 && diff_string_equal(r1->id, r2->id);
	case DIFF_SM_PASS_TITLE:
		return r1 && r2 && diff_string_set(r1->title) && diff_string_equal(r1->title, r2->title);
	case DIFF_SM_PASS_ORDER:
		return 1;
	default:
		return 0;
	}
}

int cyberiada_diff_documents(CyberiadaDocument* doc1, CyberiadaDocument* doc2,
							 const CyberiadaDiffOptions* options, CyberiadaDocumentDiff* result)
{
	CyberiadaDiffOptions defaults;
	CyberiadaSM **sms1 = NULL, **sms2 = NULL, *sm;
	size_t *pairs1 = NULL, *pairs2 = NULL, n1, n2, i, j;
	int res = CYBERIADA_NO_ERROR, pass, same_format;

	if (!doc1 || !doc2 || !result) {
		return CYBERIADA_BAD_PARAMETER;
	}
	memset(result, 0, sizeof(CyberiadaDocumentDiff));
	if (!options) {
		cyberiada_default_diff_options(&defaults);
		options = &defaults;
	}
	same_format = diff_formats_equal(doc1, doc2);
	if ((options->flags & CYBERIADA_DIFF_OPTION_GEOMETRY) && !same_format &&
		cyberiada_document_has_geometry(doc1) && cyberiada_document_has_geometry(doc2)) {
		ERROR("The documents to compare the geometry of have different coordinate formats\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if ((res = diff_meta(doc1, doc2, result)) != CYBERIADA_NO_ERROR) {
		cyberiada_cleanup_document_diff(result);
		return res;
	}
	if (result->meta_size) {
		result->flags |= CYBERIADA_DIFF_FLAG_META;
	}

	n1 = diff_sm_count(doc1);
	n2 = diff_sm_count(doc2);
	if (n1 + n2 == 0) {
		return CYBERIADA_NO_ERROR;
	}
	sms1 = (CyberiadaSM**)malloc(sizeof(CyberiadaSM*) * (n1 + 1));
	sms2 = (CyberiadaSM**)malloc(sizeof(CyberiadaSM*) * (n2 + 1));
	pairs1 = (size_t*)malloc(sizeof(size_t) * (n1 + 1));
	pairs2 = (size_t*)malloc(sizeof(size_t) * (n2 + 1));
	result->sms = (CyberiadaSMDiff*)malloc(sizeof(CyberiadaSMDiff) * (n1 + n2));
	if (!sms1 || !sms2 || !pairs1 || !pairs2 || !result->sms) {
		res = CYBERIADA_MEMORY_ERROR;
		goto finish;
	}
	for (i = 0, sm = doc1->state_machines; sm; sm = sm->next, i++) {
		sms1[i] = sm;
		pairs1[i] = DIFF_NO_PAIR;
	}
	for (j = 0, sm = doc2->state_machines; sm; sm = sm->next, j++) {
		sms2[j] = sm;
		pairs2[j] = DIFF_NO_PAIR;
	}
	for (pass = 0; pass < DIFF_SM_PASSES; pass++) {
		for (i = 0; i < n1; i++) {
			if (pairs1[i] != DIFF_NO_PAIR) continue;
			for (j = 0; j < n2; j++) {
				if (pairs2[j] == DIFF_NO_PAIR && diff_sm_match(sms1[i], sms2[j], pass)) {
					pairs1[i] = j;
					pairs2[j] = i;
					break;
				}
			}
		}
	}

	for (j = 0; j < n2; j++) {
		res = diff_sm_pair(pairs2[j] == DIFF_NO_PAIR ? NULL : sms1[pairs2[j]], sms2[j], options, same_format,
						   result->sms + result->sms_size);
		if (res != CYBERIADA_NO_ERROR) goto finish;
		result->flags |= result->sms[result->sms_size++].flags;
	}
	for (i = 0; i < n1; i++) {
		if (pairs1[i] != DIFF_NO_PAIR) continue;
		res = diff_sm_pair(sms1[i], NULL, options, same_format, result->sms + result->sms_size);
		if (res != CYBERIADA_NO_ERROR) goto finish;
		result->flags |= result->sms[result->sms_size++].flags;
	}

finish:
	free(sms1);
	free(sms2);
	free(pairs1);
	free(pairs2);
	if (res != CYBERIADA_NO_ERROR) {
		cyberiada_cleanup_document_diff(result);
	}
	return res;
}

int cyberiada_cleanup_document_diff(CyberiadaDocumentDiff* result)
{
	size_t i;
	if (!result) {
		return CYBERIADA_BAD_PARAMETER;
	}
	for (i = 0; i < result->sms_size; i++) {
		cyberiada_cleanup_sm_diff(result->sms + i);
	}
	free(result->sms);
	for (i = 0; result->meta && i < result->meta_size; i++) {
		free(result->meta[i].name);
		free(result->meta[i].value1);
		free(result->meta[i].value2);
	}
	free(result->meta);
	memset(result, 0, sizeof(CyberiadaDocumentDiff));
	return CYBERIADA_NO_ERROR;
}
