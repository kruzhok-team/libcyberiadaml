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
    cybNodeInitial = 16,          /* initial pseudostate */
    cybNodeFinal = 32,            /* final pseudostate */
    cybNodeChoice = 64,           /* final pseudostate */
    cybNodeEntry = 128,           /* entry pseudostate */
    cybNodeExit = 256,            /* exit pseudostate */
    cybNodeShallowHistory = 512,  /* shallow history pseudostate */
    cybNodeTerminate = 1024,      /* terminate pseudostate */
} CyberiadaNodeType;

typedef unsigned int CyberiadaNodeTypeMask;
	
/* SM node types: */    
typedef enum {
    cybEdgeTransition = 0,
	cybEdgeComment = 1,
} CyberiadaEdgeType;
    
/* SM behavior types: */    
typedef enum {
    cybBehaviorTransition = 0,
    cybBehaviorEntry = 1,
    cybBehaviorExit = 2,
    cybBehaviorDo = 4,
} CyberiadaBehaviorType;

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
typedef struct _CyberiadaBehavior {
    CyberiadaBehaviorType       type;
    char*                       trigger;
    size_t                      trigger_len;
    char*                       guard;
    size_t                      guard_len;
    char*                       action;
    size_t                      action_len;
	struct _CyberiadaBehavior*  next;
} CyberiadaBehavior;
    
/* SM node (state) */
typedef struct _CyberiadaNode {
    CyberiadaNodeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
	CyberiadaBehavior*          behavior;
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
    CyberiadaBehavior*          behavior;
    CyberiadaPoint*             geometry_source_point;
    CyberiadaPoint*             geometry_target_point;
    CyberiadaPolyline*          geometry_polyline;
    CyberiadaPoint*             geometry_label;
    char*                       color;
    size_t                      color_len;
    struct _CyberiadaEdge*      next;
} CyberiadaEdge;

/* SM metainformation */
typedef struct {
	char* standard_version;      /* HSM standard version (required parameter) */
	size_t standard_version_len;
	char* platform_name;         /* target platform name */
	size_t platform_name_len;
	char* platform_version;      /* target platform version */
	size_t platform_version_len;
	char* platform_language;     /* target platform language */
	size_t platform_language_len;
	char* target_system;         /* target system controlled by the SM */
	size_t target_system_len;
	char* name;                  /* document name */
	size_t name_len;
	char* author;                /* document author */
	size_t author_len;
	char* contact;               /* document author's contact */
	size_t contact_len;
	char* description;           /* document description */
	size_t description_len;
	char* version;               /* document version */
	size_t version_len;
	char* date;                  /* document date */
	size_t date_len;
	char actions_order_flag;     /* actions order flag (0 = not set, 1 = transition first, 2 = exit first) */
	char event_propagation_flag; /* event propagation flag (0 = not set, 1 = block events, 2 = propagate events) */
} CyberiadaMetainformation;
	
/* SM graph (state machine) */
typedef struct {
    char*                       format;     /* SM graph format string */
    size_t                      format_len; /* SM graph format string length */
	CyberiadaMetainformation*   meta_info;
    CyberiadaNode*              nodes;
    CyberiadaEdge*              edges;
} CyberiadaSM;

/* SM GraphML supported formats */
typedef enum {
	cybxmlCyberiada = 0,
    cybxmlYED,
    cybxmlUnknown
} CyberiadaXMLFormat;
    
/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML error codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_NO_ERROR                0
#define CYBERIADA_XML_ERROR               1
#define CYBERIADA_FORMAT_ERROR            2
#define CYBERIADA_BEHAVIOR_FORMAT_ERROR   3
#define CYBERIADA_METADATA_FORMAT_ERROR   4
#define CYBERIADA_NOT_FOUND               5
#define CYBERIADA_BAD_PARAMETER           6
#define CYBERIADA_ASSERT                  7

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library functions
 * ----------------------------------------------------------------------------- */

    /* Allocate the SM structure in memory (for heap usage) */
    CyberiadaSM* cyberiada_create_sm(void);

    /* Initialize the SM structure */
	/* Do not use the structure before the initialization! */
    int cyberiada_init_sm(CyberiadaSM* sm);

    /* Cleanup the content of the SM structure */
	/* Free the allocated memory of the structure content but not the structure itself */
    int cyberiada_cleanup_sm(CyberiadaSM* sm);

    /* Free the allocated SM structure with the content (for heap usage) */
    int cyberiada_destroy_sm(CyberiadaSM* sm);

    /* Read an XML file and decode the SM structure */
	/* Allocate the SM structure first */
    int cyberiada_read_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format);

    /* Encode the SM structure and write the data to an XML file */
    int cyberiada_write_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format);
	
    /* Print the SM structure to stdout */
    int cyberiada_print_sm(CyberiadaSM* sm);

#ifdef __cplusplus
}
#endif
    
#endif
