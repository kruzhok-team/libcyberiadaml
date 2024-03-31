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
    cybNodeMachineComment = 16,   /* machine-readable comment node */
    cybNodeInitial = 32,          /* initial pseudostate */
    cybNodeFinal = 64,            /* final state */
    cybNodeChoice = 128,          /* choice pseudostate */
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
    cybActionDo = 4,
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
    
/* SM node (state) */
typedef struct _CyberiadaNode {
    CyberiadaNodeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
    CyberiadaAction*            actions;
    CyberiadaRect*              geometry_rect;
    struct _CyberiadaNode*      next;
    struct _CyberiadaNode*      parent;
    struct _CyberiadaNode*      children;
} CyberiadaNode;

/* SM edge (transition) */
typedef struct _CyberiadaEdge {
    CyberiadaEdgeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       source_id;
    size_t                      source_id_len;
    char*                       target_id;
    size_t                      target_id_len;
    CyberiadaNode*              source;
    CyberiadaNode*              target;
    CyberiadaAction*            action;
    CyberiadaPoint*             geometry_source_point;
    CyberiadaPoint*             geometry_target_point;
    CyberiadaPolyline*          geometry_polyline;
    CyberiadaPoint*             geometry_label;
    char*                       color;
    size_t                      color_len;
    struct _CyberiadaEdge*      next;
} CyberiadaEdge;

/* SM graph (state machine) */
typedef struct _CyberiadaSM {
    CyberiadaNode*              nodes;
    CyberiadaEdge*              edges;
    struct _CyberiadaSM*        next;
} CyberiadaSM;

/* SM metainformation */
typedef struct {
    /* HSM standard version (required parameter) */
    char*                       standard_version;
    size_t                      standard_version_len;
    /* target platform name */
    char*                       platform_name;         
    size_t                      platform_name_len;
    /* target platform version */
    char*                       platform_version;
    size_t                      platform_version_len;
    /* target platform language */
    char*                       platform_language;
    size_t                      platform_language_len;
    /* target system controlled by the SM */
    char*                       target_system;         
    size_t                      target_system_len;
    /* document name */
    char*                       name;
    size_t                      name_len;
    /* document author */
    char*                       author;
    size_t                      author_len;
    /* document author's contact */
    char*                       contact;
    size_t                      contact_len;
    /* document description */
    char*                       description;
    size_t                      description_len;
    /* document version */
    char*                       version;
    size_t                      version_len;
    /* document date */
    char*                       date;
    size_t                      date_len;
    /* actions order flag (0 = not set, 1 = transition first, 2 = exit first) */
    char                        actions_order_flag;
    /* event propagation flag (0 = not set, 1 = block events, 2 = propagate events) */
    char                        event_propagation_flag;
} CyberiadaMetainformation;
	
/* SM document */
typedef struct {
    char*                       format;         /* SM document format string */
    size_t                      format_len;     /* SM document format string length */
    CyberiadaMetainformation*   meta_info;      /* SM document metainformation */
    CyberiadaSM*                state_machines; /* State machines */
} CyberiadaDocument;
	
/* SM GraphML supported formats */
typedef enum {
    cybxmlCyberiada = 0,
    cybxmlYED = 1,
    cybxmlUnknown = 2
} CyberiadaXMLFormat;
    
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

    /* Allocate the SM structure in memory (for heap usage) */
    CyberiadaDocument* cyberiada_create_sm_document(void);

    /* Initialize the SM structure */
	/* Do not use the structure before the initialization! */
    int cyberiada_init_sm_document(CyberiadaDocument* doc);

    /* Cleanup the content of the SM structure */
	/* Free the allocated memory of the structure content but not the structure itself */
    int cyberiada_cleanup_sm_document(CyberiadaDocument* doc);
		
    /* Free the allocated SM structure with the content (for heap usage) */
    int cyberiada_destroy_sm_document(CyberiadaDocument* doc);

    /* Read an XML file and decode the SM structure */
    /* Allocate the SM document structure first */
    int cyberiada_read_sm_document(CyberiadaDocument* doc, const char* filename, CyberiadaXMLFormat format);

    /* Encode the SM document structure and write the data to an XML file */
    int cyberiada_write_sm_document(CyberiadaDocument* doc, const char* filename, CyberiadaXMLFormat format);
	
    /* Print the SM structure to stdout */
    int cyberiada_print_sm_document(CyberiadaDocument* doc);

#ifdef __cplusplus
}
#endif
    
#endif
