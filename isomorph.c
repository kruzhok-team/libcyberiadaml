/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The graph algorithms
 *
 * Copyright (C) 2025 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include "cyberiadaml.h"
#include "cyb_types.h"
#include "cyb_error.h"

static int cyberiada_node_size_recursively(const CyberiadaNode* node, size_t* v)
{
	size_t cnt = 0;
	const CyberiadaNode* n;
	
	if (!node) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (n = node; n; n = n->next) {
		cnt++;
		if (n->children) {
			size_t ch_cnt = 0;
			cyberiada_node_size_recursively(n->children, &ch_cnt);
			cnt += ch_cnt;
		}
	}
	
	if (v) *v = cnt;
	return CYBERIADA_NO_ERROR;
}

int cyberiada_sm_size(const CyberiadaSM* sm, size_t* v, size_t* e)
{
	size_t n_cnt = 0, e_cnt = 0;
	const CyberiadaNode* node;
	const CyberiadaEdge* edge;
	
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}

	if (sm->nodes) {
		for (node = sm->nodes->children; node; node = node->next) {
			size_t ch_cnt = 0;
			cyberiada_node_size_recursively(node, &ch_cnt);
			n_cnt += ch_cnt;
		}
	}

	if (sm->edges) {
		for (edge = sm->edges; edge; edge = edge->next) {
			e_cnt++;
		}
	}

	if (v) *v = n_cnt;
	if (e) *e = e_cnt;
	return CYBERIADA_NO_ERROR;	
}

int cyberiada_check_isomorphism(const CyberiadaSM* sm1, const CyberiadaSM* sm2,
								int* result_flags, CyberiadaNode** new_initial,
								size_t* sm_diff_nodes_size, CyberiadaNode*** sm_diff_nodes,
								size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
								size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes,
								size_t* sm_diff_edges_size, CyberiadaEdge*** sm_diff_edges,
								size_t* sm2_new_edges_size, CyberiadaEdge*** sm2_new_edges,
								size_t* sm1_missing_edges_size, CyberiadaEdge*** sm1_missing_edges)
{
	size_t sm1_vertexes = 0, sm1_edges = 0, sm2_vertexes = 0, sm2_edges = 0;
	CyberiadaStack sm1_stack, sm2_stack;
	
	if (!sm1 || !sm2 || !result_flags) {
		return CYBERIADA_BAD_PARAMETER;
	}

	*result_flags = 0;

	cyberiada_sm_size(sm1, &sm1_vertexes, &sm1_edges);
	cyberiada_sm_size(sm2, &sm2_vertexes, &sm2_edges);

	if (sm_diff_nodes_size) {
		*sm_diff_nodes_size = 0;
		if (sm_diff_nodes) {
			*sm_diff_nodes = (CyberiadaNode**)malloc(sizeof(CyberiadaNode*) * sm1_vertexes);
		}
	}
	if (sm2_new_nodes_size) {
		*sm2_new_nodes_size = 0;
		if (sm2_new_nodes) {
			*sm2_new_nodes = (CyberiadaNode**)malloc(sizeof(CyberiadaNode*) * sm2_vertexes);
		}
	}
	if (sm1_missing_nodes_size) {
		*sm1_missing_nodes_size = 0;
		if (sm1_missing_nodes) {
			*sm1_missing_nodes = (CyberiadaNode**)malloc(sizeof(CyberiadaNode*) * sm1_vertexes);
		}
	}
	if (sm2_new_edges_size) {
		*sm2_new_edges_size = 0;
		if (sm2_new_edges) {
			*sm2_new_edges = (CyberiadaEdge**)malloc(sizeof(CyberiadaEdge*) * sm2_edges);
		}
	}
	if (sm1_missing_edges_size) {
		*sm1_missing_edges_size = 0;
		if (sm1_missing_edges) {
			*sm1_missing_edges = (CyberiadaEdge**)malloc(sizeof(CyberiadaEdge*) * sm1_edges);
		}
	}
	
	return CYBERIADA_NO_ERROR;	
}

