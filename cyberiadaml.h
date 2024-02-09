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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML format library types
 * ----------------------------------------------------------------------------- */

/* SM node types: */    
typedef enum {
	cybNodeSM = 0,          /* state machine */
    cybNodeSimple,          /* simple state */
    cybNodeComposite,       /* composite state */
    cybNodeComment,         /* comment node */
    cybNodeInitial,         /* initial pseudostate */
	cybNodeFinal,           /* final pseudostate */
	cybNodeChoice,          /* final pseudostate */
	cybNodeJunction,        /* junction pseudostate */
	cybNodeEntry,			/* entry pseudostate */
	cybNodeExit,			/* exit pseudostate */
} CyberiadaNodeType;

/* SM node types: */    
typedef enum {
	cybEdgeTransition = 0,
	cybEdgeComment
} CyberiadaEdgeType;
	
/* SM node geometry */

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

/* SM node (state) */
typedef struct _CyberiadaNode {
    CyberiadaNodeType           type;
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
    char*                       action;
    size_t                      action_len;
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
    char*                       action;
    size_t                      action_len;
    CyberiadaPoint*             geometry_source_point;
    CyberiadaPoint*             geometry_target_point;
    CyberiadaPolyline*          geometry_polyline;
	CyberiadaPoint*             geometry_label;
	char*                       color;
	size_t                      color_len;
    struct _CyberiadaEdge*      next;
} CyberiadaEdge;

/* SM extentions 
typedef struct _CyberiadaExtension {
    char*                       id;
	size_t                      id_len;
    char*                       title;
	size_t                      title_len;
    char*                       data;
	size_t                      data_len;
	struct _CyberiadaExtension* next;
	} CyberiadaExtension;*/
	
/* SM graph (state machine) */
typedef struct {
    char*                       name;
    size_t                      name_len;
    char*                       version;
    size_t                      version_len;
	char*                       info;
	size_t                      info_len;
    CyberiadaNode*              nodes;
    CyberiadaEdge*              edges;
/*	CyberiadaExtension*         extensions;*/
} CyberiadaSM;

/* SM GraphML supported formats */
typedef enum {
    cybxmlYED = 0,
    cybxmlCyberiada,
	cybxmlUnknown
} CyberiadaXMLFormat;
    
/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML error codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_NO_ERROR       0
#define CYBERIADA_XML_ERROR      1
#define CYBERIADA_FORMAT_ERROR   2
#define CYBERIADA_NOT_FOUND      3
#define CYBERIADA_BAD_PARAMETER  4
#define CYBERIADA_ASSERT         5

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library functions
 * ----------------------------------------------------------------------------- */

    /* Allocate the SM structure in memory (for heap usage) */
    CyberiadaSM* cyberiada_create_sm(void);

    /* Initialize the SM structure. Do not use the structure before the initialization! */
    int cyberiada_init_sm(CyberiadaSM* sm);

    /* Cleanup the content of the SM structure, free the conents memory */
    int cyberiada_cleanup_sm(CyberiadaSM* sm);

    /* Free the allocated SM structure (for heap usage) */
    int cyberiada_destroy_sm(CyberiadaSM* sm);

    /* Read an XML file and decode the SM structure */
    int cyberiada_read_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format);
    
    /* Print the SM structure to stdout */
    int cyberiada_print_sm(CyberiadaSM* sm);

#ifdef __cplusplus
}
#endif
    
#endif
