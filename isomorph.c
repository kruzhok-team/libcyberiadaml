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
#include "cyb_structs.h"
#include "cyb_error.h"

#ifdef __DEBUG__
/* Uncomment this if you need additional debug */
/*#define EXTRA_DEBUG 1 */
#endif

#define MAX_PROXIMITY 127

/*-----------------------------------------------------------------------------
 Recursively calculate number of nodes in a node tree 
 ------------------------------------------------------------------------------*/

static int cyberiada_node_size_recursively(CyberiadaNode* node, size_t* v, int ignore_comments, int ignore_regions)
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
		if (!ignore_regions || n->type != cybNodeRegion) {
			cnt++;
		}
		if (n->children) {
			size_t ch_cnt = 0;
			cyberiada_node_size_recursively(n->children, &ch_cnt, ignore_comments, ignore_regions);
			cnt += ch_cnt;
		}
	}

	if (v) *v = cnt;

	return CYBERIADA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 Calculate number of nodes and edges in a SM 
 ------------------------------------------------------------------------------*/

int cyberiada_sm_size(CyberiadaSM* sm, size_t* v, size_t* e, int ignore_comments, int ignore_regions)
{
	size_t n_cnt = 0, e_cnt = 0;
	CyberiadaEdge* edge;
	
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}

	if (sm->nodes) {
		size_t ch_cnt = 0;
		cyberiada_node_size_recursively(sm->nodes->children, &ch_cnt, ignore_comments, ignore_regions);
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

/*-----------------------------------------------------------------------------
 Find the initial pseudostate of a SM 
 ------------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------
 The representation of a SM node in the isomorphism check algorithm
 ------------------------------------------------------------------------------*/

typedef struct {
	CyberiadaNode* node;       /* the node itself */
	int            degree_out; /* the node outdegree */
	int            degree_in;  /* the node indegree */
	int            found;      /* the found flag - if the node was found in the comparing graph */
} Vertex;

/*-----------------------------------------------------------------------------
 Calculate degrees of SM graph nodes 
 ------------------------------------------------------------------------------*/

static int cyberiada_node_degrees(CyberiadaSM* sm, CyberiadaNode* node, int* degree_in, int* degree_out)
{
	CyberiadaEdge* e;
	int d_in = 0, d_out = 0;

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

/*-----------------------------------------------------------------------------
 Fill the node array by SM node structure  
 ------------------------------------------------------------------------------*/

static int cyberiada_enumerate_vertexes(CyberiadaSM* sm, CyberiadaNode* nodes, Vertex* v_array, size_t v_size, size_t* cur_index,
										int ignore_comments, int ignore_regions)
{
	int res;
	CyberiadaNode* n;
	size_t index = 0;

	if (!nodes || !v_array) {
		return CYBERIADA_BAD_PARAMETER;
	}

 	if (!cur_index) {
		cur_index = &index;
#ifdef EXTRA_DEBUG
		DEBUG("\nVector:\n");
#endif
	} else if (*cur_index >= v_size) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (n = nodes; n; n = n->next) {
		int d_in, d_out;
		Vertex* cur_vertex;

		/* check if we consider comment nodes during isomorphism check */
		if (ignore_comments && (n->type == cybNodeComment || n->type == cybNodeFormalComment)) continue;

		/* check if we consider region nodes during isomorphism check */
		if (!ignore_regions || n->type != cybNodeRegion) {
			cur_vertex = v_array + (*cur_index)++;
			cur_vertex->node = n;
			if ((res = cyberiada_node_degrees(sm, n, &d_in, &d_out)) != CYBERIADA_NO_ERROR) {
				return res;
			}
			cur_vertex->degree_in = d_in;
			cur_vertex->degree_out = d_out;
			cur_vertex->found = 0;
		}

#ifdef EXTRA_DEBUG
		DEBUG("%lu: %s +%lu -%lu\n", *cur_index, n->id, d_in, d_out);
#endif

		if (n->children) {
			if ((res = cyberiada_enumerate_vertexes(sm, n->children, v_array, v_size, cur_index,
													ignore_comments, ignore_regions)) != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 Print a matrix of small numbers  
 ------------------------------------------------------------------------------*/

static void debug_matrix(const char* name, char** matrix, size_t n, size_t m)
{
	size_t i, j;
	DEBUG("\n%s matrix:\n     ", name);
	for (j = 0; j < m; j++) {
		DEBUG("%3lu ", j + 1);
	}
	DEBUG("\n");
	for (i = 0; i < n; i++) {
		DEBUG(" %3lu ", i + 1);
		for (j = 0; j < m; j++) {
			DEBUG("%3d ", matrix[i][j]);
		}
		DEBUG("\n");
	}
	DEBUG("\n");
}

/*-----------------------------------------------------------------------------
 Calculate the matrix of node proximity of two SMs (the first pass) basing on
 the node difference
 ------------------------------------------------------------------------------*/

static int calculate_sm_proximity_matrix_nodes(char** M, char** ProxiM, Vertex* v1, Vertex* v2, size_t n1, size_t n2)
{
	int result = 0;
	size_t i, j;
	for (i = 0; i < n1; i++) {
		for (j = 0; j < n2; j++) {
			if (M[i][j]) {
				if (ProxiM[i][j] < 0) {
					int proximity = 0;
					int res;
					CyberiadaNode* node1 = v1[i].node;
					CyberiadaNode* node2 = v2[j].node;
					int flags = 0;

					/* if the nodes have similar or exactly the same type */ 
					if (node1->type == node2->type ||
						(node1->type == cybNodeSimpleState && node2->type == cybNodeCompositeState) ||
						(node1->type == cybNodeCompositeState && node2->type == cybNodeSimpleState) ||
						(node1->type == cybNodeComment && node2->type == cybNodeFormalComment) ||
						(node1->type == cybNodeFormalComment && node2->type == cybNodeComment)) {
						proximity += 5;
						if (node1->type == node2->type) {
							proximity += 5;
						}
					}
					/* if the nodes have the equal ids */
					if (node1->id && node2->id && strcmp(node1->id, node2->id) == 0) {
						proximity += 10;
					}
					/* if the nodes have the equal titles */
					if (node1->title && node2->title && strcmp(node1->title, node2->title) == 0) {
						proximity += 10;
					}
					
					if ((res = cyberiada_compare_node_actions(node1->actions, node2->actions, &flags)) != CYBERIADA_NO_ERROR) {
						ERROR("Error while comparing node %s and %s actions: %d\n", node1->id, node2->id, res);
						return 0;
					}
					/* if the nodes have the equal number of actions */					
					if (!(flags & CYBERIADA_ACTION_DIFF_NUMBER)) {
						proximity += 5;
					}
					/* if the nodes have the equal types of actions */					
					if (!(flags & CYBERIADA_ACTION_DIFF_TYPES)) {
						proximity += 5;
					}
					if (!(flags & (CYBERIADA_ACTION_DIFF_NUMBER |
								   CYBERIADA_ACTION_DIFF_TYPES))) {
						if (flags & CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION) {
							/* pass */
						} else if (flags & CYBERIADA_ACTION_DIFF_BEHAVIOR_ORDER) {
							/* if the nodes have the same order of behavior strings */
							proximity += 30;
						} else if (flags & CYBERIADA_ACTION_DIFF_BEHAVIOR_ARG) {
							/* if the nodes have the same arguments of behavior strings */
							proximity += 15;
						} else {
							/* if the nodes have exactly the same behaviors */
							proximity += 50;
						}
					}

					/* if the nodes have same degrees */
					if (v1[i].degree_in == v2[j].degree_in || v1[i].degree_out == v2[j].degree_out) {
						proximity += 10;
						if (v1[i].degree_in == v2[j].degree_in && v1[i].degree_out == v2[j].degree_out) {
							proximity += 20;
						}
					}
#ifdef EXTRA_DEBUG
					DEBUG("Comparing nodes %s [%s] and %s [%s] %d -> %d\n",
					  node1->id, node1->title, node2->id, node2->title, flags, proximity);
#endif
					if (proximity > MAX_PROXIMITY) {
						ProxiM[i][j] = MAX_PROXIMITY;
					} else {
						ProxiM[i][j] = (char)proximity;
					}
					result += proximity;
				} else {
					
				}
			}
		}
	}

	return result;
}

/*-----------------------------------------------------------------------------
 Calculate the matrix of node proximity of two SMs (the second pass) basing on
 the edges difference
 ------------------------------------------------------------------------------*/

static int calculate_sm_proximity_matrix_edges(char** M, char** ProxiM,
											   CyberiadaSM* sm1, CyberiadaSM* sm2,
											   Vertex* v1, Vertex* v2, size_t n1, size_t n2)
{
	size_t i, j, k;
	int** EP;

	EP = (int**)malloc(sizeof(int*) * n1);
	for (i = 0; i < n1; i++) {
		EP[i] = (int*)malloc(sizeof(int) * n2);
		memset(EP[i], 0, sizeof(int) * n2);
	}
	
	for (i = 0; i < n1; i++) {
		for (j = 0; j < n2; j++) {
			if (M[i][j] && ProxiM[i][j] > 0) {
				CyberiadaNode* node1 = v1[i].node;
				CyberiadaNode* node2 = v2[j].node;
				CyberiadaEdge *e1, *e2;
				int edge_proxy = 0;
				if (v1[i].degree_in == v2[j].degree_in && v1[i].degree_out == v2[j].degree_out) {
					for (e1 = sm1->edges; e1; e1 = e1->next) {
						for (e2 = sm2->edges; e2; e2 = e2->next) {
							if (e1->source == node1 && e2->source == node2) {
								size_t n = n1, m = n2;
								for (k = 0; k < n1; k++) {
									if (v1[k].node == e1->target) {
										n = k;
										break;
									}
								}
								for (k = 0; k < n2; k++) {
									if (v2[k].node == e2->target) {
										m = k;
										break;
									}
								}
								if (n == n1 || m == n2) {
									ERROR("Error while reconstruction edge source proximity\n");
									for (i = 0; i < n1; i++) {
										free(EP[i]);
									}
									free(EP);
									return CYBERIADA_BAD_PARAMETER;
								}
								if (ProxiM[n][m] > 0) {
									edge_proxy += 1;
									if (ProxiM[n][m] >= ProxiM[i][j]) {
										edge_proxy += 2;
									}
								}
								break;
							}
							if (e1->target == node1 && e2->target == node2) {
								size_t n = n1, m = n2;
								for (k = 0; k < n1; k++) {
									if (v1[k].node == e1->source) {
										n = k;
										break;
									}
								}
								for (k = 0; k < n2; k++) {
									if (v2[k].node == e2->source) {
										m = k;
										break;
									}
								}
								if (n == n1 || m == n2) {
									ERROR("Error while reconstruction edge target proximity\n");
									for (i = 0; i < n1; i++) {
										free(EP[i]);
									}
									free(EP);
									return CYBERIADA_BAD_PARAMETER;
								}
								if (ProxiM[n][m] > 0) {
									edge_proxy += 1;
									if (ProxiM[n][m] >= ProxiM[i][j]) {
										edge_proxy += 2;
									}
								}
								break;
							}
						}
					}
					if (edge_proxy > 0) {
						EP[i][j] += edge_proxy;
					}
				}
			}
		}
	}

	for (i = 0; i < n1; i++) {
		for (j = 0; j < n2; j++) {
			if (EP[i][j]) {
				if (ProxiM[i][j] + EP[i][j] > MAX_PROXIMITY) {
					ProxiM[i][j] = MAX_PROXIMITY;
				} else {
					ProxiM[i][j] += (char)EP[i][j];
				}
			}
		}
	}
	
	for (i = 0; i < n1; i++) {
		free(EP[i]);
	}
	free(EP);
	
	return CYBERIADA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 Calculate the total proximity number of a permutation matrix basing on
 the proximity matrix
 ------------------------------------------------------------------------------*/

static int calculate_sm_proximity(char** P, char** ProxiM, size_t n1, size_t n2)
{
	int result = 0;
	size_t i, j;
	for (i = 0; i < n1; i++) {
		for (j = 0; j < n2; j++) {
			if (P[i][j] && ProxiM[i][j] > 0) {
				result += ProxiM[i][j];
			}
		}
	}
	return result;
}

/*-----------------------------------------------------------------------------
 Comparison function for quick sort algorithm
 ------------------------------------------------------------------------------*/

static int compare_char(const void* a, const void* b) {
	char ca = *(char*)a;
	char cb = *(char*)b;
    return (cb > ca) - (cb < ca); /* reverse compare */
}

/*-----------------------------------------------------------------------------
 Find the most suitable permutation matrix of two SMs
 ------------------------------------------------------------------------------*/

static int cyberiada_build_node_permutation_matrix(CyberiadaSM* sm1, CyberiadaSM* sm2,
												   int ignore_comments, char*** perm_matrix,
												   Vertex** vertexes1, Vertex** vertexes2, 
												   size_t* n_vertexes1, size_t* n_edges1, size_t* n_vertexes2, size_t* n_edges2)
{
	(void)&debug_matrix; /* unused */

	size_t i, j, k, x, y, n_v1 = 0, n_v2 = 0, n_e1 = 0, n_e2 = 0;
	char **M = NULL, **P = NULL, **Proxi = NULL;
	size_t *row_num = NULL, *col_num = NULL; 
	Vertex *v1 = NULL, *v2 = NULL;
	int found;
	
	if (!sm1 || !sm2 || !sm1->nodes || !sm2->nodes || !sm1->nodes->children || !sm2->nodes->children) {
		return CYBERIADA_BAD_PARAMETER;
	}

	/* calculate the SMs sizes to allocate memory */
	cyberiada_sm_size(sm1, &n_v1, &n_e1, ignore_comments, 1);
	cyberiada_sm_size(sm2, &n_v2, &n_e2, ignore_comments, 1);

	/* matrix required */
	if (n_v1 == 0 || n_v2 == 0) {
		return CYBERIADA_BAD_PARAMETER;
	}

	/* the potential matrix */
	M = (char**)malloc(sizeof(char*) * n_v1);
	for (i = 0; i < n_v1; i++) {
		M[i] = malloc(sizeof(char) * n_v2);
	}
	/* the permutation matrix */
	P = (char**)malloc(sizeof(char*) * n_v1);
	for (i = 0; i < n_v1; i++) {
		P[i] = malloc(sizeof(char) * n_v2);
	}
	/* the proximity matrix */	
	Proxi = (char**)malloc(sizeof(char*) * n_v1);
	for (i = 0; i < n_v1; i++) {
		Proxi[i] = (char*)malloc(sizeof(char) * n_v2);
		for (j = 0; j < n_v2; j++) {
			Proxi[i][j] = -1; /* initializing with -1 which means unset proximity */
		}
	}
	v1 = (Vertex*)malloc(sizeof(Vertex) * n_v1);
	v2 = (Vertex*)malloc(sizeof(Vertex) * n_v2);
	row_num = (size_t*)malloc(sizeof(size_t) * n_v1);
	col_num = (size_t*)malloc(sizeof(size_t) * n_v2);
	memset(row_num, 0, sizeof(size_t) * n_v1);
	memset(col_num, 0, sizeof(size_t) * n_v2);

	cyberiada_enumerate_vertexes(sm1, sm1->nodes->children, v1, n_v1, NULL, ignore_comments, 1);
	cyberiada_enumerate_vertexes(sm2, sm2->nodes->children, v2, n_v2, NULL, ignore_comments, 1);

#ifdef EXTRA_DEBUG
	DEBUG("\nSM1:\n");
	for (i = 0; i < n_v1; i++) {
		DEBUG("\t%ld - %s [%s]\n", i + 1, v1[i].node->id, v1[i].node->title);
	}
	DEBUG("\nSM2:\n");
	for (i = 0; i < n_v2; i++) {
		DEBUG("\t%ld - %s [%s]\n", i + 1, v2[i].node->id, v2[i].node->title);
	}
#endif

	/* fill the potential matrix */
	for (i = 0; i < n_v1; i++) {
		for (j = 0; j < n_v2; j++) {
			CyberiadaNode* node1 = v1[i].node;
			CyberiadaNode* node2 = v2[j].node;
			if ((node1->type == node2->type ||
				 (node1->type == cybNodeSimpleState && node2->type == cybNodeCompositeState) ||
				 (node1->type == cybNodeCompositeState && node2->type == cybNodeSimpleState)) &&
				(abs(v1[i].degree_in - v2[j].degree_in) <= 1 || v1[i].degree_in + 1 < v2[j].degree_in) &&
				(abs(v1[i].degree_out - v2[j].degree_out) <= 1 || v1[i].degree_out + 1 < v2[j].degree_out)) {
				M[i][j] = 1;
				row_num[i]++;
				col_num[j]++;
			} else {
				M[i][j] = 0;
			}
		}
	}

#ifdef EXTRA_DEBUG
	debug_matrix("M", M, n_v1, n_v2);
#endif
	
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
		/* the trivial case:
           the potential matrix have the properties of the permutation matrix, 
		   so the SMs are potentially isomorphic */	
		for (i = 0; i < n_v1; i++) {
			for (j = 0; j < n_v2; j++) {
				P[i][j] = M[i][j];
			}
		}
	} else {
		/* the different permutations of the potential matrix are possible */
		size_t p_max = 0; /* the maximum size of permutation matrices */
		int proximity_max = -1; /* the maximum total proximity found */
		char** P_max = (char**)malloc(sizeof(char*) * n_v1); /* the maximum permutation matrix found */
		char single_proximity_max = -1; /* the maximum single value of the proximity matrix */
		char* proximity_levels; /* the ordered array of unique values of the proximity matrix */
		size_t proximity_levels_size = 0; /* the number of unique values of the proximity matrix */ 

		for (i = 0; i < n_v1; i++) {
			P_max[i] = (char*)malloc(sizeof(char) * n_v2);
			memset(P_max[i], 0, sizeof(char) * n_v2);
			memset(P[i], 0, sizeof(char) * n_v2);
		}

		/* calculate the proximity matrix in two steps */
		calculate_sm_proximity_matrix_nodes(M, Proxi, v1, v2, n_v1, n_v2);
		calculate_sm_proximity_matrix_edges(M, Proxi, sm1, sm2, v1, v2, n_v1, n_v2);

		/* find the maximum single value of the proximity matrix */
		for (i = 0; i < n_v1; i++) {
			for (j = 0; j < n_v2; j++) {
				if (Proxi[i][j] > single_proximity_max) {
					single_proximity_max = Proxi[i][j];
				}
			}
		}

		/* calculate the proximity levels array */
		if (single_proximity_max > 0) {
			proximity_levels = (char*)malloc(single_proximity_max);
			memset(proximity_levels, 0, single_proximity_max);
			proximity_levels[0] = single_proximity_max;
			proximity_levels_size = 1;
			for (i = 0; i < n_v1; i++) {
				for (j = 0; j < n_v2; j++) {
					char p = Proxi[i][j];
					int found = 0;
					if (p < 0) {
						continue;
					}
					for (k = 0; k < proximity_levels_size; k++) {
						if (p == proximity_levels[k]) {
							found = 1;
							break;
						}
					}
					if (!found) {
						proximity_levels[proximity_levels_size] = p;
						proximity_levels_size++;
					}
				}
			}
			qsort(proximity_levels, proximity_levels_size, sizeof(char), compare_char);
#ifdef EXTRA_DEBUG
			DEBUG("\nProximity levels: ");
			for (i = 0; i < proximity_levels_size; i++) {
				DEBUG("%d ", proximity_levels[i]);
			}
			DEBUG("\n");
#endif
		}

		/* go through the potential matrix and try to build permutation matrices and compare
		   them based on the matrices size and the total proximity value */
		for (i = 0; i < n_v1 && p_max <= n_v1; i++) {
			for (j = 0; j < n_v2 && p_max <= n_v2; j++) {
				if (M[i][j]) {
					/* try to build new permutation matrix */
					for (k = 0; k < n_v1; k++) {
						memset(P[k], 0, sizeof(char) * n_v2);
					}
					P[i][j] = 1; /* now we'll start from (i, j) set */
					size_t p_total = 1;
					size_t c_proximity = 0;

					/* try to build permutation matrix using the maximum possible level of proximity */
					while (p_total < n_v1 && p_total < n_v2 && c_proximity < proximity_levels_size) {
						int p_found = 0;
						char current_proximity = proximity_levels[c_proximity];
						for (x = 0; x < n_v1; x++) {
							for (y = 0; y < n_v2; y++) {
								if (x == i && y == j) continue;
								if (Proxi[x][y] == current_proximity) {
									found = 0;
									for (k = 0; k < n_v1; k++) {
										if (P[k][y]) {
											found = 1; /* non uniquie 1 in a row */
											break;
										}
									}
									if (found) continue;
									for (k = 0; k < n_v2; k++) {
										if (P[x][k]) {
											found = 1; /* non uniquie 1 in a column */
											break;
										}
									}
									/* each row & column have only one 1 */
									if (!found) {
										/* new element of the permutation matrix found */
										P[x][y] = 1;
										p_total++;
										p_found = 1;
										c_proximity = 0; /* again - start from the maximum possible level of proximity */
										continue;
									}
								}
							}
						}
						if (!p_found) {
							/* cannot find any permutation matrix element on this level of proximity:
							   reduce the level and take the next level of proximity */
							c_proximity++;
						}
					}
					if (p_total > p_max) {
						/* the permutation matrix of bigger size was found */
#ifdef EXTRA_DEBUG
						debug_matrix("P", P, n_v1, n_v2);
#endif
						/* save the maximums */
						p_max = p_total;
						proximity_max = calculate_sm_proximity(P, Proxi, n_v1, n_v2);
						for (k = 0; k < n_v1; k++) {
							memcpy(P_max[k], P[k], sizeof(char) * n_v2);
						}
					} else if (p_total == p_max) {
						/* the permutation matrix of the same size was found, 
						   we need to calculate the total proximity */
						int proximity = calculate_sm_proximity(P, Proxi, n_v1, n_v2);
						if (proximity > proximity_max) {
							/* the permutation matrix with bigger total proximity was found */
#ifdef EXTRA_DEBUG
							debug_matrix("P", P, n_v1, n_v2);
#endif
							/* save the maximums */
							proximity_max = proximity;
							for (k = 0; k < n_v1; k++) {
								memcpy(P_max[k], P[k], sizeof(char) * n_v2);
							}							
						}
					}
				}
			}
		}
		/* save the found permutation matrix to P */
		for (k = 0; k < n_v1; k++) {
			memcpy(P[k], P_max[k], sizeof(char) * n_v2);
		}

		/* free the rest */
		for (i = 0; i < n_v1; i++) {
			free(P_max[i]);
		}
		free(P_max);

		if (proximity_levels) {
			free(proximity_levels);	
		}
	}

#ifdef EXTRA_DEBUG
	debug_matrix("Proximity M", Proxi, n_v1, n_v2);
#endif
	
	for (i = 0; i < n_v1; i++) {
		free(M[i]);
		free(Proxi[i]);
	}
	free(M);
	free(Proxi);
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
	if (n_vertexes2) *n_vertexes2 = n_v2;
	if (n_edges1) *n_edges1 = n_e1;
	if (n_edges2) *n_edges2 = n_e2;

	return CYBERIADA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 Compare two SM actions: return 0 if actions are equal
 ------------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------
 Compare two SM nodes
 ------------------------------------------------------------------------------*/

static int cyberiada_compare_two_nodes(CyberiadaNode* n1, CyberiadaNode* n2,
									   int n1_degree_in, int n1_degree_out, int n2_degree_in, int n2_degree_out,
									   int* result_flags)
{
	CyberiadaNode* n;
	size_t n1_children = 0, n2_children = 0;

	if (!n1 || !n2 || !result_flags) {
		return CYBERIADA_BAD_PARAMETER;
	}

	*result_flags = 0;

	/* the two nodes have different ids */
	if (strcmp(n1->id, n2->id) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_ID;
	}

	/* the two nodes have different types */
	if (n1->type != n2->type) {
		*result_flags |= CYBERIADA_NODE_DIFF_TYPE;
	}

	/* the two nodes have different titles */
	if (n1->title && n2->title && strcmp(n1->title, n2->title) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_TITLE;		
	}

	/* the two nodes have different actions */
	if (cyberiada_compare_actions(n1->actions, n2->actions) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_ACTIONS;		
	}

	/* the two nodes have different SM links */
	if (n1->link && n2->link && n1->link->ref && n2->link->ref && strcmp(n1->link->ref, n2->title) != 0) {
		*result_flags |= CYBERIADA_NODE_DIFF_SM_LINK;
	}
	
	for (n = n1->children; n; n = n->next) {
		n1_children++;
	}
	for (n = n2->children; n; n = n->next) {
		n2_children++;
	}
	/* the two nodes have different children */
	if (n1_children != n2_children) {
		*result_flags |= CYBERIADA_NODE_DIFF_CHILDREN;
	}

	/* the two nodes have different degrees */
	if (n1_degree_in != n2_degree_in || n1_degree_out != n2_degree_out) {
		*result_flags |= CYBERIADA_NODE_DIFF_EDGES;		
	}

	return CYBERIADA_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 Check isomophism of two SM graphs and return the difference
 ------------------------------------------------------------------------------*/

int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2, int ignore_comments, int require_initial,
								int* result_flags, CyberiadaNode** new_initial,
								size_t* sm_diff_nodes_size, CyberiadaNodePair** sm_diff_nodes, size_t** sm_diff_nodes_flags,
								size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
								size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes,
								size_t* sm_diff_edges_size, CyberiadaEdgePair** sm_diff_edges, size_t** sm_diff_edges_flags,
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

	/* find the permutation matrix of possible node isomorphism */
	res = cyberiada_build_node_permutation_matrix(sm1, sm2, ignore_comments,  &perm_matrix, &vertexes1, &vertexes2, 
												  &sm1_vertexes, &sm1_edges, &sm2_vertexes, &sm2_edges);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}

#ifdef EXTRA_DEBUG
	debug_matrix("PERM", perm_matrix, sm1_vertexes, sm2_vertexes);
#endif
	
	/* allocate memory for results */
	if (sm_diff_nodes_size) {
		*sm_diff_nodes_size = 0;
		if (sm_diff_nodes) {
			*sm_diff_nodes = (CyberiadaNodePair*)malloc(sizeof(CyberiadaNodePair) * sm1_vertexes);
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
			*sm_diff_edges = (CyberiadaEdgePair*)malloc(sizeof(CyberiadaEdgePair) * sm1_edges);
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
		/* the SM graphs have the equal size, potentially identical */
		*result_flags = CYBERIADA_ISOMORPH_FLAG_IDENTICAL;		
	} else {
		/* the SM graphs have the different sizes - non isomorphic */
		*result_flags = 0;
		if (sm1_vertexes != sm2_vertexes) {
			*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_STATES;
		}
		if (sm1_edges != sm2_edges) {
			*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES;
		}
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
					/* there are differences in two nodes */
					if (sm_diff_nodes_size && sm_diff_nodes && sm_diff_nodes_flags) {
						(*sm_diff_nodes)[*sm_diff_nodes_size].n1 = vertexes1[i].node;
						(*sm_diff_nodes)[*sm_diff_nodes_size].n2 = vertexes2[j].node;
						(*sm_diff_nodes_flags)[*sm_diff_nodes_size] = node_diff;	
						(*sm_diff_nodes_size)++;
					}
					if (*result_flags != CYBERIADA_ISOMORPH_FLAG_DIFF_STATES) {
						if (node_diff == CYBERIADA_NODE_DIFF_ID) {
							/* if the only difference is node id - potentially equal */
							if (*result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL) {
								*result_flags = CYBERIADA_ISOMORPH_FLAG_EQUAL;
							}
						} else if (node_diff & (CYBERIADA_NODE_DIFF_CHILDREN | CYBERIADA_NODE_DIFF_TYPE)) {
							*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_STATES;
						} else {
							/* the types and children are the same - potentially isomorphic */
							if (*result_flags & (CYBERIADA_ISOMORPH_FLAG_IDENTICAL | CYBERIADA_ISOMORPH_FLAG_EQUAL)) {
								*result_flags = CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC;
							}
						}
					}
				}
				vertexes1[i].found = 1;
				vertexes2[j].found = 1;
				break;				
			}
		}
		if (!vertexes1[i].found) {
			/* missing SM1 nodes in SM2 found */
			*result_flags = CYBERIADA_ISOMORPH_FLAG_DIFF_STATES;
			if (sm1_missing_nodes && sm1_missing_nodes_size) {
				(*sm1_missing_nodes)[(*sm1_missing_nodes_size)++] = vertexes1[i].node;
			}
		}
	}
	for (j = 0; j < sm2_vertexes; j++) {
		if (!vertexes2[j].found) {
			/* new SM2 nodes found */
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
		CyberiadaNode *sm2_source = NULL, *sm2_target = NULL;
		int found = 0;

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
			/* the edge found not available in the second graph */
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
				found = 1;
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
				/* the edges with differences are found */
				if (edge_diff && sm_diff_edges && sm_diff_edges_flags && sm_diff_edges_size) {
					(*sm_diff_edges)[*sm_diff_edges_size].e1 = e1;
					(*sm_diff_edges)[*sm_diff_edges_size].e2 = e2;
					(*sm_diff_edges_flags)[*sm_diff_edges_size] = edge_diff;	
					(*sm_diff_edges_size)++;
				}
				cyberiada_list_add(&found_edges, e2->id, (void*)e2); 				
				break;
			}
		}
		if (!found) {
			/* the edge found not available in the second graph */
			if (sm1_missing_edges && sm1_missing_edges_size) {
				(*sm1_missing_edges)[(*sm1_missing_edges_size)++] = e1;
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
				/* the edge found not available in the first graph */
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

