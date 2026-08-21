/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The isomorphism check and diff C library header
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

#ifndef __CYBERIADA_ML_ISO_H
#define __CYBERIADA_ML_ISO_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada isomorphism check types
 * ----------------------------------------------------------------------------- */

typedef struct {                                        /* the pair of nodes */
	CyberiadaNode*              n1;
	CyberiadaNode*              n2;
} CyberiadaNodePair;

typedef struct {                                        /* the pair of edges */
	CyberiadaEdge*               e1;
	CyberiadaEdge*               e2;
} CyberiadaEdgePair;

/* -----------------------------------------------------------------------------
 * The Cyberiada isomorphism check codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_ISOMORPH_FLAG_IDENTICAL                 0x1    /* the two SM graphs are identical (even ids are the same) */
#define CYBERIADA_ISOMORPH_FLAG_EQUAL                     0x2    /* the two SM graphs are equal */
#define CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC                0x4    /* the two SM graphs are isomorphic (but there are differences) */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_STATES               0x8    /* the two SM graphs are not isomorhic and have different states */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL              0x10    /* the two SM graphs are not isomorhic and have different initial pseudostates */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES                0x20   /* the two SM graphs are not isomorhic and have different edges */
#define CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK           (CYBERIADA_ISOMORPH_FLAG_IDENTICAL | \
														   CYBERIADA_ISOMORPH_FLAG_EQUAL | \
														   CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC)
#define CYBERIADA_ISOMORPH_FLAG_DIFF_MASK                 (CYBERIADA_ISOMORPH_FLAG_DIFF_STATES | \
														   CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL | \
														   CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES)

#define CYBERIADA_NODE_DIFF_ID                            0x1    /* the two SM nodes have different identifiers */
#define CYBERIADA_NODE_DIFF_TYPE                          0x2    /* the two SM nodes have different types (excluding simple/comp. state) */
#define CYBERIADA_NODE_DIFF_TITLE                         0x4    /* the two SM nodes have different titles */
#define CYBERIADA_NODE_DIFF_ACTIONS                       0x8    /* the two SM nodes have different actions */
#define CYBERIADA_NODE_DIFF_SM_LINK                       0x10   /* the two SM nodes have different links to a state machine */
#define CYBERIADA_NODE_DIFF_CHILDREN                      0x20   /* the two SM nodes have different number of children */
#define CYBERIADA_NODE_DIFF_EDGES                         0x40   /* the two SM nodes have different incoming/outgoing edges */

#define CYBERIADA_EDGE_DIFF_ID                            0x80   /* the two SM edges have different identifiers */
#define CYBERIADA_EDGE_DIFF_ACTION                        0x100  /* the two SM edges have different actions */

#define CYBERIADA_ACTION_DIFF_BEHAVIOR_ARG                0x1    /* the two nodes' behaviors have different arguments */
#define CYBERIADA_ACTION_DIFF_BEHAVIOR_ORDER              0x2    /* the two nodes' behaviors have different order */
#define CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION             0x4    /* the two nodes' behaviors are differ */
#define CYBERIADA_ACTION_DIFF_TYPES                       0x10   /* the two nodes' have different action types */
#define CYBERIADA_ACTION_DIFF_GUARDS                      0x20   /* the two nodes' have different guards */
#define CYBERIADA_ACTION_DIFF_NUMBER                      0x40   /* the two nodes' have different action numbers */
#define CYBERIADA_ACTION_DIFF_PROPAGATION                 0x80   /* the two nodes' have different event handling parameters */

/* The isomorphism check result. The arrays are allocated by the check and
   released by cyberiada_cleanup_isomorphism_result(); the nodes and edges
   they point to belong to the compared state machines. */
typedef struct {
	int                 flags;               /* CYBERIADA_ISOMORPH_FLAG_* */
	CyberiadaNode*      new_initial;         /* the different initial pseudostate of the second SM */
	size_t              diff_nodes_size;
	CyberiadaNodePair*  diff_nodes;          /* the matched node pairs with differences */
	size_t*             diff_nodes_flags;    /* CYBERIADA_NODE_DIFF_* per pair */
	size_t              new_nodes_size;
	CyberiadaNode**     new_nodes;           /* the nodes of the second SM missing in the first */
	size_t              missing_nodes_size;
	CyberiadaNode**     missing_nodes;       /* the nodes of the first SM missing in the second */
	size_t              diff_edges_size;
	CyberiadaEdgePair*  diff_edges;          /* the matched edge pairs with differences */
	size_t*             diff_edges_flags;    /* CYBERIADA_EDGE_DIFF_* per pair */
	size_t              new_edges_size;
	CyberiadaEdge**     new_edges;           /* the edges of the second SM missing in the first */
	size_t              missing_edges_size;
	CyberiadaEdge**     missing_edges;       /* the edges of the first SM missing in the second */
} CyberiadaIsomorphismResult;

/* -----------------------------------------------------------------------------
 * The Cyberiada isomorphism check functions
 * ----------------------------------------------------------------------------- */

	/* Compare two SM graphs to detect isomorphism and fill the difference result */
	/* Note: this function ignores comment nodes and edges if the ignore_comments flag is set */
	int cyberiada_check_sm_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2,
									   int ignore_comments, int require_initial,
									   CyberiadaIsomorphismResult* result);

	/* Free the content of the isomorphism check result structure */
	int cyberiada_cleanup_isomorphism_result(CyberiadaIsomorphismResult* result);

	/* Compare two SM graphs (the deprecated form) */
	/* Use cyberiada_check_sm_isomorphism() with the result structure instead */
	int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2, int ignore_comments, int require_initial,
									int* result_flags, CyberiadaNode** new_initial,
									size_t* sm_diff_nodes_size, CyberiadaNodePair** sm_diff_nodes, size_t** sm_diff_nodes_flags,
									size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
									size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes,
									size_t* sm_diff_edges_size, CyberiadaEdgePair** sm_diff_edges, size_t** sm_diff_edges_flags,
									size_t* sm2_new_edges_size, CyberiadaEdge*** sm2_new_edges,
									size_t* sm1_missing_edges_size, CyberiadaEdge*** sm1_missing_edges);

	/* Compare SM nodes actions */
	int cyberiada_compare_node_actions(CyberiadaAction* n1action, CyberiadaAction* n2action, int* compare_flags);

#ifdef __cplusplus
}
#endif

#endif
