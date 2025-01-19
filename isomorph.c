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
#include <string.h>

#include "cyberiadaml.h"
#include "cyb_types.h"
#include "cyb_error.h"

static int cyberiada_node_size_recursively(CyberiadaNode* node, size_t* v, int ignore_comments)
{
	size_t cnt = 0;
	CyberiadaNode* n;
	
	if (!node) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (n = node; n; n = n->next) {
		if (ignore_comments && (n->type == cybNodeComment || n->type == cybNodeFormalComment)) {
			continue;
		}
		cnt++;
		if (n->children) {
			size_t ch_cnt = 0;
			cyberiada_node_size_recursively(n->children, &ch_cnt, ignore_comments);
			cnt += ch_cnt;
		}
	}

	if (v) *v = cnt;

	return CYBERIADA_NO_ERROR;
}

int cyberiada_sm_size(CyberiadaSM* sm, size_t* v, size_t* e, int ignore_comments)
{
	size_t n_cnt = 0, e_cnt = 0;
	CyberiadaNode* node;
	CyberiadaEdge* edge;
	
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}

	if (sm->nodes) {
		size_t ch_cnt = 0;
		cyberiada_node_size_recursively(sm->nodes->children, &ch_cnt, ignore_comments);
		n_cnt += ch_cnt;
	}

	if (sm->edges) {
		for (edge = sm->edges; edge; edge = edge->next) {
			if (ignore_comments && edge->type == cybEdgeComment) {
				continue;
			}
			e_cnt++;
		}
	}

	if (v) *v = n_cnt;
	if (e) *e = e_cnt;
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_get_initial_pseudostate(CyberiadaSM* sm,
											 CyberiadaNode** initial_node,
											 CyberiadaEdge** initial_edge,
											 int check)
{
	CyberiadaNode* n;
	CyberiadaEdge* e;
	CyberiadaNode* init_n = NULL;
	size_t initial = 0;
	int found;

	if (!sm || !sm->nodes) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	for (n = sm->nodes->children; n; n = n->next) {
		if (n->type == cybNodeInitial) {
			initial++;
			init_n = n;
			if (initial_node) {
				*initial_node = n;
			}
		}
	}

	if (check) {
		if (initial != 1) {
			ERROR("To check graph isomorhpism the SM %s should have one single initial pseudostate on the top level\n",
				  sm->nodes->id);
			return CYBERIADA_FORMAT_ERROR;
		}
		
		found = 0;
		for (e = sm->edges; e; e = e->next) {
			if (e->source && e->source == init_n) {
				found = 1;
				if (initial_edge) {
					*initial_edge = e;
				}
			}
		}
		if (!found) {
			ERROR("The SM %s has no edge form the top level initial pseudostate\n",
				  sm->nodes->id);
			return CYBERIADA_FORMAT_ERROR;
		}
	}
		
	return CYBERIADA_NO_ERROR;
}

/*

static int cyberiada_find_nodes_with_the_same_ids(CyberiadaNode* root1, CyberiadaNode* root2, CyberiadaList** nodes_list)
{
	CyberiadaNode *n1;

	if (!root1 || !root2 || !nodes_list) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (n1 = root1; n1; n1 = n1->next) {
		CyberadaNode* n2 = cyberiada_graph_find_node_by_id(root2, n1->id);
		if (n2) {
		cyberiada_list_add(nodes_list, n1, n2);			
		}
		if (n1->children) {
			cyberiada_find_nodes_with_the_same_ids(n1->children, root2, nodes_list);
		}
	}

	return CYBERIADA_NO_ERROR;
	}

	static int cyberiada_compare_nodes(CyberiadaNode* nodes1, CyberiadaNode* nodes2,
	int* result_flags, size_t* sm_diff_nodes_size, CyberiadaNode*** sm_diff_nodes,
	size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
								   size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes)
{
	CyberidaList **origin_list = NULL, **equal_list = ;

	int res = CYBERIADA_NO_ERROR;
	CyberiadaNode *n1, *n2;
	CyberiadaList* equal_pairs = NULL;
	CyberiadaList* isomorphic_candidates = NULL;

	if (!nodes1 || !nodes2 || !result_flags) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (n1 = nodes1; n1; n1 = n1->next) {
		if (ciberiada_list_find_key(&equal_pairs, n1)) {
			continue;
		}
		for (n2 = nodes2; n2; n2 = n2->next) {
			int flags = 0, children_flag = 0;
			if (ciberiada_list_find_data(&equal_pairs, n1)) {
				continue;
			}

			cyberiada_compare_two_nodes(n1, n2, &flags);

			if (flag == CYBERIADA_ISOMORPH_FLAG_DIFF_STATES) {
				continue;
			}
			
			if (nodes1->children && nodes2->children) {
				cyberiada_compare_nodes(nodes1->children, nodes2->children,
										&children_flag, sm_diff_nodes_size, sm_diff_nodes,
										sm2_new_nodes_size, sm2_new_nodes,
										sm1_missing_nodes_size, sm1_missing_nodes);
			}

			if ((flag | children_flag) & CYBERIADA_ISOMORPH_FLAG_EQUAL) {
			cyberiada_list_add(&equal_pairs, n1, n2);
			} else if ((flag | children_flag) & (CYBERIADA_ISOMORPH_FLAG_EQUAL & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC)) {
				if (cyberiada_list_find_key(&isomorphic_candidates, n1) != n2) {
					
				}
			}
	}

	cyberiada_list_free(&equal_pairs);
	}*/

typedef struct {
	CyberiadaNode* node;
	size_t         degree_out;
	size_t         degree_in;
	int            found;
} Vertex;

static int cyberiada_node_degrees(CyberiadaSM* sm, CyberiadaNode* node, size_t* degree_in, size_t* degree_out)
{
	CyberiadaEdge* e;
	size_t d_in = 0, d_out = 0;

	if (!sm || !node || !degree_in || !degree_out) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	for (e = sm->edges; e; e = e->next) {
		if (e->source == node) {
			d_out++;
		}
	}

	for (e = sm->edges; e; e = e->next) {
		if (e->target == node) {
			d_in++;
		}
	}
	
	if (degree_in) {
		*degree_in = d_in;
	}

	if (degree_out) {
		*degree_out = d_out;
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_enumerate_vertexes(CyberiadaSM* sm, CyberiadaNode* nodes, Vertex* v_array, size_t v_size, size_t* cur_index, int ignore_comments)
{
	int res;
	CyberiadaNode* n;
	size_t index = 0;

	if (!nodes || !v_array) {
		return CYBERIADA_BAD_PARAMETER;
	}

 	if (!cur_index) {
		cur_index = &index;
		/*DEBUG("\nVector:\n");*/
	} else if (*cur_index >= v_size) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	for (n = nodes; n; n = n->next) {
		size_t d_in, d_out;
		Vertex* cur_vertex;
		if (ignore_comments && (n->type == cybNodeComment || n->type == cybNodeFormalComment)) continue;
		cur_vertex = v_array + (*cur_index)++;
		cur_vertex->node = n;
		if ((res = cyberiada_node_degrees(sm, n, &d_in, &d_out)) != CYBERIADA_NO_ERROR) {
			return res;
		}
		cur_vertex->degree_in = d_in;
		cur_vertex->degree_out = d_out;
		cur_vertex->found = 0;
		/*DEBUG("%lu: %s +%lu -%lu\n", *cur_index, n->id, d_in, d_out);*/

		if (n->children) {
			if ((res = cyberiada_enumerate_vertexes(sm, n->children, v_array, v_size, cur_index, ignore_comments)) != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

static void debug_matrix(const char* name, char** matrix, size_t n, size_t m)
{
	size_t i, j;
	DEBUG("\n%s matrix:\n   ", name);
	for (j = 0; j < m; j++) {
		DEBUG("%lu ", j + 1);
	}
	DEBUG("\n");
	for (i = 0; i < n; i++) {
		DEBUG(" %lu ", i + 1);
		for (j = 0; j < m; j++) {
			DEBUG("%d ", matrix[i][j]);
		}
		DEBUG("\n");
	}
	DEBUG("\n");
}

static int cyberiada_build_node_permutation_matrix(CyberiadaSM* sm1, CyberiadaSM* sm2,
												   int ignore_comments, char*** perm_matrix,
												   Vertex** vertexes1, Vertex** vertexes2, 
												   size_t* n_vertexes1, size_t* n_edges1, size_t* n_vertexes2, size_t* n_edges2)
{
	size_t i, j, k, x, y, n_v1 = 0, n_v2 = 0, n_e1 = 0, n_e2 = 0;
	char **M = NULL, **P = NULL;
	size_t *row_num = NULL, *col_num = NULL; 
	Vertex *v1 = NULL, *v2 = NULL;
	int found;
	
	if (!sm1 || !sm2 || !sm1->nodes || !sm2->nodes || !sm1->nodes->children || !sm2->nodes->children) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	cyberiada_sm_size(sm1, &n_v1, &n_e1, ignore_comments);
	cyberiada_sm_size(sm2, &n_v2, &n_e2, ignore_comments);

	if (n_v1 == 0 || n_v2 == 0) {
		return CYBERIADA_ASSERT;
	}

	M = (char**)malloc(sizeof(char*) * n_v1);
	for (i = 0; i < n_v1; i++) {
		M[i] = malloc(sizeof(char) * n_v2);
	}
	P = (char**)malloc(sizeof(char*) * n_v1);
	for (i = 0; i < n_v1; i++) {
		P[i] = malloc(sizeof(char) * n_v2);
	}
	v1 = (Vertex*)malloc(sizeof(Vertex) * n_v1);
	v2 = (Vertex*)malloc(sizeof(Vertex) * n_v2);
	row_num = (size_t*)malloc(sizeof(size_t) * sizeof(n_v1));
	col_num = (size_t*)malloc(sizeof(size_t) * sizeof(n_v2));
	memset(row_num, 0, sizeof(size_t) * n_v1);
	memset(col_num, 0, sizeof(size_t) * n_v2);

	cyberiada_enumerate_vertexes(sm1, sm1->nodes->children, v1, n_v1, NULL, ignore_comments);
	cyberiada_enumerate_vertexes(sm2, sm2->nodes->children, v2, n_v2, NULL, ignore_comments);

	for (i = 0; i < n_v1; i++) {
		for (j = 0; j < n_v2; j++) {
			CyberiadaNode* node1 = v1[i].node;
			CyberiadaNode* node2 = v2[j].node;
			if ((node1->type == node2->type ||
				 node1->type == cybNodeSimpleState && node2->type == cybNodeCompositeState ||
				 node1->type == cybNodeCompositeState && node2->type == cybNodeSimpleState) &&
				v1[i].degree_in <= v2[j].degree_in &&
				v1[i].degree_out <= v2[j].degree_out) {
				M[i][j] = 1;
				row_num[i]++;
				col_num[j]++;
			} else {
				M[i][j] = 0;
			}
		}
	}

	/*debug_matrix("M", M, n_v1, n_v2);*/

	found = 0;

	for (i = 0; i < n_v1; i++) {
		if (row_num[i] > 1) {
			found = 1;
			break;
		}
	}
	if (!found) {
		for (j = 0; j < n_v2; j++) {
			if (col_num[j] > 1) {
				found = 1;
				break;
			}
		}
	}

	if (!found) {
		/* potentially isomorphic */	
		for (i = 0; i < n_v1; i++) {
			for (j = 0; j < n_v2; j++) {
				P[i][j] = M[i][j];
			}
		}
	} else {
		/* there are different permutations */

		size_t p_max = 0;
		char** P_max = (char**)malloc(sizeof(char*) * n_v1);
		for (i = 0; i < n_v1; i++) {
			P_max[i] = (char*)malloc(sizeof(char) * n_v2);
			memset(P_max[i], 0, sizeof(char) * n_v2);
			memset(P[i], 0, sizeof(char) * n_v2);
		}

		for (i = 0; i < n_v1 && row_num[i]; i++) {
			for (j = 0; j < n_v2 && col_num[j]; j++) {
				if (M[i][j]) {
					for (k = 0; k < n_v1; k++) {
						memset(P[k], 0, sizeof(char) * n_v2);
					}
					P[i][j] = 1;
					size_t p_total = 1;
					for (x = 0; x < n_v1; x++) {
						for (y = 0; y < n_v2; y++) {
							if (x == i && y == j) continue;
							if (M[x][y]) {
								found = 0;
								for (k = 0; k < n_v1; k++) {
									if (P[k][y]) {
										found = 1;
										break;
									}
								}
								if (found) continue;
								for (k = 0; k < n_v2; k++) {
									if (P[x][k]) {
										found = 1;
										break;
									}
								}
								if (!found) {
									P[x][y] = 1;
									p_total++;
								}
							}
						}
					}
					if (p_total > p_max) {
						/* debug_matrix("P", P, n_v1, n_v2); */
						for (k = 0; k < n_v1; k++) {
							memcpy(P_max[k], P[k], sizeof(char) * n_v2);
						}
						p_max = p_total;
					}
					row_num[i]--;
					col_num[j]--;
				}
			}
		}
		for (k = 0; k < n_v1; k++) {
			memcpy(P[k], P_max[k], sizeof(char) * n_v2);
		}
		for (i = 0; i < n_v1; i++) {
			free(P_max[i]);
		}
		free(P_max);
	}

	for (i = 0; i < n_v1; i++) {
		free(M[i]);
	}
	free(M);
	free(row_num);
	free(col_num);
	if (perm_matrix) {
		*perm_matrix = P;
	} else {
		for (i = 0; i < n_v1; i++) {
			free(P[i]);
		}
		free(P);
	}
	if (vertexes1) {
		*vertexes1 = v1;
	} else {
		free(v1);
	}
	if (vertexes2) {
		*vertexes2 = v2;
	} else {
		free(v2);
	}
	if (n_vertexes1) *n_vertexes1 = n_v1;
	if (n_vertexes2) *n_vertexes2 = n_v1;
	if (n_edges1) *n_edges1 = n_e1;
	if (n_edges2) *n_edges2 = n_e2;

	return CYBERIADA_NO_ERROR;
}

/* return 0 if actions are equal */
static int cyberiada_compare_actions(CyberiadaAction* action1, CyberiadaAction* action2)
{
	CyberiadaAction *a1, *a2;
	CyberiadaList* found_list = NULL;
	size_t a1_n = 0, a2_n = 0;

	if (!action1 && !action2) return 0;
	if (action1 && !action2) return 1;
	if (!action1 && action2) return -1;
	
	for (a1 = action1; a1; a1 = a1->next) {
		a1_n++;
	}
	for (a2 = action2; a2; a2 = a2->next) {
		a2_n++;
	}

	if (a1_n < a2_n) return -1;
	if (a1_n > a2_n) return 1;

	for (a1 = action1; a1; a1 = a1->next) {
		int found = 0;
		for (a2 = action2; a2; a2 = a2->next) {
			if (a1->type == a2->type &&
				strcmp(a1->trigger, a2->trigger) == 0 &&
				strcmp(a1->guard, a2->guard) == 0 &&
				strcmp(a1->behavior, a2->behavior) == 0) {
				found = 1;
				cyberiada_list_add(&found_list, a2->trigger, (void*)a2);
			}
		}
		if (!found) {
			cyberiada_list_free(&found_list);
			return 1;
		}
	}
	
	if (cyberiada_list_size(&found_list) != a2_n) {
		cyberiada_list_free(&found_list);
		return -1;
	}

	cyberiada_list_free(&found_list);		
	return 0;
}

static int cyberiada_compare_two_nodes(CyberiadaNode* n1, CyberiadaNode* n2,
									   size_t n1_degree_in, size_t n1_degree_out, size_t n2_degree_in, size_t n2_degree_out,
									   int* result_flags)
{
	CyberiadaNode* n;
	size_t n1_children = 0, n2_children = 0;

	if (!n1 || !n2 || !result_flags) {
		return CYBERIADA_BAD_PARAMETER;
	}

	*result_flags = 0;

	if (strcmp(n1->id, n2->id) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_ID;
	}

	if (n1->type != n2->type) {
		*result_flags |= CYBERIADA_NODE_DIFF_TYPE;
	}
	
	if (n1->title && n2->title && strcmp(n1->title, n2->title) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_TITLE;		
	}

	if (cyberiada_compare_actions(n1->actions, n2->actions) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_ACTIONS;		
	}

	if (n1->link && n2->link && n1->link->ref && n2->link->ref && strcmp(n1->link->ref, n2->title) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_SM_LINK;
	}
	
	for (n = n1->children; n; n = n->next) {
		n1_children++;
	}
	for (n = n2->children; n; n = n->next) {
		n2_children++;
	}
	if (n1_children != n2_children) {
		*result_flags |= CYBERIADA_NODE_DIFF_CHILDREN;
	}

	if (n1_degree_in != n2_degree_in || n1_degree_out != n2_degree_out) {
		*result_flags |= CYBERIADA_NODE_DIFF_EDGES;		
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2, int ignore_comments, int require_initial,
								int* result_flags, CyberiadaNode** new_initial,
								size_t* sm_diff_nodes_size, CyberiadaNode*** sm_diff_nodes, size_t** sm_diff_nodes_flags,
								size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
								size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes,
								size_t* sm_diff_edges_size, CyberiadaEdge*** sm_diff_edges, size_t** sm_diff_edges_flags,
								size_t* sm2_new_edges_size, CyberiadaEdge*** sm2_new_edges,
								size_t* sm1_missing_edges_size, CyberiadaEdge*** sm1_missing_edges)
{
	int res;
	size_t i, j, sm1_vertexes = 0, sm1_edges = 0, sm2_vertexes = 0, sm2_edges = 0;
	char** perm_matrix = NULL;
	Vertex *vertexes1 = NULL, *vertexes2 = NULL;
	CyberiadaNode *sm1_initial_ps = NULL, *sm2_initial_ps = NULL;
	CyberiadaEdge *sm1_initial_edge = NULL, *sm2_initial_edge = NULL, *e1, *e2;
	CyberiadaList* found_edges = NULL;
	
	if (!sm1 || !sm2 || !result_flags) {
		return CYBERIADA_BAD_PARAMETER;
	}

	/* check if thw both statemachines have initial nodes on the top level */
	res = cyberiada_get_initial_pseudostate(sm1, &sm1_initial_ps, &sm1_initial_edge, require_initial);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}
	res = cyberiada_get_initial_pseudostate(sm2, &sm2_initial_ps, &sm2_initial_edge, require_initial);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}

	/* find possible node isomorphism */
	res = cyberiada_build_node_permutation_matrix(sm1, sm2, ignore_comments,  &perm_matrix, &vertexes1, &vertexes2, 
												  &sm1_vertexes, &sm1_edges, &sm2_vertexes, &sm2_edges);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}

	/*debug_matrix("PERM", perm_matrix, sm1_vertexes, sm2_vertexes);*/
	
	/* allocate memory for results */
	if (sm_diff_nodes_size) {
		*sm_diff_nodes_size = 0;
		if (sm_diff_nodes) {
			*sm_diff_nodes = (CyberiadaNode**)malloc(sizeof(CyberiadaNode*) * sm1_vertexes);
		}
		if (sm_diff_nodes_flags) {
			*sm_diff_nodes_flags = (size_t*)malloc(sizeof(size_t) * sm1_vertexes);
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
	if (sm_diff_edges_size) {
		*sm_diff_edges_size = 0;
		if (sm_diff_edges) {
			*sm_diff_edges = (CyberiadaEdge**)malloc(sizeof(CyberiadaEdge*) * sm1_edges);
		}
		if (sm_diff_edges_flags) {
			*sm_diff_edges_flags = (size_t*)malloc(sizeof(size_t) * sm1_edges);
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

	if (sm1_vertexes == sm2_vertexes && sm1_edges == sm2_edges) {
		*result_flags = CYBERIADA_ISOMORPH_FLAG_IDENTICAL;		
	} else {
		*result_flags = 0;
	}

	for (i = 0; i < sm1_vertexes; i++) {
		for (j = 0; j < sm2_vertexes; j++) {
			if (perm_matrix[i][j]) {
				int node_diff = 0;
				cyberiada_compare_two_nodes(vertexes1[i].node, vertexes2[j].node,
											vertexes1[i].degree_in, vertexes1[i].degree_out,
											vertexes2[j].degree_in, vertexes2[j].degree_out,
											&node_diff);
				if (node_diff) {
					if (sm_diff_nodes_size && sm_diff_nodes && sm_diff_nodes_flags) {
						(*sm_diff_nodes)[*sm_diff_nodes_size] = vertexes1[i].node;
						(*sm_diff_nodes_flags)[*sm_diff_nodes_size] = node_diff;	
						(*sm_diff_nodes_size)++;
					}
					if (*result_flags != CYBERIADA_ISOMORPH_FLAG_DIFF_STATES) {
						if (*result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL) {
							*result_flags = CYBERIADA_ISOMORPH_FLAG_EQUAL;
						}
						if (node_diff != CYBERIADA_NODE_DIFF_ID) {
							*result_flags = CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC;
						}
					}
				}
				vertexes1[i].found = 1;
				vertexes2[j].found = 1;
				break;				
			}
		}
		if (!vertexes1[i].found) {
			*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_STATES;
			if (sm1_missing_nodes && sm1_missing_nodes_size) {
				(*sm1_missing_nodes)[(*sm1_missing_nodes_size)++] = vertexes1[i].node;
			}
		}
	}
	for (j = 0; j < sm2_vertexes; j++) {
		if (!vertexes2[j].found) {
			*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_STATES;
			if (sm2_new_nodes && sm2_new_nodes_size) {
				(*sm2_new_nodes)[(*sm2_new_nodes_size)++] = vertexes2[j].node;
			}
		}
	}
	if (!*result_flags) {
		ERROR("result flag is empty after the node comparison\n");
		return CYBERIADA_ASSERT;
	}

	for (e1 = sm1->edges; e1; e1 = e1->next) {
		int found = 0;
		CyberiadaNode *sm2_source = NULL, *sm2_target = NULL;

		if (ignore_comments && (e1->type == cybEdgeComment)) continue;
		
		for (i = 0; i < sm1_vertexes; i++) {
			if (e1->source == vertexes1[i].node) {
				for (j = 0; j < sm2_vertexes; j++) {
					if (perm_matrix[i][j]) {
						sm2_source = vertexes2[j].node;
						break;
					}
				}
			}
			if (e1->target == vertexes1[i].node) {
				for (j = 0; j < sm2_vertexes; j++) {
					if (perm_matrix[i][j]) {
						sm2_target = vertexes2[j].node;
						break;
					}
				}			
			}
			if (sm2_source && sm2_target) break;
		}
		if (!sm2_source || !sm2_target) {
			if (!(*result_flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES)) {
				*result_flags |= CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES;
				if (*result_flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK) {
					*result_flags &= CYBERIADA_ISOMORPH_FLAG_DIFF_MASK;
				}
			}
			if (sm1_missing_edges && sm1_missing_edges_size) {
				(*sm1_missing_edges)[(*sm1_missing_edges_size)++] = e1;
			}
			if (require_initial && e1 == sm1_initial_edge) {
				*result_flags |= CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL;
				if (new_initial) {
					*new_initial = sm2_initial_edge->target;
				}
			}				
			continue;
		}
		
		for (e2 = sm2->edges; e2; e2 = e2->next) {
			if (ignore_comments && (e2->type == cybEdgeComment)) continue;
			if (e2->source == sm2_source && e2->target == sm2_target &&
				cyberiada_list_find(&found_edges, e2->id) == NULL) {
				int edge_diff = 0;
				if (strcmp(e1->id, e2->id) != 0) {
					edge_diff |= CYBERIADA_EDGE_DIFF_ID;
					if (*result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL) {
						*result_flags = CYBERIADA_ISOMORPH_FLAG_EQUAL;
					}
				}
				if (cyberiada_compare_actions(e1->action, e2->action) != 0) {
					edge_diff |= CYBERIADA_EDGE_DIFF_ACTION;
					if (*result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL || *result_flags == CYBERIADA_ISOMORPH_FLAG_EQUAL) {
						*result_flags = CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC;
					}
				}
				if (edge_diff && sm_diff_edges && sm_diff_edges_flags && sm_diff_edges_size) {
					(*sm_diff_edges)[*sm_diff_edges_size] = e1;
					(*sm_diff_edges_flags)[*sm_diff_edges_size] = edge_diff;	
					(*sm_diff_edges_size)++;
				}
				cyberiada_list_add(&found_edges, e2->id, (void*)e2); 				
				break;
			}
		}
	}
	if (cyberiada_list_size(&found_edges) < sm2_edges) {
		for (e2 = sm2->edges; e2; e2 = e2->next) {
			if (ignore_comments && e2->type == cybEdgeComment) continue;
			if (cyberiada_list_find(&found_edges, e2->id) == NULL) {
				if (!(*result_flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES)) {
					*result_flags |= CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES;
					if (*result_flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK) {
						*result_flags &= CYBERIADA_ISOMORPH_FLAG_DIFF_MASK;
					}
				}
				if (sm2_new_edges && sm2_new_edges_size) {
					(*sm2_new_edges)[(*sm2_new_edges_size)++] = e2;
				}
				if (require_initial && e2 == sm2_initial_edge) {
					*result_flags |= CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL;
					if (new_initial) {
						*new_initial = sm2_initial_edge->target;
					}
				}
			}
		}
	}

	cyberiada_list_free(&found_edges);
	if (perm_matrix) {
		for (i = 0; i < sm1_vertexes; i++) {
			free(perm_matrix[i]);
		}
		free(perm_matrix);
		free(vertexes1);
		free(vertexes2);
	}
	
	return CYBERIADA_NO_ERROR;	
}

