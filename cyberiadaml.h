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
 * ----------------------------------------------------------------------------- */

#ifndef __CYBERIADA_ML_H
#define __CYBERIADA_ML_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library types
 * ----------------------------------------------------------------------------- */

/* SM node types: */    
typedef enum {
    cybNodeInitial = 0,     /* initial node */
    cybNodeSimple,          /* simple node */
    cybNodeComplex,         /* complex node */
    cybNodeComment          /* comment node */
} CyberiadaNodeType;

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
    char*                       id;
    size_t                      id_len;
    char*                       title;
    size_t                      title_len;
    CyberiadaNodeType           type;
    char*                       action;
    size_t                      action_len;
    CyberiadaRect               geometry_rect;
    struct _CyberiadaNode*      next;
    struct _CyberiadaNode*      parent;
    struct _CyberiadaNode*      children;
} CyberiadaNode;

/* SM edge (transition) */
typedef struct _CyberiadaEdge {
    char*                       id;
    size_t                      id_len;
    CyberiadaNode*              source;
    CyberiadaNode*              target;
    char*                       action;
    size_t                      action_len;
    CyberiadaPoint              geometry_source_point;
    CyberiadaPoint              geometry_target_point;
    CyberiadaPolyline*          geometry_polyline;
    struct _CyberiadaEdge*      next;
} CyberiadaEdge;

/* SM graph (state machine) */
typedef struct {
    char*                       name;
    size_t                      name_len;
    char*                       version;
    size_t                      version_len;
    CyberiadaNode*              nodes;
    CyberiadaNode*              start;
    CyberiadaEdge*              edges;
} CyberiadaSM;

/* SM GraphML supported formats */
typedef enum {
    cybxmlYED = 0,
    cybxmlCyberiada
} CyberiadaXMLFormat;
    
/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML error codes
 * ----------------------------------------------------------------------------- */

#define CYBERIADA_NO_ERROR       0
#define CYBERIADA_XML_ERROR      1
#define CYBERIADA_FORMAT_ERROR   2
#define CYBERIADA_NOT_FOUND      3
#define CYBERIADA_BAD_PARAMETER  4

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
