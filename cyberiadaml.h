/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The C library header
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

#ifndef __CYBERIADA_ML_H
#define __CYBERIADA_ML_H

#include <cyberiada/htgeom.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML format library types
 * ----------------------------------------------------------------------------- */

/* SM node types: */    
typedef enum {
    cybNodeSM = 0,                /* state machine */
    cybNodeSimpleState = 1,       /* simple state */
    cybNodeCompositeState = 2,    /* composite state */
    cybNodeSubmachineState = 4,   /* submachine state */
    cybNodeComment = 8,           /* comment node */
    cybNodeFormalComment = 16,    /* machine-readable comment node */
    cybNodeInitial = 32,          /* initial pseudostate */
    cybNodeFinal = 64,            /* final state */
    cybNodeChoice = 128,          /* choice pseudostate */
	cybNodeTerminate = 256,       /* terminate pseudostate */
	cybNodeEntryPoint = 512,      /* entry point pseudostate */
	cybNodeExitPoint = 1024,      /* exit point pseudostate */
	cybNodeShallowHistory = 2048, /* shallow history pseudostate */
	cybNodeDeepHistory = 4096,    /* deep history pseudostate */
	cybNodeFork = 8192,           /* fork pseudostate */
	cybNodeJoin = 16384,          /* join pseudostate */
} CyberiadaNodeType;

typedef unsigned int CyberiadaNodeTypeMask;
	
/* SM edge types: */
typedef enum {
    cybEdgeLocalTransition = 0,
	cybEdgeExternalTransition = 1,
    cybEdgeComment = 2,
} CyberiadaEdgeType;

/* SM action types: */    
typedef enum {
    cybActionTransition = 0,
    cybActionEntry = 1,
    cybActionExit = 2,
    cybActionDo = 4
} CyberiadaActionType;

/* SM behavior */
typedef struct _CyberiadaAction {
    CyberiadaActionType         type;
    char*                       trigger;
    size_t                      trigger_len;
    char*                       guard;
    size_t                      guard_len;
    char*                       behavior;
    size_t                      behavior_len;
    struct _CyberiadaAction*    next;
} CyberiadaAction;

/* SM comment content */
typedef struct _CyberiadaCommentData {
	char*                       body;
	size_t                      body_len;
	char*                       markup;
	size_t                      markup_len;
} CyberiadaCommentData;

/* SM link (e.g. to a SM definition) */
typedef struct _CyberiadaLink {
	char*                       ref;
	size_t                      ref_len;
} CyberiadaLink;

/* SM node (state)
 *
 * Comment on the node geometry. There are three geometry coordinates formats
 * (see the libhtgeom for details):
 * - Cyberiada-GraphML 1.0 geometry (hierarhical, relative to the top-left corner);
 * - Legacy YED geometry (absolute);
 * - Qt QGraphicsView geometry (hierarchical, relative to the parent node center). 
 *
 * Qt QGraphicsView geometry details:
 *
 * - rectangles and points store the node center (x, y) as well as the node width/length; 
 * - if the top-level node (state machine) has geometry, it can be interpreted as
 *   absolute coordinates (x: left-to-right, y: top-to-bottom);
 * - if the state machine has no geometry, the first level nodes (comporite states, initial
 *   vertexes, etc.) are calculated in absolute coordinates;
 * - the SM / the first level nodes geometry is calculated relatively to the _center_ of the
 *   bounding rect; 
 * - the child nodes are calculated in local coordinates relative to the _center_ of the
 *   parent node. 
 *
 * NOTE: this geomerty is different from the Cyberiada GraphML 1.0 / YED formats geometry
 * and can be converted during the document import/export.
 */

typedef HTreePoint    CyberiadaPoint;
typedef HTreeRect     CyberiadaRect;
typedef HTreePolyline CyberiadaPolyline;
	
typedef struct _CyberiadaNode {
    CyberiadaNodeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
	CyberiadaAction*            actions;              /* for simple & composite state nodes */
	CyberiadaCommentData*       comment_data;         /* for comments */
	CyberiadaLink*              link;                 /* for submachine states */
	CyberiadaPoint*             geometry_point;       /* for some pseudostates & final state */
	CyberiadaRect*              geometry_rect;        /* for sm, states, and choice pseudostate */
    char*                       color;                /* for nodes with geometry */
    size_t                      color_len;
    struct _CyberiadaNode*      parent;
    struct _CyberiadaNode*      children;
    struct _CyberiadaNode*      next;
} CyberiadaNode;

typedef enum {
	cybCommentSubjectNode = 0,
	cybCommentSubjectNameFragment = 1,
	cybCommentSubjectDataFragment = 2,
} CyberiadaCommentSubjectType;
	
typedef struct _CyberiadaCommentSubject {
	CyberiadaCommentSubjectType type;
	char*                       fragment;
	size_t                      fragment_len;
} CyberiadaCommentSubject;
	
/* SM edge (transition) 
 *
 * Comment on the edge geometry: 
 * - the source and target points are calculated in local coordinates relative
 *   to the center of the corresponding source/target node;
 * - the label point is calculated in local coordinates relative to the geometrical
 *   center of the edge (based on the bounding rectangle);
 * - the polyline points are calculated in local coordinated relative to the center of
 *   the source node.
 *
 * NOTE: this geomerty is different from the Cyberiada GraphML 1.0 / YED formats geometry
 * and is being converted during the document import/export.
 */
typedef struct _CyberiadaEdge {
    CyberiadaEdgeType            type;
    char*                        id;
    size_t                       id_len;
    char*                        source_id;
    size_t                       source_id_len;
    char*                        target_id;
    size_t                       target_id_len;
    CyberiadaNode*               source;
    CyberiadaNode*               target;
	CyberiadaAction*             action;                /* for transition */
	CyberiadaCommentSubject*     comment_subject;       /* for comment subject */
    CyberiadaPolyline*           geometry_polyline;
    CyberiadaPoint*              geometry_source_point;
    CyberiadaPoint*              geometry_target_point;
    CyberiadaPoint*              geometry_label_point;
	CyberiadaRect*               geometry_label_rect;
    char*                        color;
    size_t                       color_len;
    struct _CyberiadaEdge*       next;
} CyberiadaEdge;

/* SM graph (state machine) */
typedef struct _CyberiadaSM {
    CyberiadaNode*               nodes;
    CyberiadaEdge*               edges;
    struct _CyberiadaSM*         next;
} CyberiadaSM;

/* SM mandatory metainformation constants */
#define CYBERIADA_META_STANDARD_VERSION          "standardVersion"
#define CYBERIADA_META_NAME                      "name"
#define CYBERIADA_META_TRANSITION_ORDER          "transitionOrder"
#define CYBERIADA_META_AO_TRANSITION             "transitionFirst"
#define CYBERIADA_META_AO_EXIT                   "exitFirst"
#define CYBERIADA_META_EVENT_PROPAGATION         "eventPropagation"
#define CYBERIADA_META_EP_PROPAGATE              "propagate"
#define CYBERIADA_META_EP_BLOCK                  "block"

typedef struct _CyberiadaMetaStringList {
	char*                            name;
	size_t                           name_len;
	char*                            value;
	size_t                           value_len;
	struct _CyberiadaMetaStringList* next; 
} CyberiadaMetaStringList;

/* SM metainformation */
typedef struct {
    /* HSM standard version (required parameter) */
    char*                        standard_version;
	size_t                       standard_version_len;
    /* transition order flag (0 = not set, 1 = transition first, 2 = exit first) */
    char                         transition_order_flag;
    /* event propagation flag (0 = not set, 1 = block events, 2 = propagate events) */
    char                         event_propagation_flag;
	CyberiadaMetaStringList*     strings;
} CyberiadaMetainformation;

/* SM document node coordinates format */
typedef HTCoordFormat CyberiadaGeometryCoordFormat;

/* SM document edges source/target point placement & coordinates format */
typedef HTEdgeFormat CyberiadaGeometryEdgeFormat;

/* SM document */
typedef struct {
    char*                            format;               /* SM document format string (additional info) */
    size_t                           format_len;           /* SM document format string length */
    CyberiadaMetainformation*        meta_info;            /* SM document metainformation */
	CyberiadaGeometryCoordFormat     node_coord_format;    /* SM document node geometry coordinates format */
	CyberiadaGeometryCoordFormat     edge_coord_format;    /* SM document edge (source/target points) geometry coordinates format */
	CyberiadaGeometryCoordFormat     edge_pl_coord_format; /* SM document edge (polylines) geometry coordinates format */
	CyberiadaGeometryEdgeFormat      edge_geom_format;     /* SM document edges geometry format */ 
	CyberiadaRect*                   bounding_rect;        /* SM document bounding rect */
    CyberiadaSM*                     state_machines;       /* State machines */
} CyberiadaDocument;

/* Cyberiada GraphML Library supported formats */
typedef enum {
    cybxmlCyberiada10 = 0,                       /* Cyberiada 1.0 format */
    cybxmlYED = 1,                               /* Old YED-based Berloga/Ostranna format */
    cybxmlUnknown = 99                           /* Format is not specified */
} CyberiadaXMLFormat;

/* Cyberiada GraphML Library import/export flags */
#define CYBERIADA_FLAG_NO                                 0

#define CYBERIADA_FLAG_NODES_ABSOLUTE_GEOMETRY            1    /* convert imported nodes geometry to absolute coordinates */ 
#define CYBERIADA_FLAG_NODES_LEFTTOP_LOCAL_GEOMETRY       2    /* convert imported nodes geometry to left-top-oriented local coordinates */
#define CYBERIADA_FLAG_NODES_CENTER_LOCAL_GEOMETRY        4    /* convert imported nodes geometry to center-oriented local coordinates */
#define CYBERIADA_FLAG_NODES_GEOMETRY                     (1 | 2 | 4)

#define CYBERIADA_FLAG_EDGES_ABSOLUTE_GEOMETRY            8    /* convert imported edges geometry to absolute coordinates */ 
#define CYBERIADA_FLAG_EDGES_LEFTTOP_LOCAL_GEOMETRY       16   /* convert imported edges geometry to left-top-oriented local coordinates */
#define CYBERIADA_FLAG_EDGES_CENTER_LOCAL_GEOMETRY        32   /* convert imported edges geometry to center-oriented local coordinates */
#define CYBERIADA_FLAG_EDGES_GEOMETRY                     (8 | 16 | 32)
	
#define CYBERIADA_FLAG_EDGES_PL_ABSOLUTE_GEOMETRY         64   /* convert imported edge polylines geometry to absolute coordinates */ 
#define CYBERIADA_FLAG_EDGES_PL_LEFTTOP_LOCAL_GEOMETRY    128  /* convert imported edge polylines geometry to left-top-oriented local coordinates */
#define CYBERIADA_FLAG_EDGES_PL_CENTER_LOCAL_GEOMETRY     256  /* convert imported edge polylines geometry to center-oriented local coordinates */
#define CYBERIADA_FLAG_EDGES_PL_GEOMETRY                  (64 | 128 | 256)

#define CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY               512  /* convert imported geometry to center edge coordinates */
#define CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY               1024 /* convert imported geometry to border edge coordinates (left-top) */
#define CYBERIADA_FLAG_EDGE_TYPE_GEOMETRY                 (512 | 1024)

#define CYBERIADA_FLAG_ANY_GEOMETRY                       0x7ff

#define CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY               2048 /* reconstruct absent node/edge geometry on import */
#define CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY            4096 /* reconstruct absent SM geometry on import */
#define CYBERIADA_FLAG_SKIP_GEOMETRY                      8192 /* skip geometry node/edge during import/export */
#define CYBERIADA_FLAG_ROUND_GEOMETRY                     16384 /* export geometry with round coordinates to 0.001 */

#define CYBERIADA_FLAG_FLATTENED                          32768 /* the document is flattened  */
#define CYBERIADA_FLAG_CHECK_INITIAL                      65536 /* check initial state on the top level  */
#define CYBERIADA_NON_GEOMETRY_FLAGS_MASK                 (32768 | 65536)

/* -----------------------------------------------------------------------------
 * The Cyberiada isomorphism check codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_ISOMORPH_FLAG_IDENTICAL                 1    /* the two SM graphs are identical (even ids are the same) */
#define CYBERIADA_ISOMORPH_FLAG_EQUAL                     2    /* the two SM graphs are equal */
#define CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC                4    /* the two SM graphs are isomorphic (but there are differences) */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_STATES               8    /* the two SM graphs are not isomorhic and have different states */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL              16    /* the two SM graphs are not isomorhic and have different initial pseudostates */
#define CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES                32   /* the two SM graphs are not isomorhic and have different edges */
#define CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK           (1 | 2 | 4)
#define CYBERIADA_ISOMORPH_FLAG_DIFF_MASK                 (8 | 16 | 32)
	
#define CYBERIADA_NODE_DIFF_ID                            1    /* the two SM nodes have different identifiers */
#define CYBERIADA_NODE_DIFF_TYPE                          2    /* the two SM nodes have different types (excluding simple/comp. state) */
#define CYBERIADA_NODE_DIFF_TITLE                         4    /* the two SM nodes have different titles */
#define CYBERIADA_NODE_DIFF_ACTIONS                       8    /* the two SM nodes have different actions */
#define CYBERIADA_NODE_DIFF_SM_LINK                       16   /* the two SM nodes have different links to a state machine */
#define CYBERIADA_NODE_DIFF_CHILDREN                      32   /* the two SM nodes have different number of children */
#define CYBERIADA_NODE_DIFF_EDGES                         64   /* the two SM nodes have different incoming/outgoing edges */

#define CYBERIADA_EDGE_DIFF_ID                            128  /* the two SM edges have different identifiers */
#define CYBERIADA_EDGE_DIFF_ACTION                        256  /* the two SM edges have different actions */

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML error codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_NO_ERROR                0
#define CYBERIADA_XML_ERROR               1
#define CYBERIADA_FORMAT_ERROR            2
#define CYBERIADA_ACTION_FORMAT_ERROR     3
#define CYBERIADA_METADATA_FORMAT_ERROR   4
#define CYBERIADA_NOT_FOUND               5
#define CYBERIADA_BAD_PARAMETER           6
#define CYBERIADA_ASSERT                  7
#define CYBERIADA_NOT_IMPLEMENTED         8

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library functions
 * ----------------------------------------------------------------------------- */

    /* Allocate the SM document structure in memory (for heap usage) */
    CyberiadaDocument* cyberiada_new_sm_document(void);

    /* Initialize the SM structure */
	/* Do not use the structure before the initialization! */
    int cyberiada_init_sm_document(CyberiadaDocument* doc);

    /* Deep copy the SM structure */
    CyberiadaDocument* cyberiada_copy_sm_document(CyberiadaDocument* source_doc);
	
    /* Cleanup the content of the SM structure */
	/* Free the allocated memory of the structure content but not the structure itself */
    int cyberiada_cleanup_sm_document(CyberiadaDocument* doc);
		
    /* Free the allocated SM structure with the content (for heap usage) */
    int cyberiada_destroy_sm_document(CyberiadaDocument* doc);

    /* Read an XML file and decode the SM structure */
    /* Allocate the SM document structure first */
    int cyberiada_read_sm_document(CyberiadaDocument* doc, const char* filename, CyberiadaXMLFormat format, int flags);

    /* Encode the SM document structure and write the data to an XML file */
    int cyberiada_write_sm_document(CyberiadaDocument* doc, const char* filename, CyberiadaXMLFormat format, int flags);

    /* Decode the SM structure */
    /* Allocate the SM document structure first */
    int cyberiada_decode_sm_document(CyberiadaDocument* doc, const char* buffer, size_t buffer_size,
									 CyberiadaXMLFormat format, int flags);

    /* Encode the SM document structure */
    int cyberiada_encode_sm_document(CyberiadaDocument* doc, char** buffer, size_t* buffer_size,
									 CyberiadaXMLFormat format, int flags);
	
    /* Print the SM structure to stdout */
    int cyberiada_print_sm_document(CyberiadaDocument* doc);

	/* Print the node structure to stdout using indent of <level> spaces */
	int cyberiada_print_node(CyberiadaNode* node, int level);

	/* Print the edge structure to stdout */	
	int cyberiada_print_edge(CyberiadaEdge* edge);
	
	/* Allocate and initialize the SM structure in memory */
	CyberiadaSM* cyberiada_new_sm(void);

	/* Get SM graph size (vertexes and edges) */
	int cyberiada_sm_size(CyberiadaSM* sm, size_t* v, size_t* e, int ignore_comments);

	/* Free the SM structure */	
	int cyberiada_destroy_sm(CyberiadaSM* sm);
	
	/* Allocate and initialize the SM node structure in memory */
	CyberiadaNode* cyberiada_new_node(const char* id);

	/* Allocate and initialize the SM edge structure in memory */
	CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target, int external);

	/* Allocate and initialize the SM comment data structure in memory */
	CyberiadaCommentData* cyberiada_new_comment_data(void);

	/* Allocate and initialize the SM link structure in memory */
	CyberiadaLink* cyberiada_new_link(const char* ref);

	/* Allocate and initialize the SM comment subject structure in memory */
	CyberiadaCommentSubject* cyberiada_new_comment_subject(CyberiadaCommentSubjectType type);

	/* Allocate and initialize the SM action structure in memory */
	CyberiadaAction* cyberiada_new_action(CyberiadaActionType type, const char* trigger, const char* guard, const char* behavior);

	/* Compare two SM graphs to detect isomorphism and the difference if the graphs are not isomorphic */
	/* Note: this function ignores comment nodes and edges if the ignore_comments flag is set          */
	int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2, int ignore_comments, int require_initial,
									int* result_flags, CyberiadaNode** new_initial,
									size_t* sm_diff_nodes_size, CyberiadaNode*** sm_diff_nodes, size_t** sm_diff_nodes_flags,
									size_t* sm2_new_nodes_size, CyberiadaNode*** sm2_new_nodes,
									size_t* sm1_missing_nodes_size, CyberiadaNode*** sm1_missing_nodes,
									size_t* sm_diff_edges_size, CyberiadaEdge*** sm_diff_edges, size_t** sm_diff_edges_flags,
									size_t* sm2_new_edges_size, CyberiadaEdge*** sm2_new_edges,
									size_t* sm1_missing_edges_size, CyberiadaEdge*** sm1_missing_edges);

	/* Check the presence of the SM document geometry, return 1 if there is any geometry object available */
	int cyberiada_document_has_geometry(CyberiadaDocument* doc);
	
	/* Clean the SM document geometry */
	int cyberiada_clean_document_geometry(CyberiadaDocument* doc);

	/* Reconstruct the SM document geometry from scratch */
	int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
	
	/* Change the SM document geometry format and convert the SMs geometry data */
	int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
											CyberiadaGeometryCoordFormat new_node_coord_format,
											CyberiadaGeometryCoordFormat new_edge_coord_format,
											CyberiadaGeometryCoordFormat new_edge_pl_coord_format,
											CyberiadaGeometryEdgeFormat new_edge_format);

    /* Allocate and initialize the SM metainformation structure in memory */
	CyberiadaMetainformation* cyberiada_new_meta(void);
	
	/* Initialize and copy string. Use this function to initialize strings in Cyberiada structures */
	int cyberiada_copy_string(char** target, size_t* size, const char* source);

	/* Encode metainformation to string */
	int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body, size_t* meta_body_len);

	/* Initialize new metainfo string parameter */
	CyberiadaMetaStringList* cyberiada_new_meta_string(const char* _name, const char* _value);
	
    /* Free metainformation struct */
	int cyberiada_destroy_meta(CyberiadaMetainformation* meta);
	
	/* Find node by id, return NULL if not found */
	CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id);
	
	/* Find first node by type, return NULL if not found */
	CyberiadaNode* cyberiada_graph_find_node_by_type(CyberiadaNode* root, CyberiadaNodeTypeMask mask);

#ifdef __cplusplus
}
#endif
    
#endif
