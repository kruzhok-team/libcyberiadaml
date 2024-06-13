/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The C library header
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

#ifndef __CYBERIADA_ML_H
#define __CYBERIADA_ML_H

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
	
/* SM node types: */
typedef enum {
    cybEdgeTransition = 0,
    cybEdgeComment = 1,
} CyberiadaEdgeType;

/* SM action types: */    
typedef enum {
    cybActionTransition = 0,
    cybActionEntry = 1,
    cybActionExit = 2,
    cybActionDo = 4
} CyberiadaActionType;

/* SM node & transitions geometry */

typedef struct {
    double x, y;
} CyberiadaPoint;

typedef struct {
    double x, y, width, height;
} CyberiadaRect;

typedef struct _CyberiadaPolyline {
    CyberiadaPoint              point;
    struct _CyberiadaPolyline*  next;
} CyberiadaPolyline;

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
 * Comment on the node geometry: 
 * - if the top-level node (state machine) has geometry, it is calculated in
 *   absolute coordinates (x: left-to-right, y: top-to-bottom);
 * - if the state machine has no geometry, the first level nodes (comporite states, initial
 *   vertexes, etc.) are calculated in absolute coordinates;
 * - the child nodes are calculated in local coordinates relative to the _center_ of the
 *   parent node. 
 *
 * NOTE: this geomerty is different from the Cyberiada GraphML 1.0 / YED formats geometry
 * and is being converted during the document import/export.
 */
typedef struct _CyberiadaNode {
    CyberiadaNodeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
	CyberiadaAction*            actions;        /* for simple & composite state nodes */
	CyberiadaCommentData*       comment_data;   /* for comments */
	CyberiadaLink*              link;           /* for submachine states */
	CyberiadaPoint*             geometry_point; /* for some pseudostates & final state */
	CyberiadaRect*              geometry_rect;  /* for sm, states, and choice pseudostate */
    char*                       color;
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
    char*                        color;
    size_t                       color_len;
    struct _CyberiadaEdge*       next;
} CyberiadaEdge;

/* SM graph (state machine) */
typedef struct _CyberiadaSM {
    CyberiadaNode*               nodes;
    CyberiadaEdge*               edges;
	CyberiadaRect*               bounding_rect;
    struct _CyberiadaSM*         next;
} CyberiadaSM;

/* SM metainformation */
typedef struct {
    /* HSM standard version (required parameter) */
    char*                        standard_version;
    size_t                       standard_version_len;
    /* target platform name */
    char*                        platform_name;         
    size_t                       platform_name_len;
    /* target platform version */
    char*                        platform_version;
    size_t                       platform_version_len;
    /* target platform language */
    char*                        platform_language;
    size_t                       platform_language_len;
    /* target system controlled by the SM */
    char*                        target_system;         
    size_t                       target_system_len;
    /* document name */
    char*                        name;
    size_t                       name_len;
    /* document author */
    char*                        author;
    size_t                       author_len;
    /* document author's contact */
    char*                        contact;
    size_t                       contact_len;
    /* document description */
    char*                        description;
    size_t                       description_len;
    /* document version */
    char*                        version;
    size_t                       version_len;
    /* document date */
    char*                        date;
    size_t                       date_len;
    /* transition order flag (0 = not set, 1 = transition first, 2 = exit first) */
    char                         transition_order_flag;
    /* event propagation flag (0 = not set, 1 = block events, 2 = propagate events) */
    char                         event_propagation_flag;
	/* default comments markup language */
	char*                        markup_language;
	size_t                       markup_language_len;
} CyberiadaMetainformation;

/* SM document node coordinates format */
typedef enum {
	cybCoordNone = 0,                    /* no geometry information */
	cybCoordAbsolute = 1,                /* absolute coordinates (used in YED) */ 
	cybCoordLeftTop = 2,                 /* left-top-oriented local coordinates (used in Cyberiada10) */
	cybCoordLocalCenter = 4,             /* center-oriented local coordinates (used by default) */	
} CyberiadaGeometryCoordFormat;

/* SM document edges source/target point placement & coordinates format */
typedef enum {
	cybEdgeNone = 0,
	cybEdgeLocalCenter = 1,               /* source & target points are bind to a node's center */
	cybEdgeLeftTopBorder = 2,             /* source & target points are placed on a node's border with left-top coordinates */
	cybEdgeCenterBorder = 4,              /* source & target points are placed on a node's border with center coordinates */
} CyberiadaGeometryEdgeFormat;

/* SM document */
typedef struct {
    char*                            format;           /* SM document format string (additional info) */
    size_t                           format_len;       /* SM document format string length */
    CyberiadaMetainformation*        meta_info;        /* SM document metainformation */
	CyberiadaGeometryCoordFormat     geometry_format;  /* SM document geometry coordinates format */
	CyberiadaGeometryEdgeFormat      edge_geom_format; /* SM document edges geometry format */ 
    CyberiadaSM*                     state_machines;   /* State machines */
} CyberiadaDocument;
	
/* Cyberiada GraphML Library supported formats */
typedef enum {
    cybxmlCyberiada10 = 0,                       /* Cyberiada 1.0 format */
    cybxmlYED = 1,                               /* Old YED-based Berloga/Ostranna format */
    cybxmlUnknown = 99                           /* Format is not specified */
} CyberiadaXMLFormat;

/* Cyberiada GraphML Library import/export flags */
#define CYBERIADA_FLAG_NO                           0
#define CYBERIADA_FLAG_ABSOLUTE_GEOMETRY            1   /* convert imported geometry to absolute coordinates */ 
#define CYBERIADA_FLAG_LEFTTOP_LOCAL_GEOMETRY       2   /* convert imported geometry to left-top-oriented local coordinates */
#define CYBERIADA_FLAG_CENTER_LOCAL_GEOMETRY        4   /* convert imported geometry to center-oriented local coordinates */
#define CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY         8   /* convert imported geometry to center edge coordinates */
#define CYBERIADA_FLAG_LEFTTOP_BORDER_EDGE_GEOMETRY 16  /* convert imported geometry to border edge coordinates (left-top) */
#define CYBERIADA_FLAG_CENTER_BORDER_EDGE_GEOMETRY  32  /* convert imported geometry to border edge coordinates (center) */
#define CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY         64  /* reconstruct absent node/edge geometry on import */
#define CYBERIADA_FLAG_SKIP_GEOMETRY                128 /* skip geometry node/edge during import/export */
#define CYBERIADA_FLAG_ROUND_GEOMETRY               256 /* export geometry with round coordinates to 0.001 */

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

	/* Allocate and initialize the SM structure in memory */
	CyberiadaSM* cyberiada_new_sm(void);

	/* Allocate and initialize the SM node structure in memory */
	CyberiadaNode* cyberiada_new_node(const char* id);

	/* Allocate and initialize the SM edge structure in memory */
	CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target);

	/* Allocate and initialize the SM comment data structure in memory */
	CyberiadaCommentData* cyberiada_new_comment_data(void);

	/* Allocate and initialize the SM link structure in memory */
	CyberiadaLink* cyberiada_new_link(const char* ref);

	/* Allocate and initialize the SM comment subject structure in memory */
	CyberiadaCommentSubject* cyberiada_new_comment_subject(CyberiadaCommentSubjectType type);

	/* Allocate and initialize the SM action structure in memory */
	CyberiadaAction* cyberiada_new_action(CyberiadaActionType type, const char* trigger, const char* guard, const char* behavior);

	/* Allocate and initialize the SM point structure in memory */
	CyberiadaPoint* cyberiada_new_point(void);

	/* Allocate and initialize the SM rect structure in memory */
	CyberiadaRect* cyberiada_new_rect(void);
	
	/* Allocate and initialize the SM polyline structure in memory */
	CyberiadaPolyline* cyberiada_new_polyline(void);

	/* Free the SM polyline structure in memory */
	int cyberiada_destroy_polyline(CyberiadaPolyline* polyline);

	/* Check the presence of the SM document geometry, return 1 if there is any geometry object available */
	int cyberiada_document_has_geometry(CyberiadaDocument* doc);
	
	/* Change the SM document geometry format and convert the SMs geometry data */
	int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
											CyberiadaGeometryCoordFormat new_format,
											CyberiadaGeometryEdgeFormat new_edge_format);

    /* Allocate and initialize the SM metainformation structure in memory */
	CyberiadaMetainformation* cyberiada_new_meta(void);
	
	/* Initialize and copy string. Use this function to initialize strings in Cyberiada structures */
	int cyberiada_copy_string(char** target, size_t* size, const char* source);

	/* Encode metainformation to string */
	int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body, size_t* meta_body_len);

    /* Free metainformation struct */
	int cyberiada_destroy_meta(CyberiadaMetainformation* meta);

#ifdef __cplusplus
}
#endif
    
#endif
