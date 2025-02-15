/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The C library implementation
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <stddef.h>
#include <math.h>

#include "cyberiadaml.h"
#include "cyb_actions.h"
#include "cyb_error.h"
#include "cyb_graph.h"
#include "cyb_graph_recon.h"
#include "cyb_meta.h"
#include "cyb_regexps.h"
#include "cyb_string.h"
#include "cyb_structs.h"
#include "cyb_types.h"
#include "geometry.h"
#include "utf8enc.h"

/* -----------------------------------------------------------------------------
 * Cyberiada parser constants 
 * ----------------------------------------------------------------------------- */

/* Core GraphML constants */

#define GRAPHML_XML_ENCODING                     "utf-8"
#define GRAPHML_NAMESPACE_URI 				     "http://graphml.graphdrawing.org/xmlns"
#define GRAPHML_GRAPHML_ELEMENT				     "graphml"
#define GRAPHML_GRAPH_ELEMENT				     "graph"
#define GRAPHML_NODE_ELEMENT				     "node"
#define GRAPHML_EDGE_ELEMENT				     "edge"
#define GRAPHML_DATA_ELEMENT				     "data"
#define GRAPHML_KEY_ELEMENT					     "key"
#define GRAPHML_PORT_ELEMENT                     "port"
#define GRAPHML_POINT_ELEMENT					 "point"
#define GRAPHML_RECT_ELEMENT					 "rect"
#define GRAPHML_ID_ATTRIBUTE				     "id"
#define GRAPHML_KEY_ATTRIBUTE				     "key"
#define GRAPHML_FOR_ATTRIBUTE				     "for"
#define GRAPHML_NAME_ATTRIBUTE				     "name"
#define GRAPHML_EDGEDEFAULT_ATTRIBUTE            "edgedefault"
#define GRAPHML_ATTR_NAME_ATTRIBUTE			     "attr.name"
#define GRAPHML_ATTR_TYPE_ATTRIBUTE			     "attr.type"
#define GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE      "directed"
#define GRAPHML_SOURCE_ATTRIBUTE			     "source"
#define GRAPHML_TARGET_ATTRIBUTE			     "target"

/* Common GraphML constants */

#define GRAPHML_GEOM_X_ATTRIBUTE			     "x"
#define GRAPHML_GEOM_Y_ATTRIBUTE			     "y"
#define GRAPHML_GEOM_WIDTH_ATTRIBUTE		     "width"
#define GRAPHML_GEOM_HEIGHT_ATTRIBUTE		     "height"

/* Cyberiada-GraphML format constants */

#define GRAPHML_CYB_GRAPH_VERTEX_INITIAL         "initial"
#define GRAPHML_CYB_GRAPH_VERTEX_FINAL           "final"
#define GRAPHML_CYB_GRAPH_VERTEX_CHOICE          "choice"
#define GRAPHML_CYB_GRAPH_VERTEX_TERMINATE       "terminate"
#define GRAPHML_CYB_GRAPH_VERTEX_SHALLOW_HISTORY "shallowHistory"
#define GRAPHML_CYB_GRAPH_VERTEX_DEEP_HISTORY    "deepHistory"
#define GRAPHML_CYB_GRAPH_VERTEX_ENTRY_POINT     "entryPoint"
#define GRAPHML_CYB_GRAPH_VERTEX_EXIT_POINT      "exitPoint"
#define GRAPHML_CYB_GRAPH_VERTEX_FORK            "fork"
#define GRAPHML_CYB_GRAPH_VERTEX_JOIN            "join"

#define GRAPHML_CYB_COMMENT_FORMAL               "formal"
#define GRAPHML_CYB_COMMENT_INFORMAL             "informal"

/* YED format constants */

#define GRAPHML_NAMESPACE_URI_YED			     "http://www.yworks.com/xml/graphml"
#define GRAPHML_BERLOGA_SCHEMENAME_ATTR 	     "SchemeName"
#define GRAPHML_YED_YFILES_TYPE_ATTR             "yfiles.type"
#define GRAPHML_YED_GEOMETRYNODE			     "Geometry"
#define GRAPHML_YED_BORDERSTYLENODE			     "BorderStyle"
#define GRAPHML_YED_LINESTYLENODE			     "LineStyle"
#define GRAPHML_YED_FILLNODE			         "Fill"
#define GRAPHML_YED_PATHNODE				     "Path"
#define GRAPHML_YED_POINTNODE				     "Point"
#define GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE	     "sx"
#define GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE	     "sy"
#define GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE	     "tx"
#define GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE	     "ty"
#define GRAPHML_YED_COMMENTNODE				     "UMLNoteNode"
#define GRAPHML_YED_GROUPNODE				     "GroupNode"
#define GRAPHML_YED_GENERICNODE				     "GenericNode"
#define GRAPHML_YED_LABELNODE				     "NodeLabel"
#define GRAPHML_YED_NODE_CONFIG_ATTRIBUTE	     "configuration"
#define GRAPHML_YED_NODE_CONFIG_START		     "com.yworks.bpmn.Event"
#define GRAPHML_YED_NODE_CONFIG_START2		     "com.yworks.bpmn.Event.withShadow"
#define GRAPHML_YED_PROPNODE				     "Property"
#define GRAPHML_YED_PROP_VALUE_ATTRIBUTE	     "value"
#define GRAPHML_YED_PROP_VALUE_START		     "EVENT_CHARACTERISTIC_START"
#define GRAPHML_YED_EDGELABEL				     "EdgeLabel"
#define GRAPHML_YED_POLYLINEEDGE                 "PolyLineEdge"

/* HSM format / standard constants */

#define CYBERIADA_FORMAT_CYBERIADAML             "Cyberiada-GraphML-1.0"
#define CYBERIADA_FORMAT_BERLOGA                 "yEd Berloga"
#define CYBERIADA_FORMAT_OSTRANNA                "yEd Ostranna"

/* CybediadaML metadata constants */

#define CYBERIADA_META_NODE_DEFAULT_ID           "nMeta"
#define CYBERIADA_META_NODE_TITLE                "CGML_META"

/* Misc. constants */

#define PSEUDO_NODE_SIZE					     20
#define DEFAULT_NODE_SIZE                        100
#define EMPTY_TITLE                              ""

/* Types & macros for XML/GraphML processing */

#define INDENT_STR      "  "
#define XML_WRITE_OPEN_E(w, e)                         if ((res = xmlTextWriterStartElement(w, (const xmlChar *)e)) < 0) {	\
		                                                   fprintf(stderr, "xml open element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
#define XML_WRITE_OPEN_E_I(w, e, indent)               xmlTextWriterWriteRaw(w, (const xmlChar *)"\n"); \
                                                       for (int _i = 0; _i < indent; _i++) { \
                                                           xmlTextWriterWriteRaw(w, (const xmlChar *)INDENT_STR); \
                                                       } \
                                                       if ((res = xmlTextWriterStartElement(w, (const xmlChar *)e)) < 0) {	\
		                                                   fprintf(stderr, "xml open element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
#define XML_WRITE_OPEN_E_NS_I(w, e, ns, indent)        xmlTextWriterWriteRaw(w, (const xmlChar *)"\n"); \
                                                       for (int _i = 0; _i < indent; _i++) { \
                                                           xmlTextWriterWriteRaw(w, (const xmlChar *)INDENT_STR); \
                                                       } \
                                                       if ((res = xmlTextWriterStartElementNS(w, (const xmlChar *)ns, \
																							  (const xmlChar *)e, NULL)) < 0) { \
		                                                   fprintf(stderr, "xml open element error %d at %s:%d", res, __FILE__, __LINE__); \
														   return CYBERIADA_XML_ERROR; \
													   }

#define XML_WRITE_ATTR(w, a, v)                        if ((res = xmlTextWriterWriteAttribute(w, (const xmlChar *)a, \
																							  (const xmlChar *)v)) < 0) { \
		                                                   fprintf(stderr, "xml attribute error %d at %s:%d", res, __FILE__, __LINE__); \
														   return CYBERIADA_XML_ERROR; \
	                                                   }

#define XML_WRITE_TEXT(w, txt)                         if ((res = xmlTextWriterWriteString(w, (const xmlChar *)txt)) < 0) { \
		                                                   fprintf(stderr, "xml text error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }

#define XML_WRITE_CLOSE_E(w)                           if ((res = xmlTextWriterEndElement(w)) < 0) { \
		                                                   fprintf(stderr, "xml close element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
#define XML_WRITE_CLOSE_E_I(w, indent)                 xmlTextWriterWriteRaw(w, (const xmlChar *)"\n"); \
                                                       for (int _i = 0; _i < indent; _i++) { \
                                                           xmlTextWriterWriteRaw(w, (const xmlChar *)INDENT_STR); \
                                                       } \
                                                       if ((res = xmlTextWriterEndElement(w)) < 0) { \
		                                                   fprintf(stderr, "xml close element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
# define XML_READMEMORY_BASENAME                       "noname.xml"

typedef struct {
	char* attr_id;
	const char* attr_for;
	const char* attr_name;
	const char* attr_type;
	char* extra;
} GraphMLKey;

#define GRAPHML_CYB_KEY_FORMAT                  "gFormat"
#define GRAPHML_CYB_KEY_NAME				    "dName"
#define GRAPHML_CYB_KEY_STATE_MACHINE		    "dStateMachine"
#define GRAPHML_CYB_KEY_SUBMACHINE              "dSubmachineState"
#define GRAPHML_CYB_KEY_DATA				    "dData"
#define GRAPHML_CYB_KEY_COMMENT                 "dNote"
#define GRAPHML_CYB_KEY_COMMENT_SUBJECT         "dPivot"
#define GRAPHML_CYB_KEY_COMMENT_CHUNK           "dChunk"
#define GRAPHML_CYB_KEY_GEOMETRY			    "dGeometry"
#define GRAPHML_CYB_KEY_SOURCE_POINT            "dSourcePoint"
#define GRAPHML_CYB_KEY_TARGET_POINT            "dTargetPoint"
#define GRAPHML_CYB_KEY_LABEL_GEOMETRY          "dLabelGeometry"
#define GRAPHML_CYB_KEY_VERTEX			        "dVertex"
#define GRAPHML_CYB_KEY_MARKUP                  "dMarkup"
#define GRAPHML_CYB_KEY_COLOR			        "dColor"

#define GRAPHML_CYB_KEY_FORMAT_NAME				"format"
#define GRAPHML_CYB_KEY_NAME_NAME				"name"
#define GRAPHML_CYB_KEY_STATE_MACHINE_NAME		"stateMachine"
#define GRAPHML_CYB_KEY_SUBMACHINE_NAME         "submachineState"
#define GRAPHML_CYB_KEY_DATA_NAME				"data"
#define GRAPHML_CYB_KEY_COMMENT_NAME            "note"
#define GRAPHML_CYB_KEY_COMMENT_SUBJECT_NAME    "pivot"
#define GRAPHML_CYB_KEY_COMMENT_CHUNK_NAME      "chunk"
#define GRAPHML_CYB_KEY_GEOMETRY_NAME			"geometry"
#define GRAPHML_CYB_KEY_SOURCE_POINT_NAME       "sourcePoint"
#define GRAPHML_CYB_KEY_TARGET_POINT_NAME       "targetPoint"
#define GRAPHML_CYB_KEY_LABEL_GEOMETRY_NAME     "labelGeometry"
#define GRAPHML_CYB_KEY_VERTEX_NAME			    "vertex"
#define GRAPHML_CYB_KEY_MARKUP_NAME             "markup"
#define GRAPHML_CYB_KEY_COLOR_NAME			    "color"

static GraphMLKey cyberiada_graphml_keys[] = {
	{ GRAPHML_CYB_KEY_FORMAT,          GRAPHML_GRAPHML_ELEMENT, GRAPHML_CYB_KEY_FORMAT_NAME,          "string", NULL },
	{ GRAPHML_CYB_KEY_NAME,            GRAPHML_GRAPH_ELEMENT,   GRAPHML_CYB_KEY_NAME_NAME,            "string", NULL },	
	{ GRAPHML_CYB_KEY_NAME,            GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_NAME_NAME,            "string", NULL },
	{ GRAPHML_CYB_KEY_STATE_MACHINE,   GRAPHML_GRAPH_ELEMENT,   GRAPHML_CYB_KEY_STATE_MACHINE_NAME,   "string", NULL },
	{ GRAPHML_CYB_KEY_SUBMACHINE,      GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_SUBMACHINE_NAME,      "string", NULL },
	{ GRAPHML_CYB_KEY_GEOMETRY,        GRAPHML_GRAPH_ELEMENT,   GRAPHML_CYB_KEY_GEOMETRY_NAME,        NULL,     NULL },
    { GRAPHML_CYB_KEY_GEOMETRY,        GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_GEOMETRY_NAME,        NULL,     NULL },
	{ GRAPHML_CYB_KEY_GEOMETRY,        GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_GEOMETRY_NAME,        NULL,     NULL },
	{ GRAPHML_CYB_KEY_SOURCE_POINT,    GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_SOURCE_POINT_NAME,    NULL,     NULL },
	{ GRAPHML_CYB_KEY_TARGET_POINT,    GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_TARGET_POINT_NAME,    NULL,     NULL },
	{ GRAPHML_CYB_KEY_LABEL_GEOMETRY,  GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_LABEL_GEOMETRY_NAME,  NULL,     NULL },
	{ GRAPHML_CYB_KEY_COMMENT,         GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_COMMENT_NAME,         "string", NULL },
	{ GRAPHML_CYB_KEY_VERTEX,          GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_VERTEX_NAME,          "string", NULL },
	{ GRAPHML_CYB_KEY_DATA,            GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_DATA_NAME,            "string", NULL },
	{ GRAPHML_CYB_KEY_DATA,            GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_DATA_NAME,            "string", NULL },
	{ GRAPHML_CYB_KEY_MARKUP,          GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_MARKUP_NAME,          "string", NULL },
	{ GRAPHML_CYB_KEY_COLOR,           GRAPHML_NODE_ELEMENT,    GRAPHML_CYB_KEY_COLOR_NAME,           "string", NULL },	  
	{ GRAPHML_CYB_KEY_COLOR,           GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_COLOR_NAME,           "string", NULL },
	{ GRAPHML_CYB_KEY_COMMENT_SUBJECT, GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_COMMENT_SUBJECT_NAME, "string", NULL },
	{ GRAPHML_CYB_KEY_COMMENT_CHUNK,   GRAPHML_EDGE_ELEMENT,    GRAPHML_CYB_KEY_COMMENT_CHUNK_NAME,   "string", NULL }
};
static const size_t cyberiada_graphml_keys_count = sizeof(cyberiada_graphml_keys) / sizeof(GraphMLKey); 

#define GRAPHML_YED_KEY_GRAPH_DESCR     "d0"
#define GRAPHML_YED_KEY_PORT_GRAPHICS   "d1"
#define GRAPHML_YED_KEY_PORT_GEOMETRY   "d2"
#define GRAPHML_YED_KEY_PORT_USER_DATA  "d3"
#define GRAPHML_YED_KEY_NODE_URL        "d4"
#define GRAPHML_YED_KEY_NODE_DESCR      "d5"
#define GRAPHML_YED_KEY_NODE_GRAPHICS   "d6"
#define GRAPHML_YED_KEY_GRAPHML_RES     "d7"
#define GRAPHML_YED_KEY_EDGE_URL        "d8"
#define GRAPHML_YED_KEY_EDGE_DESCR      "d9"
#define GRAPHML_YED_KEY_EDGE_GRAPHICS   "d10"

static GraphMLKey yed_graphml_keys[] = {
	{ GRAPHML_YED_KEY_GRAPH_DESCR,    GRAPHML_GRAPH_ELEMENT,   "description", "string", NULL           },
	{ GRAPHML_YED_KEY_PORT_GRAPHICS,  GRAPHML_PORT_ELEMENT,    NULL,          NULL,     "portgraphics" },
	{ GRAPHML_YED_KEY_PORT_GEOMETRY,  GRAPHML_PORT_ELEMENT,    NULL,          NULL,     "portgeometry" },
	{ GRAPHML_YED_KEY_PORT_USER_DATA, GRAPHML_PORT_ELEMENT,    NULL,          NULL,     "portuserdata" },
	{ GRAPHML_YED_KEY_NODE_URL,       GRAPHML_NODE_ELEMENT,    "url",         "string", NULL           },
	{ GRAPHML_YED_KEY_NODE_DESCR,     GRAPHML_NODE_ELEMENT,    "description", "string", NULL           },
	{ GRAPHML_YED_KEY_NODE_GRAPHICS,  GRAPHML_NODE_ELEMENT,    NULL,          NULL,     "nodegraphics" },
	{ GRAPHML_YED_KEY_GRAPHML_RES,    GRAPHML_GRAPHML_ELEMENT, NULL,          NULL,     "resources"    },
	{ GRAPHML_YED_KEY_EDGE_URL,       GRAPHML_EDGE_ELEMENT,    "url",         "string", NULL           },
	{ GRAPHML_YED_KEY_EDGE_DESCR,     GRAPHML_EDGE_ELEMENT,    "description", "string", NULL           },
	{ GRAPHML_YED_KEY_EDGE_GRAPHICS,  GRAPHML_EDGE_ELEMENT,    NULL,          NULL,     "edgegraphics" }
};
static const size_t yed_graphml_keys_count = sizeof(yed_graphml_keys) / sizeof(GraphMLKey); 

/* -----------------------------------------------------------------------------
 * Graph manipulation functions
 * ----------------------------------------------------------------------------- */

typedef int (*CyberiadaGraphHandler)(CyberiadaNode* node,
									 CyberiadaNode* parent);

static int cyberiada_process_graph(CyberiadaNode* root,
								   CyberiadaNode* parent,
								   CyberiadaGraphHandler handler)
{
	int res;
	CyberiadaNode *node;
	for (node = root; node; node = node->next) {
		if ((res = handler(node, parent)) != CYBERIADA_NO_ERROR) {
			return res;
		}
		if (node->children) {
			if ((res = cyberiada_process_graph(node->children,
											   node, handler)) != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML XML processor state machine types
 * ----------------------------------------------------------------------------- */

typedef enum {
	gpsInit = 0,
	gpsGraph,
	gpsNode,
	gpsNodeGeometry,
	gpsNodeTitle,
	gpsNodeAction,
	gpsNodeStart,
	gpsEdge,
	gpsEdgeGeometry,
	gpsEdgeSourcePoint,
	gpsEdgeTargetPoint,
	gpsEdgeLabelGeometry,
	gpsInvalid
} GraphProcessorState;

const char* debug_state_names[] = {
	"Init",
	"Graph",
	"Node",
	"NodeGeometry",
	"NodeTitle",
	"NodeAction",
	"NodeStart",
	"Edge",
	"EdgeGeometry",
	"EdgeSourcePoint",
	"EdgeTargetPoint",
	"EdgeLabelGeometry",
	"Invalid"
};

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML XML processor node stack
 * ----------------------------------------------------------------------------- */

typedef CyberiadaStack NodeStack;
/* key  - const char* xml_element
   data - CyberiadaNode* node */

static int node_stack_push(NodeStack** stack)
{
	cyberiada_stack_push(stack);
	return CYBERIADA_NO_ERROR;
}

static int node_stack_set_top_node(NodeStack** stack, CyberiadaNode* node)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_update_top_data((CyberiadaStack**)stack, (void*)node);
	return CYBERIADA_NO_ERROR;
}

static int node_stack_set_top_element(NodeStack** stack, const char* element)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_update_top_key(stack, element);
	return CYBERIADA_NO_ERROR;	
}

static CyberiadaNode* node_stack_current_node(NodeStack** stack)
{
	return cyberiada_stack_get_top_data(stack);
}

static int node_stack_pop(NodeStack** stack)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_pop(stack);
	return CYBERIADA_NO_ERROR;
}

static int node_stack_empty(NodeStack** stack)
{
	return cyberiada_stack_is_empty(stack);
}

static int node_stack_free(NodeStack** stack)
{
	return cyberiada_stack_free(stack);
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML XML reader functions
 * ----------------------------------------------------------------------------- */

static int cyberiada_has_attr(xmlNode* node, const char* attrname)
{
	xmlAttr* attribute = node->properties;
	while(attribute) {
		if (strcmp((const char*)attribute->name, attrname) == 0) {
			return CYBERIADA_NO_ERROR;
		}
		attribute = attribute->next;
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_get_attr_value(char* buffer, size_t buffer_len,
									xmlNode* node, const char* attrname)
{
	xmlAttr* attribute = node->properties;
	while(attribute) {
		if (strcmp((const char*)attribute->name, attrname) == 0) {
			xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
			strncpy(buffer, (char*)value, buffer_len);
			xmlFree(value);
			return CYBERIADA_NO_ERROR;
		}
		attribute = attribute->next;
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_get_element_text(char* buffer, size_t buffer_len,
									  xmlNode* node)
{
	xmlChar* value = xmlNodeListGetString(node->doc,
										  node->xmlChildrenNode,
										  1);
	if (value) {
		strncpy(buffer, (char*)value, buffer_len);
		xmlFree(value);
	} else {
		buffer[0] = 0;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_xml_read_coord(xmlNode* xml_node,
									const char* attr_name,
									double* result)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 attr_name) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_BAD_PARAMETER;
	}
	*result = (double)atof(buffer);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_xml_read_point(xmlNode* xml_node,
									CyberiadaPoint** point)
{
	CyberiadaPoint* p = htree_new_point();
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_X_ATTRIBUTE,
								 &(p->x))) {
		p->x = 0.0;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_Y_ATTRIBUTE,
								 &(p->y)) != CYBERIADA_NO_ERROR) {
		p->y = 0.0;
	}
	*point = p;
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_xml_read_rect(xmlNode* xml_node,
								   CyberiadaRect** rect)
{
	CyberiadaRect* r = htree_new_rect();
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_X_ATTRIBUTE,
								 &(r->x)) != CYBERIADA_NO_ERROR) {
		r->x = 0.0;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_Y_ATTRIBUTE,
								 &(r->y)) != CYBERIADA_NO_ERROR) {
		r->y = 0.0;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_WIDTH_ATTRIBUTE,
								 &(r->width)) != CYBERIADA_NO_ERROR) {
		r->width = 0.0;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_GEOM_HEIGHT_ATTRIBUTE,
								 &(r->height)) != CYBERIADA_NO_ERROR) {
		r->height = 0.0;
	}
	*rect = r;
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * Common handlers for the GraphML processor 
 * ----------------------------------------------------------------------------- */

static GraphProcessorState handle_new_graph(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack,
											CyberiadaRegexps* regexps)
{
	(void)regexps; /* unused parameter */
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaSM* sm = doc->state_machines;
	CyberiadaNode* parent = node_stack_current_node(stack);
	/* process the top graph element only */
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	/* DEBUG("found graph %s \n", buffer); */
	while (sm->next) sm = sm->next;
	if (parent == NULL) {
		if (sm->nodes != NULL) {
			sm->next = cyberiada_new_sm();
			sm = sm->next;
		}
		sm->nodes = cyberiada_new_node(buffer);
		sm->nodes->type = cybNodeSM;
		node_stack_set_top_node(stack, sm->nodes);
	}
	return gpsGraph;
}

static GraphProcessorState handle_new_node(xmlNode* xml_node,	
										   CyberiadaDocument* doc,
										   NodeStack** stack,
										   CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */
	(void)regexps; /* unused parameter */

	CyberiadaNode* node;	
	CyberiadaNode* parent;	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	/* DEBUG("found node %s\n", buffer); */
	parent = node_stack_current_node(stack);
	if (parent == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	node = cyberiada_new_node(buffer);
	node->parent = parent;
	node_stack_set_top_node(stack, node);
	if (parent->children == NULL) {
		parent->children = node;
	} else {
		cyberiada_graph_add_sibling_node(parent->children, node);
	}
	return gpsNode;
}

static GraphProcessorState handle_new_edge(xmlNode* xml_node,
										   CyberiadaDocument* doc,
										   NodeStack** stack,
										   CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	char source_buffer[MAX_STR_LEN];
	size_t source_buffer_len = sizeof(source_buffer) - 1;
	char target_buffer[MAX_STR_LEN];
	size_t target_buffer_len = sizeof(target_buffer) - 1;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	if(cyberiada_get_attr_value(source_buffer, source_buffer_len,
								xml_node,
								GRAPHML_SOURCE_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if(cyberiada_get_attr_value(target_buffer, target_buffer_len,
								xml_node,
								GRAPHML_TARGET_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		buffer[0] = 0;
	}
	/* DEBUG("add edge '%s' '%s' -> '%s'\n", buffer, source_buffer, target_buffer); */
	cyberiada_graph_add_edge(sm, buffer, source_buffer, target_buffer, 0);
	return gpsEdge;
}

static GraphProcessorState handle_edge_point(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack,
											 CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaEdge *current;
	CyberiadaPoint* p;
	CyberiadaPolyline *pl, *last_pl;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node, &p) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	pl = htree_new_polyline();
	pl->point.x = p->x;
	pl->point.y = p->y;
	htree_destroy_point(p);
	if (current->geometry_polyline == NULL) {
		current->geometry_polyline = pl;
	} else {
		last_pl = current->geometry_polyline;
		while (last_pl->next) last_pl = last_pl->next;
		last_pl->next = pl;
	}
	return gpsEdgeGeometry;
}

/* -----------------------------------------------------------------------------
 * YED-specific handlers for the GraphML processor 
 * ----------------------------------------------------------------------------- */

static GraphProcessorState handle_group_node(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack,
											 CyberiadaRegexps* regexps)
{
	(void)xml_node; /* unused parameter */	
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	current->type = cybNodeCompositeState;
	return gpsNodeGeometry;
}

static GraphProcessorState handle_comment_node(xmlNode* xml_node,
											   CyberiadaDocument* doc,
											   NodeStack** stack,
											   CyberiadaRegexps* regexps)
{
	(void)xml_node; /* unused parameter */	
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	current->type = cybNodeComment;
	/* DEBUG("Set node type comment\n"); */
	/*cyberiada_copy_string(&(current->title), &(current->title_len), COMMENT_TITLE);*/
	return gpsNodeGeometry;
}

static GraphProcessorState handle_generic_node(xmlNode* xml_node,
											   CyberiadaDocument* doc,
											   NodeStack** stack,
											   CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_YED_NODE_CONFIG_ATTRIBUTE) == CYBERIADA_NO_ERROR &&
		(strcmp(buffer, GRAPHML_YED_NODE_CONFIG_START) == 0 ||
		 strcmp(buffer, GRAPHML_YED_NODE_CONFIG_START2) == 0)) {
		current->type = cybNodeInitial;
		if (current->title != NULL) {
			ERROR("Trying to set start node %s label twice\n", current->id);
			return gpsInvalid;
		}
		cyberiada_copy_string(&(current->title), &(current->title_len), "");
	} else {
		current->type = cybNodeSimpleState;
	}
	return gpsNodeGeometry;
}

static GraphProcessorState handle_node_geometry(xmlNode* xml_node,
												CyberiadaDocument* doc,
												NodeStack** stack,
												CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaNodeType type;
	CyberiadaNode* current = node_stack_current_node(stack);
	CyberiadaRect* rect;
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	type = current->type;
	if (cyberiada_xml_read_rect(xml_node,
								&rect) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if (type == cybNodeInitial || type == cybNodeFinal) {
		current->geometry_point = htree_new_point();
		current->geometry_point->x = rect->x + rect->width / 2.0;
		current->geometry_point->y = rect->y + rect->height / 2.0;
		htree_destroy_rect(rect);
		return gpsNodeStart;
	} else {
		if (rect->width == 0.0 && rect->height == 0.0) {
			/* rect with zero width & height is empty actually */
			htree_destroy_rect(rect);
			current->geometry_rect = NULL;
		} else {
			current->geometry_rect = rect;
		}
		if (type == cybNodeComment) {					
			return gpsNodeAction;
		} else {
			return gpsNodeTitle;
		}
	}
}

static GraphProcessorState handle_property(xmlNode* xml_node,
										   CyberiadaDocument* doc,
										   NodeStack** stack,
										   CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_YED_PROP_VALUE_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if (strcmp(buffer, GRAPHML_YED_PROP_VALUE_START) == 0) {
		return gpsGraph;
	}
	return gpsNodeStart;
}

static GraphProcessorState handle_node_title(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack,
											 CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */
	(void)regexps; /* unused parameter */		
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	if (current->title != NULL) {
		ERROR("Trying to set node %s label twice\n", current->id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	/* DEBUG("Set node %s title '%s'\n", current->id, buffer); */
	cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
	cyberiada_string_trim(current->title);
	/* DEBUG("After trim: '%s'\n", current->title); */
	return gpsNodeAction;
}

static GraphProcessorState handle_node_action(xmlNode* xml_node,
											  CyberiadaDocument* doc,
											  NodeStack** stack,
											  CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	if (current->actions != NULL) {
		ERROR("Trying to set node %s actions twice\n", current->id);
		return gpsInvalid;
	}
	if (current->type == cybNodeComment) {
		/* DEBUG("Set node %s comment text %s\n", current->id, buffer); */
		if (current->comment_data != NULL) {
			if (current->comment_data->body) {
				ERROR("Trying to set node %s body twice\n", current->id);
				return gpsInvalid;
			}
		} else {
			current->comment_data = cyberiada_new_comment_data();
		}
		cyberiada_copy_string(&(current->comment_data->body),
							  &(current->comment_data->body_len), buffer);
	} else {
		/* DEBUG("Set node %s action %s\n", current->id, buffer); */
		if (cyberiada_decode_state_actions_yed(buffer, &(current->actions), regexps) != CYBERIADA_NO_ERROR) {
			ERROR("cannot decode yed node action\n");
			return gpsInvalid;
		}
	}
	return gpsGraph;
}

static GraphProcessorState handle_edge_geometry(xmlNode* xml_node,
												CyberiadaDocument* doc,
												NodeStack** stack,
												CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	current->geometry_source_point = htree_new_point();
	current->geometry_target_point = htree_new_point();
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE,
								 &(current->geometry_source_point->x)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE,
								 &(current->geometry_source_point->y)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE,
								 &(current->geometry_target_point->x)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE,
								 &(current->geometry_target_point->y)) != CYBERIADA_NO_ERROR) {
		htree_destroy_point(current->geometry_source_point);
		htree_destroy_point(current->geometry_target_point);
		current->geometry_source_point = NULL;
		current->geometry_target_point = NULL;
		return gpsInvalid;
	}
	return gpsEdgeGeometry;
}

static GraphProcessorState handle_edge_label(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack,
											 CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	double x = 0.0, y = 0.0;
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->action != NULL) {
		ERROR("Trying to set edge %s:%s label twice\n",
			  current->source_id, current->target_id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	/* DEBUG("add edge %s:%s action %s\n",
	   current->source_id, current->target_id, buffer); */
	if (cyberiada_decode_edge_action(buffer, &(current->action), regexps) != CYBERIADA_NO_ERROR) {
		ERROR("cannot decode edge action\n");
		return gpsInvalid;
	}
	if (current->action &&
		cyberiada_has_attr(xml_node, GRAPHML_GEOM_X_ATTRIBUTE) == CYBERIADA_NO_ERROR &&
		cyberiada_has_attr(xml_node, GRAPHML_GEOM_Y_ATTRIBUTE) == CYBERIADA_NO_ERROR) {
		
		if (cyberiada_xml_read_coord(xml_node, GRAPHML_GEOM_X_ATTRIBUTE, &x) == CYBERIADA_NO_ERROR &&
			cyberiada_xml_read_coord(xml_node, GRAPHML_GEOM_Y_ATTRIBUTE, &y) == CYBERIADA_NO_ERROR) {
			
			if (current->geometry_label_point) {
				ERROR("Trying to set edge %s:%s label coordinates twice\n",
					  current->source_id, current->target_id);
				return gpsInvalid;
			}
		
			current->geometry_label_point = htree_new_point();
			current->geometry_label_point->x = x;
			current->geometry_label_point->y = y;
		}
	}
	return gpsGraph;
}

/* -----------------------------------------------------------------------------
 * CyberiadaML-specific handlers for the GraphML processor 
 * ----------------------------------------------------------------------------- */

typedef struct {
	const char*       name;
	CyberiadaNodeType type;
} CyberidaVertex;

CyberidaVertex cyberiada_vertexes[] = {
	{ GRAPHML_CYB_GRAPH_VERTEX_INITIAL,         cybNodeInitial },
	{ GRAPHML_CYB_GRAPH_VERTEX_FINAL,           cybNodeFinal },
	{ GRAPHML_CYB_GRAPH_VERTEX_CHOICE,          cybNodeChoice },
	{ GRAPHML_CYB_GRAPH_VERTEX_TERMINATE,       cybNodeTerminate },
	{ GRAPHML_CYB_GRAPH_VERTEX_SHALLOW_HISTORY, cybNodeShallowHistory },
	{ GRAPHML_CYB_GRAPH_VERTEX_DEEP_HISTORY,    cybNodeDeepHistory },
	{ GRAPHML_CYB_GRAPH_VERTEX_ENTRY_POINT,     cybNodeEntryPoint },
	{ GRAPHML_CYB_GRAPH_VERTEX_EXIT_POINT,      cybNodeExitPoint },
	{ GRAPHML_CYB_GRAPH_VERTEX_FORK,            cybNodeFork },
	{ GRAPHML_CYB_GRAPH_VERTEX_JOIN,            cybNodeJoin },
};
static const size_t cyberiada_vertexes_count = sizeof(cyberiada_vertexes) / sizeof(CyberidaVertex); 

static const char* cyberiada_init_table_find_id(const char* element, const char* name, size_t* index)
{
	size_t i;
	for (i = 0; i < cyberiada_graphml_keys_count; i++ ) {
		if (strcmp(cyberiada_graphml_keys[i].attr_name, name) == 0 &&
			strcmp(cyberiada_graphml_keys[i].attr_for, element) == 0) {
			if (index) {
				*index = i;
			}
			return cyberiada_graphml_keys[i].attr_id;
		}
	}
	return NULL;
}

static const char* cyberiada_init_table_find_name(const char* id)
{
	size_t i;
	for (i = 0; i < cyberiada_graphml_keys_count; i++ ) {
		if (strcmp(cyberiada_graphml_keys[i].attr_id, id) == 0) {
			return cyberiada_graphml_keys[i].attr_name;
		}
	}
	return NULL;	
}

static void cyberiada_init_table_free_extensitions(void)
{
	size_t i;
	for (i = 0; i < cyberiada_graphml_keys_count; i++ ) {
		if (cyberiada_graphml_keys[i].extra) {
			free(cyberiada_graphml_keys[i].attr_id);
			cyberiada_graphml_keys[i].attr_id = cyberiada_graphml_keys[i].extra;
			cyberiada_graphml_keys[i].extra = NULL;
		}
	}
}

static GraphProcessorState handle_new_init_data(xmlNode* xml_node,
												CyberiadaDocument* doc,
												NodeStack** stack,
												CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	const char* format_name; 
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_KEY_ATTRIBUTE) == CYBERIADA_NO_ERROR) {
		format_name = cyberiada_init_table_find_name(buffer);
		if (format_name == NULL) {
			ERROR("cannot find format key with id %s\n", buffer);
			return gpsInvalid;
		}
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		if (strcmp(buffer, CYBERIADA_FORMAT_CYBERIADAML) == 0) {
			cyberiada_copy_string(&(doc->format), &(doc->format_len),
								  CYBERIADA_FORMAT_CYBERIADAML);
			/* DEBUG("doc format %s\n", doc->format); */
			return gpsInit;
		} else {
			ERROR("Bad Cyberida-GraphML format: %s\n", buffer);
		}
	} else {
		ERROR("No graph version node\n");
	}
	return gpsInvalid;
}

static GraphProcessorState handle_new_init_key(xmlNode* xml_node,
											   CyberiadaDocument* doc,
											   NodeStack** stack,
											   CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	char *attr_id = NULL, *attr_for = NULL, *attr_name = NULL;
	const char *table_id;
	size_t index;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_FOR_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInit;
	}
	cyberiada_copy_string(&attr_for, NULL, buffer);
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_NAME_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		free(attr_for);
		return gpsInit;
	}
	cyberiada_copy_string(&attr_name, NULL, buffer);
	table_id = cyberiada_init_table_find_id(attr_for, attr_name, &index);
	if (table_id) {	
		if (cyberiada_get_attr_value(buffer, buffer_len,
									 xml_node,
									 GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
			ERROR("Cannot find 'id' attribute of the key node\n");
			free(attr_for);
			free(attr_name);
			return gpsInvalid;
		}
		cyberiada_copy_string(&attr_id, NULL, buffer);
		if (strcmp(table_id, attr_id) != 0) {
			cyberiada_graphml_keys[index].extra = cyberiada_graphml_keys[index].attr_id; /* save as modification flag */
			cyberiada_graphml_keys[index].attr_id = attr_id;
		} else {
			free(attr_id);
		}
	}
	free(attr_for);
	free(attr_name);
	return gpsInit;
}

static GraphProcessorState handle_node_data(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack,
											CyberiadaRegexps* regexps)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
	const char* key_name;
	size_t i;
	int found; 
	if (current == NULL) {
		ERROR("no current node\n");
		return gpsInvalid;
	}
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_KEY_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		ERROR("no data node key attribute\n");
		return gpsInvalid;
	}
	key_name = cyberiada_init_table_find_name(buffer);
	if (key_name == NULL) {
		ERROR("cannot find key with id %s\n", buffer);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	/* DEBUG("node id %s data key '%s' value '%s'\n", current->id, key_name, buffer); */
	if (strcmp(key_name, GRAPHML_CYB_KEY_NAME_NAME) == 0) {
		if (current->title != NULL) {
			ERROR("Trying to set node %s label twice\n", current->id);
			return gpsInvalid;
		}
		/* DEBUG("Set node %s title %s\n", current->id, buffer); */
		cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
		cyberiada_string_trim(current->title);
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_STATE_MACHINE_NAME) == 0) {
		if (current->type != cybNodeSM) {
			ERROR("Using state machine key outside the graph element in %s\n", current->id);
			return gpsInvalid;
		}
		return gpsGraph;
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_DATA_NAME) == 0) {
		if (current->actions != NULL) {
			ERROR("Trying to set comment node %s action\n", current->id);
			return gpsInvalid;
		}
		if (current->type == cybNodeComment || current->type == cybNodeFormalComment) {
			if (current->comment_data != NULL) {
				if (current->comment_data->body) {
					ERROR("Trying to set node %s body twice\n", current->id);
					return gpsInvalid;
				}
			} else {
				current->comment_data = cyberiada_new_comment_data();
			}
			cyberiada_copy_string(&(current->comment_data->body),
								  &(current->comment_data->body_len), buffer);
			if (current->type == cybNodeFormalComment &&
				current->title && strcmp(current->title, CYBERIADA_META_NODE_TITLE) == 0) {
				if (cyberiada_decode_meta(doc, buffer, regexps) != CYBERIADA_NO_ERROR) {
					ERROR("Error while decoging metainfo comment\n");
					return gpsInvalid;
				}
			}
		} else {
			/* DEBUG("Set node %s action %s\n", current->id, buffer); */
			if (cyberiada_decode_state_actions(buffer, &(current->actions), regexps) != CYBERIADA_NO_ERROR) {
				ERROR("Cannot decode cyberiada node action\n");
				return gpsInvalid;
			}
		}
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_VERTEX_NAME) == 0) {
		if (current->actions != NULL) {
			ERROR("Trying to set the vertex %s action\n", current->id);
			return gpsInvalid;
		}
		found = 0;
		for (i = 0; i < cyberiada_vertexes_count; i++) {
			if (strcmp(buffer, cyberiada_vertexes[i].name) == 0) {
				current->type = cyberiada_vertexes[i].type;
				found = 1;
				break;
			}
		}
		if (!found) {
			ERROR("Unknown vertex type '%s'\n", buffer);
			return gpsInvalid;
		}
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_COMMENT_NAME) == 0) {
		if (strcmp(buffer, GRAPHML_CYB_COMMENT_FORMAL) == 0) {
			current->type = cybNodeFormalComment;
		} else if (strcmp(buffer, GRAPHML_CYB_COMMENT_INFORMAL) == 0 ||
				   cyberiada_string_is_empty(buffer)) { /* default */
			current->type = cybNodeComment;
		} else {
			ERROR("Bad comment type '%s'\n", buffer);
			return gpsInvalid;
		}
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_COLOR_NAME) == 0) {
		if (current->color) {
			ERROR("Trying to set node %s color twice\n", current->id);
			return gpsInvalid;
		}
		cyberiada_copy_string(&(current->color), &(current->color_len), buffer);
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_MARKUP_NAME) == 0) {
		if (current->type != cybNodeComment) {
			ERROR("Trying to set markup for non-comment node %s\n", current->id);
			return gpsInvalid;
		}
		if (current->comment_data != NULL) {
			if (current->comment_data->markup) {
				ERROR("Trying to set node %s markup twice\n", current->id);
				return gpsInvalid;
			}
		} else {
			current->comment_data = cyberiada_new_comment_data();
		}
		cyberiada_copy_string(&(current->comment_data->markup),
							  &(current->comment_data->markup_len), buffer);
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_SUBMACHINE_NAME) == 0) {
		if (current->link != NULL) {
			ERROR("Trying to set submachine node %s link twice\n", current->id);
			return gpsInvalid;
		}
		if (cyberiada_string_is_empty(buffer)) {
			ERROR("Empty link in the submachine state node %s\n", current->id);
			return gpsInvalid;
		}
		current->link = cyberiada_new_link(buffer);
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_GEOMETRY_NAME) == 0) {
		return gpsNodeGeometry;
	} else {
		ERROR("Bad data key attribute '%s'\n", key_name);
		return gpsInvalid;
	}
	return gpsNode;
}

static GraphProcessorState handle_node_point(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack,
											 CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("no current node\n");
		return gpsInvalid;
	}
	if (current->geometry_point) {
		ERROR("Trying to set node %s geometry point twice\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node,
								 &(current->geometry_point)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsNode;
}

static GraphProcessorState handle_node_rect(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack,
											CyberiadaRegexps* regexps)
{
	(void)doc; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("no current node\n");
		return gpsInvalid;
	}
	if (current->geometry_rect) {
		ERROR("Trying to set node %s geometry rect twice\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_rect(xml_node,
							   &(current->geometry_rect)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if (current->geometry_rect->width == 0.0 && current->geometry_rect->height == 0.0) {
		/* rect with zero width & height is empty actually */
		htree_destroy_rect(current->geometry_rect);
		current->geometry_rect = NULL;
	}
	return gpsNode;
}

static GraphProcessorState handle_edge_data(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack,
											CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaEdge *current;
	const char* key_name;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (cyberiada_get_attr_value(buffer, buffer_len, xml_node,
								 GRAPHML_KEY_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		ERROR("no data node key attribute\n");
		return gpsInvalid;
	}
	key_name = cyberiada_init_table_find_name(buffer);
	if (key_name == NULL) {
		ERROR("cannot find key with id %s\n", buffer);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	if (strcmp(key_name, GRAPHML_CYB_KEY_DATA_NAME) == 0) {
		if (current->action != NULL) {
			ERROR("Trying to set edge %s action twice\n", current->id);
			return gpsInvalid;
		}
		/* DEBUG("Set edge %s action %s\n", current->id, buffer); */
		if (cyberiada_decode_edge_action(buffer, &(current->action), regexps) != CYBERIADA_NO_ERROR) {
			ERROR("cannot decode edge action\n");
			return gpsInvalid;
		}
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_GEOMETRY_NAME) == 0) {
		return gpsEdgeGeometry;
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_SOURCE_POINT_NAME) == 0) {
		return gpsEdgeSourcePoint;
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_TARGET_POINT_NAME) == 0) {
		return gpsEdgeTargetPoint;
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_LABEL_GEOMETRY_NAME) == 0) {
		return gpsEdgeLabelGeometry;
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_COLOR_NAME) == 0) {
		cyberiada_copy_string(&(current->color), &(current->color_len), buffer);
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_COMMENT_SUBJECT_NAME) == 0) {
		if (current->comment_subject) {
			ERROR("Trying to set edge %s comment subject twice\n", current->id);
			return gpsInvalid;
		}
		current->type = cybEdgeComment;
		if (cyberiada_string_is_empty(buffer)) {
			current->comment_subject = cyberiada_new_comment_subject(cybCommentSubjectNode);
		} else {
			key_name = cyberiada_init_table_find_name(buffer);
			if (key_name == NULL) {
				ERROR("cannot find pivot key with id %s\n", buffer);
				return gpsInvalid;
			}
			if (strcmp(key_name, GRAPHML_CYB_KEY_NAME_NAME) == 0) {
				current->comment_subject = cyberiada_new_comment_subject(cybCommentSubjectNameFragment);
			} else if (strcmp(key_name, GRAPHML_CYB_KEY_DATA_NAME) == 0) {
				current->comment_subject = cyberiada_new_comment_subject(cybCommentSubjectDataFragment);
			} else {
				ERROR("Unsupported edge comment subject type %s\n", key_name);
				return gpsInvalid;
			}
		}
	} else if (strcmp(key_name, GRAPHML_CYB_KEY_COMMENT_CHUNK_NAME) == 0) {
		if (!current->comment_subject) {
			ERROR("Edge %s comment subject is empty\n", current->id);
			return gpsInvalid;
		}
		if (current->comment_subject->type == cybCommentSubjectNameFragment ||
			current->comment_subject->type == cybCommentSubjectDataFragment) {
			if (current->comment_subject->fragment) {
				ERROR("Trying to set edge %s comment subject fragent twice", current->id);
				return gpsInvalid;
			}
			cyberiada_copy_string(&(current->comment_subject->fragment),
								  &(current->comment_subject->fragment_len),
								  buffer);
		}
	} else {
		ERROR("bad data key attribute %s\n", key_name);
		return gpsInvalid;
	}
	return gpsEdge;
}

static GraphProcessorState handle_edge_source_point(xmlNode* xml_node,
													CyberiadaDocument* doc,
													NodeStack** stack,
													CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->geometry_source_point) {
		ERROR("Trying to set edge %s source point twice\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node,
								 &(current->geometry_source_point)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdge;
}

static GraphProcessorState handle_edge_target_point(xmlNode* xml_node,
													CyberiadaDocument* doc,
													NodeStack** stack,
													CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */
	(void)regexps; /* unused parameter */
	
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->geometry_target_point) {
		ERROR("Trying to set edge %s target point twice\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node,
								 &(current->geometry_target_point)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdge;
}

static GraphProcessorState handle_edge_label_point(xmlNode* xml_node,
												   CyberiadaDocument* doc,
												   NodeStack** stack,
												   CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->geometry_label_point || current->geometry_label_rect) {
		ERROR("Trying to set edge %s label geometry twice (point)\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node,
								 &(current->geometry_label_point)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdge;
}

static GraphProcessorState handle_edge_label_rect(xmlNode* xml_node,
												  CyberiadaDocument* doc,
												  NodeStack** stack,
												  CyberiadaRegexps* regexps)
{
	(void)stack; /* unused parameter */	
	(void)regexps; /* unused parameter */	
	
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->geometry_label_point || current->geometry_label_rect) {
		ERROR("Trying to set edge %s label geometry twice (rect)\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_rect(xml_node,
								&(current->geometry_label_rect)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdge;
}

typedef GraphProcessorState (*GraphProcessorHandler)(xmlNode* xml_root,
													 CyberiadaDocument* doc,
													 NodeStack** stack,
													 CyberiadaRegexps* regexps);

typedef struct {
	GraphProcessorState		state;
	const char* 			symbol;
	GraphProcessorHandler	handler;
} ProcessorTransition;

static ProcessorTransition cyb_processor_state_table[] = {
	{gpsInit,              GRAPHML_DATA_ELEMENT,  &handle_new_init_data},
	{gpsInit,              GRAPHML_KEY_ELEMENT,   &handle_new_init_key},
	{gpsInit,              GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsGraph,             GRAPHML_NODE_ELEMENT,  &handle_new_node},
	{gpsGraph,             GRAPHML_EDGE_ELEMENT,  &handle_new_edge},
	{gpsGraph,             GRAPHML_DATA_ELEMENT,  &handle_node_data},
	{gpsNode,              GRAPHML_DATA_ELEMENT,  &handle_node_data},
	{gpsNode,              GRAPHML_NODE_ELEMENT,  &handle_new_node},
	{gpsNode,              GRAPHML_EDGE_ELEMENT,  &handle_new_edge},
	{gpsNode,              GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsEdge,              GRAPHML_DATA_ELEMENT,  &handle_edge_data},
	{gpsEdge,              GRAPHML_EDGE_ELEMENT,  &handle_new_edge},
	{gpsEdge,              GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsNodeGeometry,      GRAPHML_POINT_ELEMENT, &handle_node_point},
	{gpsNodeGeometry,      GRAPHML_RECT_ELEMENT,  &handle_node_rect},
	{gpsEdgeGeometry,      GRAPHML_POINT_ELEMENT, &handle_edge_point},
	{gpsEdgeGeometry,      GRAPHML_DATA_ELEMENT,  &handle_edge_data},
	{gpsEdgeGeometry,      GRAPHML_EDGE_ELEMENT,  &handle_new_edge},
	{gpsEdgeGeometry,      GRAPHML_GRAPH_ELEMENT, &handle_new_graph},	
	{gpsEdgeSourcePoint,   GRAPHML_POINT_ELEMENT, &handle_edge_source_point},
	{gpsEdgeTargetPoint,   GRAPHML_POINT_ELEMENT, &handle_edge_target_point},
	{gpsEdgeLabelGeometry, GRAPHML_POINT_ELEMENT, &handle_edge_label_point},
	{gpsEdgeLabelGeometry, GRAPHML_RECT_ELEMENT,  &handle_edge_label_rect},
};
const size_t cyb_processor_state_table_size = sizeof(cyb_processor_state_table) / sizeof(ProcessorTransition);

static ProcessorTransition yed_processor_state_table[] = {
	{gpsInit,         GRAPHML_GRAPH_ELEMENT,    &handle_new_graph},
	{gpsGraph,        GRAPHML_NODE_ELEMENT,     &handle_new_node},
	{gpsGraph,        GRAPHML_EDGE_ELEMENT,     &handle_new_edge},
	{gpsGraph,        GRAPHML_GRAPH_ELEMENT,    &handle_new_graph},
	{gpsNode,         GRAPHML_YED_COMMENTNODE,  &handle_comment_node},
	{gpsNode,         GRAPHML_YED_GROUPNODE,    &handle_group_node},
	{gpsNode,         GRAPHML_YED_GENERICNODE,  &handle_generic_node},
	{gpsNodeGeometry, GRAPHML_YED_GEOMETRYNODE, &handle_node_geometry},
	{gpsNodeStart,    GRAPHML_YED_PROPNODE,     &handle_property},
	{gpsNodeStart,    GRAPHML_NODE_ELEMENT,     &handle_new_node},
	{gpsNodeTitle,    GRAPHML_YED_LABELNODE,    &handle_node_title},
	{gpsNodeAction,   GRAPHML_YED_LABELNODE,    &handle_node_action},
	{gpsNodeAction,   GRAPHML_NODE_ELEMENT,     &handle_new_node},
	{gpsEdge,         GRAPHML_EDGE_ELEMENT,     &handle_new_edge},
	{gpsEdge,         GRAPHML_YED_PATHNODE,     &handle_edge_geometry},
	{gpsEdgeGeometry, GRAPHML_YED_POINTNODE,    &handle_edge_point},
	{gpsEdgeGeometry, GRAPHML_YED_EDGELABEL,    &handle_edge_label},
	{gpsEdgeGeometry,  GRAPHML_EDGE_ELEMENT,    &handle_new_edge},
};
const size_t yed_processor_state_table_size = sizeof(yed_processor_state_table) / sizeof(ProcessorTransition);

static int dispatch_processor(xmlNode* xml_node,
							  CyberiadaDocument* doc,
							  NodeStack** stack,
							  GraphProcessorState* gps,
							  ProcessorTransition* processor_state_table,
							  size_t processor_state_table_size,
							  CyberiadaRegexps* regexps)
{
	size_t i;
	if (xml_node->type == XML_ELEMENT_NODE) {
		const char* xml_element_name = (const char*)xml_node->name;
		node_stack_set_top_element(stack, xml_element_name);
		for (i = 0; i < processor_state_table_size; i++) {
			if (processor_state_table[i].state == *gps &&
				strcmp(xml_element_name, processor_state_table[i].symbol) == 0) {
				*gps = (*(processor_state_table[i].handler))(xml_node, doc, stack, regexps);
				return CYBERIADA_NO_ERROR;
			}
		}
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_build_graphs(xmlNode* xml_root,
								  CyberiadaDocument* doc,
								  NodeStack** stack,
								  GraphProcessorState* gps,
								  ProcessorTransition* processor_state_table,
								  size_t processor_state_table_size,
								  CyberiadaRegexps* regexps)
{
	xmlNode *cur_xml_node = NULL;
	/* NodeStack* s = *stack; */
	/* DEBUG("\nStack:"); */
	/* while (s) { */
	/* 	DEBUG(" (%s %p)", */
	/* 		  s->xml_element ? s->xml_element : "null", */
	/* 		  s->node); */
	/* 	s = s->next; */
	/* } */
	/* DEBUG("\n"); */
	for (cur_xml_node = xml_root; cur_xml_node; cur_xml_node = cur_xml_node->next) {
		/* DEBUG("xml node %s sm root %s gps %s\n",
			  cur_xml_node->name,
			  sm->nodes ? sm->nodes->id : "none",
			  debug_state_names[*gps]); */
		node_stack_push(stack);
		dispatch_processor(cur_xml_node, doc, stack, gps,
						   processor_state_table, processor_state_table_size,
						   regexps);
		if (*gps == gpsInvalid) {
			return CYBERIADA_FORMAT_ERROR;
		}
		if (cur_xml_node->children) {
			int res = cyberiada_build_graphs(cur_xml_node->children, doc, stack, gps,
											 processor_state_table, processor_state_table_size,
											 regexps);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node_stack_pop(stack);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_yed_xml(xmlNode* root, CyberiadaDocument* doc, CyberiadaRegexps* regexps)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	GraphProcessorState gps = gpsInit;
	CyberiadaNode* node = NULL;
	NodeStack* stack = NULL;
	char berloga_format = 0;
	int res;
	char* sm_name;
	
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 root,
								 GRAPHML_BERLOGA_SCHEMENAME_ATTR) == CYBERIADA_NO_ERROR) {
		cyberiada_copy_string(&(doc->format), &(doc->format_len), CYBERIADA_FORMAT_BERLOGA);
		berloga_format = 1;
		regexps->berloga_legacy = 1;
	} else {
		cyberiada_copy_string(&(doc->format), &(doc->format_len), CYBERIADA_FORMAT_OSTRANNA);
		berloga_format = 0;
	}
	/* DEBUG("doc format %s\n", doc->format); */
	
	if ((res = cyberiada_build_graphs(root, doc, &stack, &gps,
									  yed_processor_state_table,
									  yed_processor_state_table_size,
									  regexps)) != CYBERIADA_NO_ERROR) {
		return res;
	}
	
	if (!node_stack_empty(&stack)) {
		ERROR("error with node stack\n");
		node_stack_free(&stack);
		return CYBERIADA_FORMAT_ERROR;
	}

	if (berloga_format) {
		sm_name = buffer;
	} else {
		if (doc->state_machines->nodes) {
			node = cyberiada_graph_find_node_by_type(doc->state_machines->nodes, cybNodeCompositeState);
		}
		if (node) {
			sm_name = node->title;
		} else {
			sm_name = EMPTY_TITLE;
		}
	}
	cyberiada_add_default_meta(doc, sm_name);
	if (doc->state_machines->nodes && !doc->state_machines->nodes->title) {
		cyberiada_copy_string(&(doc->state_machines->nodes->title),
							  &(doc->state_machines->nodes->title_len),
							  sm_name);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_cyberiada_xml(xmlNode* root, CyberiadaDocument* doc, CyberiadaRegexps* regexps)
{
	GraphProcessorState gps = gpsInit;
	CyberiadaSM* sm;
	NodeStack* stack = NULL;
	int res;
/*	CyberiadaNode *meta_node, *ext_node;
	CyberiadaEdge *edge, *prev_edge;*/
	
	if ((res = cyberiada_build_graphs(root, doc, &stack, &gps,
									  cyb_processor_state_table,
									  cyb_processor_state_table_size,
									  regexps)) != CYBERIADA_NO_ERROR) {
		cyberiada_init_table_free_extensitions();
		return res;
	}

	if (!node_stack_empty(&stack)) {
		ERROR("error with node stack\n");
		cyberiada_stack_free(&stack);
		cyberiada_init_table_free_extensitions();
		return CYBERIADA_FORMAT_ERROR;
	}

	/* update composite node types */
	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_process_graph(sm->nodes, NULL, &cyberiada_update_complex_state);
	}

	if (!doc->format || strcmp(doc->format, CYBERIADA_FORMAT_CYBERIADAML) != 0) {
		if (!doc->format) {
			ERROR("CyberiadaML-GraphML format tag not found\n");
		} else {
			ERROR("Wrong CyberiadaML-GraphML format tag: %s\n", doc->format);
		}
		return CYBERIADA_FORMAT_ERROR;
	}

	
	/*meta_node = cyberiada_graph_find_node_by_id(sm->nodes, "");

	if (meta_node == NULL) {
		ERROR("meta node not found\n");
		return CYBERIADA_FORMAT_ERROR;
	}*/

	/* check extension nodes and remove non-visible nodes & edges */
	/*if (sm->edges) {
		prev_edge = NULL;
		edge = sm->edges;
		do {
			if (*(edge->source_id) == 0) {
				ext_node = cyberiada_graph_find_node_by_id(sm->nodes, edge->target_id);
				cyberiada_graph_add_extension(sm, ext_node->id, ext_node->title, ext_node->action);
				if (cyberiada_remove_node(sm, ext_node) != CYBERIADA_NO_ERROR) {
					ERROR("cannot remove ext node %s\n", edge->target_id);
					return CYBERIADA_ASSERT;
				}
				cyberiada_destroy_node(ext_node);
				if (prev_edge == NULL) {
					sm->edges = edge->next;
				} else {
					prev_edge->next = edge->next;
				}
				edge->next = NULL;
				cyberiada_destroy_edge(edge);
			} else {
				prev_edge = edge;
			}
			edge = edge->next;
		} while (edge);
	}
	if (cyberiada_remove_node(sm, meta_node) != CYBERIADA_NO_ERROR) {
		ERROR("cannot remove meta node\n");
		return CYBERIADA_ASSERT;
	}
	cyberiada_destroy_node(meta_node);
	*/
	cyberiada_init_table_free_extensitions();
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_check_graphml_ns(xmlNode* root, CyberiadaXMLFormat* format)
{
	xmlNs* ns;
	int graphml_found = 0;
	int yed_found = 0;
	if (!root || !(ns = root->nsDef)) {
		ERROR("bad GraphML XML NS: null ns ptr\n");
		return CYBERIADA_XML_ERROR;
	}
	do {
		if (strcmp((const char*)ns->href, GRAPHML_NAMESPACE_URI) == 0) {
			graphml_found = 1;
		} else if (strcmp((const char*)ns->href, GRAPHML_NAMESPACE_URI_YED) == 0) {
			yed_found = 1;
		}  
		ns = ns->next;
	} while (ns);
	if (!graphml_found) {
		ERROR("no GraphML XML NS href\n");
		return CYBERIADA_XML_ERROR;
	}
	if (*format == cybxmlUnknown) {
		if (yed_found) {
			*format = cybxmlYED;
		} else {
			*format = cybxmlCyberiada10;
		}
	} else if (*format == cybxmlYED && !yed_found) {
		ERROR("no GraphML YED NS href\n");
		return CYBERIADA_XML_ERROR;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_check_pseudostates(CyberiadaNode* nodes, CyberiadaEdge* edges, int check_initial, int toplevel)
{
	CyberiadaNode *n, *init_n = NULL;
	CyberiadaEdge *e;
	size_t initial = 0, initial_edges = 0;

	if (!nodes) {
		return CYBERIADA_NO_ERROR;
	}
	
	if (!nodes->parent) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	for (n = nodes; n; n = n->next) {
		if (n->type == cybNodeInitial) {
			initial++;
			init_n = n;
		}
		if (n->children) {
			int res = cyberiada_check_pseudostates(n->children, edges, check_initial, 0);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}

	if (initial > 1) {
		ERROR("Too many initial pseudostates (%lu) inside the node %s\n", initial, nodes->parent->id);
		return CYBERIADA_FORMAT_ERROR;
	}

	if (init_n) {
		for (e = edges; e; e = e->next) {
			if (e->source && e->source == init_n) {
				initial_edges++;
			}
		}
		
		if (initial_edges > 1) {
			ERROR("Too many edges from the initial pseudostate %s: %lu\n", init_n->id, initial_edges);
			return CYBERIADA_FORMAT_ERROR;
		}
	}
	
	if (check_initial && toplevel) {
		if (initial != 1) {
			ERROR("SM should have single initial pseudostate on the top level\n");
			return CYBERIADA_FORMAT_ERROR;
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_check_nodes_geometry(CyberiadaNode* nodes)
{
	CyberiadaNode* n;

	for (n = nodes; n; n = n->next) {
		if (n->type == cybNodeInitial || n->type == cybNodeFinal || n->type == cybNodeTerminate) {
			if (n->geometry_rect) {
				ERROR("Point node %s has rect geometry\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;
			}
		} else if (n->type == cybNodeSM || n->type == cybNodeSimpleState || n->type == cybNodeCompositeState ||
				   n->type == cybNodeSubmachineState || n->type == cybNodeChoice) {
			if (n->geometry_point) {
				ERROR("Rect (node %s) has point geometry\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;
			}
			if (n->geometry_rect && (n->geometry_rect->width == 0.0 && n->geometry_rect->height)) {
				ERROR("Rect (node %s) has zero width & height\n", n->id);
				return CYBERIADA_ACTION_FORMAT_ERROR;				
			}
		}
		if (n->children) {
			int res = cyberiada_check_nodes_geometry(n->children);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_check_graphs(CyberiadaDocument* doc, int skip_geometry, int check_initial)
{
	int res = CYBERIADA_NO_ERROR;
	CyberiadaSM* sm;

	if(!doc) {
		return CYBERIADA_BAD_PARAMETER;
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		if (sm->nodes) {
			if ((res = cyberiada_check_pseudostates(sm->nodes->children, sm->edges, check_initial, 1)) != CYBERIADA_NO_ERROR) {
				ERROR("error: state machine %s has wrong structure\n", sm->nodes->id);
				break;
			}
			if (!skip_geometry && (res = cyberiada_check_nodes_geometry(sm->nodes)) != CYBERIADA_NO_ERROR) {
				ERROR("error: state machine %s has wrong structure\n", sm->nodes->id);
				break;
			}
		}
	}
	return res;
}

/* -----------------------------------------------------------------------------
 * GraphML reader interface
 * ----------------------------------------------------------------------------- */
static int cyberiada_process_decode_sm_document(CyberiadaDocument* cyb_doc, xmlDoc* doc, CyberiadaXMLFormat format, int flags)
{
	int res;
	int skip_geometry = 0;
	xmlNode* root = NULL;
	CyberiadaSM* sm;
	NamesList* nl = NULL;
	int geom_flags;
	CyberiadaRegexps cyberiada_regexps;

	if (flags & CYBERIADA_FLAG_ROUND_GEOMETRY) {
		ERROR("Round geometry flag is not supported on import\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	skip_geometry = flags & CYBERIADA_FLAG_SKIP_GEOMETRY;
	if (skip_geometry &&
		(flags & ~CYBERIADA_NON_GEOMETRY_FLAGS_MASK) != CYBERIADA_FLAG_SKIP_GEOMETRY) {
		ERROR("The skip geometry flag is not compatible with other geometry flags\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (!skip_geometry) {

		geom_flags = flags & CYBERIADA_FLAG_NODES_GEOMETRY;
		if (!geom_flags) {
			/* set default nodes geometry flag */
			flags |= CYBERIADA_FLAG_NODES_CENTER_LOCAL_GEOMETRY;			
		} else if (geom_flags != CYBERIADA_FLAG_NODES_ABSOLUTE_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_NODES_LEFTTOP_LOCAL_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_NODES_CENTER_LOCAL_GEOMETRY) {
			ERROR("Single nodes geometry flag (abs, left-top, center) can be used at the same time\n");
			return CYBERIADA_BAD_PARAMETER;
		}

		geom_flags = flags & CYBERIADA_FLAG_EDGES_GEOMETRY;
		if (!geom_flags) {
			/* set default edges geometry flag */
			flags |= CYBERIADA_FLAG_EDGES_CENTER_LOCAL_GEOMETRY;			
		} else if (geom_flags != CYBERIADA_FLAG_EDGES_ABSOLUTE_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_EDGES_LEFTTOP_LOCAL_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_EDGES_CENTER_LOCAL_GEOMETRY) {
			ERROR("Single edges geometry flag (abs, left-top, center) can be used at the same time\n");
			return CYBERIADA_BAD_PARAMETER;
		}

		geom_flags = flags & CYBERIADA_FLAG_EDGES_PL_GEOMETRY;
		if (!geom_flags) {
			/* set default edges polylines geometry flag */
			flags |= CYBERIADA_FLAG_EDGES_PL_CENTER_LOCAL_GEOMETRY;			
		} else if (geom_flags != CYBERIADA_FLAG_EDGES_PL_ABSOLUTE_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_EDGES_PL_LEFTTOP_LOCAL_GEOMETRY &&
				   geom_flags != CYBERIADA_FLAG_EDGES_PL_CENTER_LOCAL_GEOMETRY) {
			ERROR("Single edge polylines geometry flag (abs, left-top, center) can be used at the same time\n");
			return CYBERIADA_BAD_PARAMETER;
		}

		geom_flags = flags & CYBERIADA_FLAG_EDGE_TYPE_GEOMETRY;
		if (!geom_flags) {
			/* set default edge geometry flag */
			flags |= CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY;	
		} else if (geom_flags != CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY &&
			  geom_flags != CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY) {
			ERROR("Single geometry edge flag (border, center) can be used at the same time\n");
			return CYBERIADA_BAD_PARAMETER;
		}
	}
	
	cyberiada_init_sm_document(cyb_doc);
	cyberiada_init_action_regexps(&cyberiada_regexps, flags & CYBERIADA_FLAG_FLATTENED);
	
	do {

		/* get the root element node */
		root = xmlDocGetRootElement(doc);

		if (strcmp((const char*)root->name, GRAPHML_GRAPHML_ELEMENT) != 0) {
			ERROR("error: could not find GraphML root node\n");
			res = CYBERIADA_XML_ERROR;
			break;
		}

		/* check whether the xml is graphml */
		if (cyberiada_check_graphml_ns(root, &format)) {
			ERROR("error: no valid graphml namespace\n");
			res = CYBERIADA_XML_ERROR;
			break;
		}

		cyb_doc->state_machines = cyberiada_new_sm();
		
		/* DEBUG("reading format %d\n", format); */
		if (format == cybxmlYED) {
			res = cyberiada_decode_yed_xml(root, cyb_doc, &cyberiada_regexps);
		} else if (format == cybxmlCyberiada10) {
			res = cyberiada_decode_cyberiada_xml(root, cyb_doc, &cyberiada_regexps);
		} else {
			ERROR("error: unsupported GraphML format of file\n");
			res = CYBERIADA_XML_ERROR;
			break;
		}

		if (res != CYBERIADA_NO_ERROR) {
			break;
		}

		for (sm = cyb_doc->state_machines; sm; sm = sm->next) {
			if ((res = cyberiada_graphs_reconstruct_node_identifiers(sm->nodes, &nl)) != CYBERIADA_NO_ERROR) {
				ERROR("error: cannot reconstruct graph nodes' indentifiers\n");
				break;
			}
		}
		
		if ((res = cyberiada_graphs_reconstruct_edge_identifiers(cyb_doc, &nl)) != CYBERIADA_NO_ERROR) {
			ERROR("error: cannot reconstruct graph edges' identifier\n");
			break;
		}

		res = cyberiada_check_graphs(cyb_doc, flags & CYBERIADA_FLAG_SKIP_GEOMETRY, flags & CYBERIADA_FLAG_CHECK_INITIAL);
		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error while checking graphs\n");
			break;
		}

		if (res == CYBERIADA_NO_ERROR) {
			if (flags & CYBERIADA_FLAG_SKIP_GEOMETRY) {
				cyberiada_clean_document_geometry(cyb_doc);
			} else if (cyberiada_document_has_geometry(cyb_doc) ||
					   flags & (CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY | CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY)) {
				cyberiada_import_document_geometry(cyb_doc, flags, format);
			} else {
				/* document has no geometry */
				cyberiada_document_no_geometry(cyb_doc);
			}
		}
	} while(0);

	cyberiada_free_name_list(&nl);
	cyberiada_free_action_regexps(&cyberiada_regexps);
	
    return res;	
}

static int cyberiada_detect_flattened_file(const char* filename, int* flattened)
{
	FILE* f;
	
	if (!filename || !flattened) {
		return CYBERIADA_BAD_PARAMETER;
	}

	f = fopen(filename, "r");
	if (!f) {
		return CYBERIADA_XML_ERROR;
	}

	*flattened = 1;
	
	while (!feof(f)) {
		char buffer[2];
		size_t r;
		r = fread(buffer, 1, 2, f);
		if (r < 2) break;
		if (isspace(buffer[0]) && isspace(buffer[1])) {
			*flattened = 0;
			break;
		}
	}
	
	fclose(f);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_detect_flattened_buffer(const char* buffer, size_t buffer_size, int* flattened)
{
	size_t i;
	
	if (!buffer || !flattened) {
		return CYBERIADA_BAD_PARAMETER;
	}

	*flattened = 1;
	
	for (i = 0; i + 1 < buffer_size; i += 2) {
		if (isspace(buffer[i]) && isspace(buffer[i + 1])) {
			*flattened = 0;
			break;
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_read_sm_document(CyberiadaDocument* cyb_doc, const char* filename,
							   CyberiadaXMLFormat format, int flags)
{
	int res;
	xmlDoc* doc = NULL;
	xmlInitParser();

	if (format != cybxmlCyberiada10) {
		int flattened = 0;
		if ((res = cyberiada_detect_flattened_file(filename, &flattened)) != CYBERIADA_NO_ERROR) {
			ERROR("error: could not read file %s\n", filename);
			return res;
		}
		if (flattened) flags |= CYBERIADA_FLAG_FLATTENED;
	}
	
	/* parse the file and get the DOM */
	if ((doc = xmlReadFile(filename, NULL, 0)) == NULL) {
		ERROR("error: could not parse file %s\n", filename);
		xmlCleanupParser();
		return CYBERIADA_XML_ERROR;
	}

	res = cyberiada_process_decode_sm_document(cyb_doc, doc, format, flags);
	
	if (doc) {
		xmlFreeDoc(doc);
	}
	xmlCleanupParser();
	return res;
}

int cyberiada_decode_sm_document(CyberiadaDocument* cyb_doc, const char* buffer, size_t buffer_size,
								 CyberiadaXMLFormat format, int flags)
{
	int res;
	xmlDoc* doc = NULL;
	xmlInitParser();

	if (format != cybxmlCyberiada10) {
		int flattened = 0;
		if ((res = cyberiada_detect_flattened_buffer(buffer, buffer_size, &flattened)) != CYBERIADA_NO_ERROR) {
			ERROR("error: could not read buffer\n");
			return res;
		}
		if (flattened) flags |= CYBERIADA_FLAG_FLATTENED;
	}
	
	/* parse the file and get the DOM */
	if ((doc = xmlReadMemory(buffer, buffer_size, XML_READMEMORY_BASENAME, NULL, 0)) == NULL) {
		ERROR("error: could not read buffer\n");
		xmlCleanupParser();
		return CYBERIADA_XML_ERROR;
	}

	res = cyberiada_process_decode_sm_document(cyb_doc, doc, format, flags);
	
	if (doc) {
		xmlFreeDoc(doc);
	}
	xmlCleanupParser();
	return res;
}

/* -----------------------------------------------------------------------------
 * CyberiadaML format writer
 * ----------------------------------------------------------------------------- */

static int cyberiada_write_action_text(xmlTextWriterPtr writer, CyberiadaAction* action)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	
	while (action) {

		if (action->type != cybActionTransition || *(action->trigger) || *(action->behavior) || *(action->guard)) { 
			if (action->type != cybActionTransition) {
				if (action->type == cybActionEntry) {
					snprintf(buffer, buffer_len, "entry/");
				} else if (action->type == cybActionExit) {
					snprintf(buffer, buffer_len, "exit/");
				} else {
					ERROR("Bad action type %d", action->type);
					return CYBERIADA_ASSERT;
				}
			} else {
				if (*(action->guard)) {
					if (*(action->trigger)) {
						snprintf(buffer, buffer_len, "%s [%s]/", action->trigger, action->guard);
					} else {
						snprintf(buffer, buffer_len, "[%s]/", action->guard);
					}
				} else {
					snprintf(buffer, buffer_len, "%s/", action->trigger);
				}
			}
			XML_WRITE_TEXT(writer, buffer);
			if (action->next || *(action->behavior)) {
				XML_WRITE_TEXT(writer, "\n");
		
				if (*(action->behavior)) {
					XML_WRITE_TEXT(writer, action->behavior);
					XML_WRITE_TEXT(writer, "\n");
				}
				
				if (action->next) {
					XML_WRITE_TEXT(writer, "\n");
				}
			}
		}

		action = action->next;
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_geometry_rect_cyberiada(xmlTextWriterPtr writer, CyberiadaRect* rect, int indent)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;

	XML_WRITE_OPEN_E_I(writer, GRAPHML_RECT_ELEMENT, indent);
	snprintf(buffer, buffer_len, "%lf", rect->x);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_X_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->y);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_Y_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->width);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_WIDTH_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->height);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_HEIGHT_ATTRIBUTE, buffer);
	XML_WRITE_CLOSE_E(writer);

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_geometry_point_cyberiada(xmlTextWriterPtr writer, CyberiadaPoint* point, int indent)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;

	XML_WRITE_OPEN_E_I(writer, GRAPHML_POINT_ELEMENT, indent);
	snprintf(buffer, buffer_len, "%lf", point->x);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_X_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", point->y);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_Y_ATTRIBUTE, buffer);
	XML_WRITE_CLOSE_E(writer);

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_node_cyberiada(xmlTextWriterPtr writer, CyberiadaNode* node, int indent)
{
	int res, found;
	CyberiadaNode* cur_node;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	size_t i;
	
	XML_WRITE_OPEN_E_I(writer, GRAPHML_NODE_ELEMENT, indent);
	XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, node->id);

	if (node->type == cybNodeSM) {
		ERROR("Embedded SM node %s not supported\n", node->id);
		return CYBERIADA_BAD_PARAMETER;		
	} else if (node->type == cybNodeComment || node->type == cybNodeFormalComment) {

		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_COMMENT);
		if (node->type == cybNodeFormalComment) {
			XML_WRITE_TEXT(writer, GRAPHML_CYB_COMMENT_FORMAL);
		} else {
			XML_WRITE_TEXT(writer, GRAPHML_CYB_COMMENT_INFORMAL);
		}
		XML_WRITE_CLOSE_E(writer);
	
	} else if (node->type == cybNodeSubmachineState) {

		if (!node->link || !node->link->ref) {
			ERROR("Submachine state %s has no link\n", node->id);
			return CYBERIADA_BAD_PARAMETER;
		}
		
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_SUBMACHINE);
		XML_WRITE_TEXT(writer, node->link->ref);
		XML_WRITE_CLOSE_E(writer);
		
	} else if (node->type != cybNodeSimpleState && node->type != cybNodeCompositeState) {

		found = 0;
		for (i = 0; i < cyberiada_vertexes_count; i++) {
			if (node->type == cyberiada_vertexes[i].type) {
				XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
				XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_VERTEX);
				XML_WRITE_TEXT(writer, cyberiada_vertexes[i].name);
				XML_WRITE_CLOSE_E(writer);
				found = 1;
				break;
			}
		}
		if (!found) {
			ERROR("Unsupported node type %d\n", node->type);
			return CYBERIADA_BAD_PARAMETER;
		}
	}

	if (node->title) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_NAME);
		XML_WRITE_TEXT(writer, node->title);
		XML_WRITE_CLOSE_E(writer);		
	}

	if ((node->type == cybNodeComment || node->type == cybNodeFormalComment) && node->comment_data) {
		if (node->comment_data->body) {
			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_DATA);
			XML_WRITE_TEXT(writer, node->comment_data->body);
			XML_WRITE_CLOSE_E(writer);
		}
		if (node->comment_data->markup) {
			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_MARKUP);
			XML_WRITE_TEXT(writer, node->comment_data->markup);
			XML_WRITE_CLOSE_E(writer);
		}
	}

	if (node->actions) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_DATA);
		cyberiada_write_action_text(writer, node->actions);
		XML_WRITE_CLOSE_E(writer);		
	}

	if (node->geometry_rect) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_GEOMETRY);
		if ((res = cyberiada_write_geometry_rect_cyberiada(writer,
														   node->geometry_rect,
														   indent + 2)) != CYBERIADA_NO_ERROR) {
			ERROR("Cannot write node %s geometry rect\n", node->id);
			return res;
		}
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (node->geometry_point) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_GEOMETRY);
		if ((res = cyberiada_write_geometry_point_cyberiada(writer,
															node->geometry_point,
															indent + 2)) != CYBERIADA_NO_ERROR) {
			ERROR("Cannot write node %s geometry point\n", node->id);
			return res;
		}
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (node->color) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_COLOR);
		XML_WRITE_TEXT(writer, node->color);
		XML_WRITE_CLOSE_E(writer);		
	}

	if (node->type == cybNodeCompositeState) {
		/* the root graph element */
		XML_WRITE_OPEN_E_I(writer, GRAPHML_GRAPH_ELEMENT, indent + 1);
		snprintf(buffer, buffer_len, "%s:", node->id);
		XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, buffer);
		XML_WRITE_ATTR(writer, GRAPHML_EDGEDEFAULT_ATTRIBUTE, GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE);
		
		for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
			res = cyberiada_write_node_cyberiada(writer, cur_node, indent + 2);
			if (res != CYBERIADA_NO_ERROR) {
				ERROR("error while writing node %s\n", cur_node->id);
				return CYBERIADA_XML_ERROR;
			}
		}
		
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	XML_WRITE_CLOSE_E_I(writer, indent);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_edge_cyberiada(xmlTextWriterPtr writer, CyberiadaEdge* edge, int indent)
{
	int res;
/*	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;*/
	CyberiadaPolyline* pl;

	if (!edge->id) {
		ERROR("SM edge %s -> %s w/o id\n", edge->source_id, edge->target_id);
		return CYBERIADA_BAD_PARAMETER;				
	}
	
	XML_WRITE_OPEN_E_I(writer, GRAPHML_EDGE_ELEMENT, indent);
	XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, edge->id);
	XML_WRITE_ATTR(writer, GRAPHML_SOURCE_ATTRIBUTE, edge->source_id);
	XML_WRITE_ATTR(writer, GRAPHML_TARGET_ATTRIBUTE, edge->target_id);

	if (edge->action) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_DATA);
		cyberiada_write_action_text(writer, edge->action);
		XML_WRITE_CLOSE_E(writer);		
	}

	if (edge->type == cybEdgeComment && edge->comment_subject) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_COMMENT_SUBJECT);
		if (edge->comment_subject->type == cybCommentSubjectNameFragment) {
			XML_WRITE_TEXT(writer, GRAPHML_CYB_KEY_NAME);
		} else if (edge->comment_subject->type == cybCommentSubjectDataFragment) {
			XML_WRITE_TEXT(writer, GRAPHML_CYB_KEY_DATA);
		}
		XML_WRITE_CLOSE_E(writer);
		
		if (edge->comment_subject->fragment) {
			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_COMMENT_CHUNK);
			XML_WRITE_TEXT(writer, edge->comment_subject->fragment);
			XML_WRITE_CLOSE_E(writer);
		}
	}
	
	/* XML_WRITE_OPEN_E_I(writer, GRAPHML_YED_PATHNODE, indent + 3); */
	/* if (edge->geometry_source_point && edge->geometry_target_point) {	 */
	/* 	snprintf(buffer, buffer_len, "%lf", edge->geometry_source_point->x); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE, buffer); */
	/* 	snprintf(buffer, buffer_len, "%lf", edge->geometry_source_point->y); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE, buffer); */
	/* 	snprintf(buffer, buffer_len, "%lf", edge->geometry_target_point->x); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE, buffer); */
	/* 	snprintf(buffer, buffer_len, "%lf", edge->geometry_target_point->y); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE, buffer); */
	/* } else { */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE, "0"); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE, "0"); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE, "0"); */
	/* 	XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE, "0"); */
	/* } */

	if (edge->geometry_polyline) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_GEOMETRY);
		pl = edge->geometry_polyline;
		do {
			cyberiada_write_geometry_point_cyberiada(writer, &(pl->point), indent + 2);
			pl = pl->next;
		} while (pl);
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}
	
	if (edge->geometry_source_point) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_SOURCE_POINT);
		cyberiada_write_geometry_point_cyberiada(writer, edge->geometry_source_point, indent + 2);
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (edge->geometry_target_point) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_TARGET_POINT);
		cyberiada_write_geometry_point_cyberiada(writer, edge->geometry_target_point, indent + 2);
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (edge->geometry_label_point) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_LABEL_GEOMETRY);
		cyberiada_write_geometry_point_cyberiada(writer, edge->geometry_label_point, indent + 2);
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (edge->geometry_label_rect) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_LABEL_GEOMETRY);
		cyberiada_write_geometry_rect_cyberiada(writer, edge->geometry_label_rect, indent + 2);
		XML_WRITE_CLOSE_E_I(writer, indent + 1);
	}

	if (edge->color) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_COLOR);
		XML_WRITE_TEXT(writer, edge->color);
		XML_WRITE_CLOSE_E(writer);
	}
	
	XML_WRITE_CLOSE_E_I(writer, indent);
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_write_sm_cyberiada(CyberiadaSM* sm, xmlTextWriterPtr writer)
{
	int res;
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;

	if (!sm->nodes) {
		ERROR("SM node is required\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!sm->nodes->id) {
		ERROR("SM node id is required\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!sm->nodes->title) {
		ERROR("SM node title is required\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	/* the root graph element */
	XML_WRITE_OPEN_E_I(writer, GRAPHML_GRAPH_ELEMENT, 1);
	XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, sm->nodes->id);
	XML_WRITE_ATTR(writer, GRAPHML_EDGEDEFAULT_ATTRIBUTE, GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, 2);
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_STATE_MACHINE);
	XML_WRITE_CLOSE_E(writer);
	
	XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, 2);
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_NAME);
	XML_WRITE_TEXT(writer, sm->nodes->title);	
	XML_WRITE_CLOSE_E(writer);

	if (sm->nodes->geometry_rect) {
		XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, 2);
		XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_GEOMETRY);
		if ((res = cyberiada_write_geometry_rect_cyberiada(writer,
														   sm->nodes->geometry_rect,
														   3)) != CYBERIADA_NO_ERROR) {
			ERROR("Cannot write SM %s geometry rect\n", sm->nodes->id);
			return CYBERIADA_XML_ERROR;
		}
		XML_WRITE_CLOSE_E_I(writer, 2);
	}
	
	/* write nodes */
	for (cur_node = sm->nodes->children; cur_node; cur_node = cur_node->next) {
		res = cyberiada_write_node_cyberiada(writer, cur_node, 2);
		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error while writing node %s\n", cur_node->id);
			return CYBERIADA_XML_ERROR;
		}
	}
		
	/* write edges */
	for (cur_edge = sm->edges; cur_edge; cur_edge = cur_edge->next) {
		res = cyberiada_write_edge_cyberiada(writer, cur_edge, 2);
		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error while writing edge %s\n", cur_edge->id);
			return CYBERIADA_XML_ERROR;
		}
	}

	XML_WRITE_CLOSE_E_I(writer, 1);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_update_metainfo_comment(CyberiadaDocument* doc)
{
	CyberiadaNode *sm_node, *first_node, *meta_node;
	if (!doc->state_machines) {
		/* empty doc */
		ERROR("At least one SM required\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	sm_node = doc->state_machines->nodes;
	if (sm_node->type != cybNodeSM ||
		sm_node->next != NULL) {
		ERROR("Inconsistem SM node\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	first_node = sm_node->children; 
	if (first_node &&
		first_node->type == cybNodeFormalComment &&
		first_node->title &&
		strcmp(first_node->title, CYBERIADA_META_NODE_TITLE) == 0) {
		if (first_node->comment_data) {
			if (first_node->comment_data->body) {
				free(first_node->comment_data->body);
				first_node->comment_data->body = NULL;
				first_node->comment_data->body_len = 0;
			}
		} else {
			first_node->comment_data = cyberiada_new_comment_data();
		}
		meta_node = first_node;
	} else {
		meta_node = cyberiada_new_node(CYBERIADA_META_NODE_DEFAULT_ID);
		meta_node->type = cybNodeFormalComment;
		meta_node->comment_data = cyberiada_new_comment_data();
		cyberiada_copy_string(&(meta_node->title),
							  &(meta_node->title_len),
							  CYBERIADA_META_NODE_TITLE);
		sm_node->children = meta_node;
		meta_node->next = first_node;
	}
	cyberiada_encode_meta(doc->meta_info,
						  &(meta_node->comment_data->body),
						  &(meta_node->comment_data->body_len));
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_sm_document_cyberiada(CyberiadaDocument* doc, xmlTextWriterPtr writer)
{
	int res;
	size_t i;
	GraphMLKey* key;
	CyberiadaSM* sm;
	if (!doc->format) {
		cyberiada_copy_string(&(doc->format),
							  &(doc->format_len),
							  CYBERIADA_FORMAT_CYBERIADAML);	
	}
	res = cyberiada_update_metainfo_comment(doc);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}

	XML_WRITE_OPEN_E(writer, GRAPHML_GRAPHML_ELEMENT);
	XML_WRITE_ATTR(writer, "xmlns", GRAPHML_NAMESPACE_URI);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, 1);
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_FORMAT);
	XML_WRITE_TEXT(writer, CYBERIADA_FORMAT_CYBERIADAML);	
	XML_WRITE_CLOSE_E(writer);
	
	/* write graphml keys */
	for (i = 0; i < cyberiada_graphml_keys_count; i++) {
		key = cyberiada_graphml_keys + i;
		XML_WRITE_OPEN_E_I(writer, GRAPHML_KEY_ELEMENT, 1);
		XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, key->attr_id);
		XML_WRITE_ATTR(writer, GRAPHML_FOR_ATTRIBUTE, key->attr_for);
		XML_WRITE_ATTR(writer, GRAPHML_ATTR_NAME_ATTRIBUTE, key->attr_name);
		if (key->attr_type) {
			XML_WRITE_ATTR(writer, GRAPHML_ATTR_TYPE_ATTRIBUTE, key->attr_type);
		}
		XML_WRITE_CLOSE_E(writer);
	}

	for (sm = doc->state_machines; sm; sm = sm->next) {
		if ((res = cyberiada_write_sm_cyberiada(sm, writer)) != CYBERIADA_NO_ERROR) {
			ERROR("Error while writing SM: %d\n", res);
			return res;
		}
	}
	
	XML_WRITE_CLOSE_E_I(writer, 0);
	
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * Legacy YED format writer
 * ----------------------------------------------------------------------------- */

#define GRAPHML_YED_NS                  "y"
#define GRAPHML_YED_ROOT_GRAPH_ID       "G"

static const char* yed_graphml_attributes[] = {
	"xmlns", "http://graphml.graphdrawing.org/xmlns",
	"xmlns:java", "http://www.yworks.com/xml/yfiles-common/1.0/java",
	"xmlns:sys", "http://www.yworks.com/xml/yfiles-common/markup/primitives/2.0",
	"xmlns:x", "http://www.yworks.com/xml/yfiles-common/markup/2.0",
	"xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance",
	"xmlns:y", "http://www.yworks.com/xml/graphml",
	"xmlns:yed", "http://www.yworks.com/xml/yed/3",
	"yed:schemaLocation", "http://graphml.graphdrawing.org/xmlns http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd"
};
const size_t yed_graphml_attributes_count = (sizeof(yed_graphml_attributes) / sizeof(const char*));

static int cyberiada_write_node_style_yed(xmlTextWriterPtr writer, CyberiadaNodeType type, int indent)
{
	int res;

	if (type == cybNodeCompositeState) {
		XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_FILLNODE, GRAPHML_YED_NS, indent);
		XML_WRITE_ATTR(writer, "color", "#E8EEF7");
		XML_WRITE_ATTR(writer, "color2", "#B7C9E3");
		XML_WRITE_ATTR(writer, "transparent", "false");
		XML_WRITE_CLOSE_E(writer);
	} else if (type == cybNodeInitial) {
		XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_FILLNODE, GRAPHML_YED_NS, indent);
		XML_WRITE_ATTR(writer, "color", "#333333");
		XML_WRITE_ATTR(writer, "color2", "#000000");
		XML_WRITE_ATTR(writer, "transparent", "false");
		XML_WRITE_CLOSE_E(writer);
	} else if (type == cybNodeCompositeState) {
		XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_FILLNODE, GRAPHML_YED_NS, indent);
		XML_WRITE_ATTR(writer, "color", "#F5F5F5");
		XML_WRITE_ATTR(writer, "transparent", "false");
		XML_WRITE_CLOSE_E(writer);
	} 
	
	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_BORDERSTYLENODE, GRAPHML_YED_NS, indent);
	XML_WRITE_ATTR(writer, "color", "#000000");
	XML_WRITE_ATTR(writer, "type", "line");
	XML_WRITE_ATTR(writer, "width", "1.0");
	XML_WRITE_CLOSE_E(writer);

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_node_title_yed(xmlTextWriterPtr writer, const char* title, int indent)
{
	int res;
	
	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_LABELNODE, GRAPHML_YED_NS, indent);

	if (*title) {
		XML_WRITE_ATTR(writer, "alignment", "center");
		XML_WRITE_ATTR(writer, "backgroundColor", "#EBEBEB");
		XML_WRITE_ATTR(writer, "fontSize", "15");
		XML_WRITE_ATTR(writer, "fontStyle", "bold");
		XML_WRITE_ATTR(writer, "textColor", "#000000");
		XML_WRITE_ATTR(writer, "xml:space", "preserve");
		XML_WRITE_ATTR(writer, "hasLineColor", "false");
		XML_WRITE_ATTR(writer, "visible", "true");
		XML_WRITE_ATTR(writer, "horizontalTextPosition", "center");
		XML_WRITE_ATTR(writer, "verticalTextPosition", "top");
		XML_WRITE_ATTR(writer, "autoSizePolicy", "node_width");
		XML_WRITE_ATTR(writer, "y", "0");
		XML_WRITE_ATTR(writer, "height", "20");
		XML_WRITE_ATTR(writer, "configuration", "com.yworks.entityRelationship.label.name");
		XML_WRITE_ATTR(writer, "modelName", "internal");
		XML_WRITE_ATTR(writer, "modelPosition", "t");
		XML_WRITE_TEXT(writer, title);
	}
	XML_WRITE_CLOSE_E(writer);
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_write_node_action_yed(xmlTextWriterPtr writer, CyberiadaAction* action, int indent)
{
	int res;

	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_LABELNODE, GRAPHML_YED_NS, indent);
	XML_WRITE_ATTR(writer, "alignment", "left");
	XML_WRITE_ATTR(writer, "hasBackgroundColor", "false");
	XML_WRITE_ATTR(writer, "fontSize", "12");
	XML_WRITE_ATTR(writer, "fontStyle", "plain");
	XML_WRITE_ATTR(writer, "textColor", "#000000");
	XML_WRITE_ATTR(writer, "xml:space", "preserve");
	XML_WRITE_ATTR(writer, "hasLineColor", "false");
	XML_WRITE_ATTR(writer, "visible", "true");
	XML_WRITE_ATTR(writer, "horizontalTextPosition", "center");
	XML_WRITE_ATTR(writer, "verticalTextPosition", "bottom");
	XML_WRITE_ATTR(writer, "autoSizePolicy", "node_size");
	XML_WRITE_TEXT(writer, "\n");
	XML_WRITE_TEXT(writer, "\n");

	if (cyberiada_write_action_text(writer, action) != CYBERIADA_NO_ERROR) {
		ERROR("error while writing node bevavior text\n");
		return CYBERIADA_XML_ERROR;
	}
	
	XML_WRITE_CLOSE_E(writer);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_edge_action_yed(xmlTextWriterPtr writer, CyberiadaAction* action, int indent)
{
	int res;

	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_EDGELABEL, GRAPHML_YED_NS, indent);
	XML_WRITE_ATTR(writer, "alignment", "center");
	XML_WRITE_ATTR(writer, "fontSize", "12");
	XML_WRITE_ATTR(writer, "fontStyle", "plain");
	XML_WRITE_ATTR(writer, "textColor", "#000000");
	XML_WRITE_ATTR(writer, "backgroundColor", "#F5F5F5");
	XML_WRITE_ATTR(writer, "configuration", "AutoFlippingLabel");
	XML_WRITE_ATTR(writer, "distance", "2.0");
	XML_WRITE_ATTR(writer, "hasLineColor", "false");
	XML_WRITE_ATTR(writer, "visible", "true");
	XML_WRITE_ATTR(writer, "xml:space", "preserve");
	XML_WRITE_ATTR(writer, "modelName", "centered");
	XML_WRITE_ATTR(writer, "modelPosition", "center");
	XML_WRITE_ATTR(writer, "preferredPlacement", "center_on_edge");
	
	if (cyberiada_write_action_text(writer, action) != CYBERIADA_NO_ERROR) {
		ERROR("error while writing node bevavior text\n");
		return CYBERIADA_XML_ERROR;
	}

	/*	<y:PreferredPlacementDescriptor angle="0.0" angleOffsetOnRightSide="0" angleReference="absolute" angleRotationOnRightSide="co" distance="-1.0" placement="center" side="on_edge" sideReference="relative_to_edge_flow" /></y:EdgeLabel>
*/
	
	XML_WRITE_CLOSE_E(writer);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_geometry_yed(xmlTextWriterPtr writer, CyberiadaRect* rect, int indent)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;

	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_GEOMETRYNODE, GRAPHML_YED_NS, indent);
	snprintf(buffer, buffer_len, "%lf", rect->x);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_X_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->y);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_Y_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->width);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_WIDTH_ATTRIBUTE, buffer);
	snprintf(buffer, buffer_len, "%lf", rect->height);
	XML_WRITE_ATTR(writer, GRAPHML_GEOM_HEIGHT_ATTRIBUTE, buffer);
	XML_WRITE_CLOSE_E(writer);

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_node_yed(xmlTextWriterPtr writer, CyberiadaNode* node, int indent)
{
	int res;
	CyberiadaNode* cur_node;
	const char* text;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;

	if (node->type == cybNodeSM) {
		
		for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
			res = cyberiada_write_node_yed(writer, cur_node, indent);
			if (res != CYBERIADA_NO_ERROR) {
				ERROR("error while writing root node %s\n", cur_node->id);
				return CYBERIADA_XML_ERROR;
			}
		}
		
	} else {
	
		XML_WRITE_OPEN_E_I(writer, GRAPHML_NODE_ELEMENT, indent);
		XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, node->id);
		
		if (node->type == cybNodeInitial) {
			
			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_NODE_GRAPHICS);

			XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_GENERICNODE, GRAPHML_YED_NS, indent + 2);
			XML_WRITE_ATTR(writer, "configuration", GRAPHML_YED_NODE_CONFIG_START2);

			if (node->geometry_rect) {
				if (cyberiada_write_geometry_yed(writer, node->geometry_rect, indent + 3) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing initial node geometry\n");
					return CYBERIADA_XML_ERROR;
				}
				if (cyberiada_write_node_style_yed(writer, node->type, indent + 3) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing initial node style\n");
					return CYBERIADA_XML_ERROR;
				}
			}
			
			if (node->title) {
				if (cyberiada_write_node_title_yed(writer, node->title, indent + 3) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing initial node label\n");
					return CYBERIADA_XML_ERROR;
				}
			}
			
			XML_WRITE_CLOSE_E_I(writer, indent + 2);
			XML_WRITE_CLOSE_E_I(writer, indent + 1);
			
		} else if (node->type == cybNodeSimpleState) {

			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_NODE_GRAPHICS);
			XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_GENERICNODE, GRAPHML_YED_NS, indent + 2);

			if (node->geometry_rect) {
				if (cyberiada_write_geometry_yed(writer, node->geometry_rect, indent + 3) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing composite node geometry\n");
					return CYBERIADA_XML_ERROR;
				}
				if (cyberiada_write_node_style_yed(writer, node->type, indent + 3) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing composite node style\n");
					return CYBERIADA_XML_ERROR;
				}
			}

			text = node->title;
			if (!text) text = "";
			if (cyberiada_write_node_title_yed(writer, text, indent + 3) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing simple node label\n");
				return CYBERIADA_XML_ERROR;
			}

			if (cyberiada_write_node_action_yed(writer, node->actions, indent + 3) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing simple node action\n");
				return CYBERIADA_XML_ERROR;
			}

			XML_WRITE_CLOSE_E_I(writer, indent + 2);
			XML_WRITE_CLOSE_E_I(writer, indent + 1);
			
		} else if (node->type == cybNodeCompositeState) {
			XML_WRITE_ATTR(writer, "yfiles.foldertype", "group");

			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_NODE_DESCR);
			XML_WRITE_ATTR(writer, "xml:space", "preserve");
			XML_WRITE_CLOSE_E(writer);

			XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
			XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_NODE_GRAPHICS);
			XML_WRITE_OPEN_E_NS_I(writer, "ProxyAutoBoundsNode", GRAPHML_YED_NS, indent + 2);
			XML_WRITE_OPEN_E_NS_I(writer, "Realizers", GRAPHML_YED_NS, indent + 3);
			XML_WRITE_ATTR(writer, "active", "0");
			XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_GROUPNODE, GRAPHML_YED_NS, indent + 4);

			if (node->geometry_rect) {
				if (cyberiada_write_geometry_yed(writer, node->geometry_rect, indent + 5) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing composite node geometry\n");
					return CYBERIADA_XML_ERROR;
				}
				if (cyberiada_write_node_style_yed(writer, node->type, indent + 5) != CYBERIADA_NO_ERROR) {
					ERROR("error while writing composite node style\n");
					return CYBERIADA_XML_ERROR;
				}
			}

			text = node->title;
			if (!text) text = "";
			if (cyberiada_write_node_title_yed(writer, text, indent + 5) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing composite node label\n");
				return CYBERIADA_XML_ERROR;
			}

			if (cyberiada_write_node_action_yed(writer, node->actions, indent + 5) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing composite node action\n");
				return CYBERIADA_XML_ERROR;
			}

			XML_WRITE_OPEN_E_NS_I(writer, "Shape", GRAPHML_YED_NS, indent + 5);
			XML_WRITE_ATTR(writer, "type", "roundrectangle");
			XML_WRITE_CLOSE_E(writer);
			
			XML_WRITE_CLOSE_E_I(writer, indent + 4);
			XML_WRITE_CLOSE_E_I(writer, indent + 3);
			XML_WRITE_CLOSE_E_I(writer, indent + 2);
			XML_WRITE_CLOSE_E_I(writer, indent + 1);

			/* the root graph element */
			XML_WRITE_OPEN_E_I(writer, GRAPHML_GRAPH_ELEMENT, indent + 1);
			snprintf(buffer, buffer_len, "%s:", node->id);
			XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, buffer);
			XML_WRITE_ATTR(writer, GRAPHML_EDGEDEFAULT_ATTRIBUTE, GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE);

			for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
				res = cyberiada_write_node_yed(writer, cur_node, indent + 2);
				if (res != CYBERIADA_NO_ERROR) {
					ERROR("error while writing node %s\n", cur_node->id);
					return CYBERIADA_XML_ERROR;
				}
			}

			XML_WRITE_CLOSE_E_I(writer, indent + 1);
		}

		XML_WRITE_CLOSE_E_I(writer, indent);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_edge_yed(xmlTextWriterPtr writer, CyberiadaEdge* edge, int indent)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	
	XML_WRITE_OPEN_E_I(writer, GRAPHML_EDGE_ELEMENT, indent);
	XML_WRITE_ATTR(writer, GRAPHML_SOURCE_ATTRIBUTE, edge->source_id);
	XML_WRITE_ATTR(writer, GRAPHML_TARGET_ATTRIBUTE, edge->target_id);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, indent + 1);
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_EDGE_GRAPHICS);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_YED_POLYLINEEDGE, indent + 2);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_YED_PATHNODE, indent + 3);
	if (edge->geometry_source_point && edge->geometry_target_point) {	
		snprintf(buffer, buffer_len, "%lf", edge->geometry_source_point->x);
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE, buffer);
		snprintf(buffer, buffer_len, "%lf", edge->geometry_source_point->y);
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE, buffer);
		snprintf(buffer, buffer_len, "%lf", edge->geometry_target_point->x);
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE, buffer);
		snprintf(buffer, buffer_len, "%lf", edge->geometry_target_point->y);
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE, buffer);
	} else {
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE, "0");
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE, "0");
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE, "0");
		XML_WRITE_ATTR(writer, GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE, "0");
	}
	XML_WRITE_CLOSE_E(writer);

	XML_WRITE_OPEN_E_NS_I(writer, GRAPHML_YED_LINESTYLENODE, GRAPHML_YED_NS, indent + 3);
	XML_WRITE_ATTR(writer, "color", "#000000");
	XML_WRITE_ATTR(writer, "type", "line");
	XML_WRITE_ATTR(writer, "width", "1.0");
	XML_WRITE_CLOSE_E(writer);

	XML_WRITE_OPEN_E_NS_I(writer, "Arrows", GRAPHML_YED_NS, indent + 3);
	XML_WRITE_ATTR(writer, "source", "none");
	XML_WRITE_ATTR(writer, "target", "standard");
	XML_WRITE_CLOSE_E(writer);

	if (cyberiada_write_edge_action_yed(writer, edge->action, indent + 3) != CYBERIADA_NO_ERROR) {
		ERROR("error while writing edge action\n");
		return CYBERIADA_XML_ERROR;
	}
	    			 
	XML_WRITE_CLOSE_E_I(writer, indent + 2);
	XML_WRITE_CLOSE_E_I(writer, indent + 1);
	XML_WRITE_CLOSE_E_I(writer, indent);
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_write_sm_document_yed(CyberiadaDocument* doc, xmlTextWriterPtr writer)
{
	size_t i;
	int res;
	GraphMLKey* key;
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;
	CyberiadaSM* sm = doc->state_machines;
	
	XML_WRITE_OPEN_E(writer, GRAPHML_GRAPHML_ELEMENT);

	/* add YED graphml attributes */
	for (i = 0; i < yed_graphml_attributes_count; i += 2) {
		XML_WRITE_ATTR(writer, yed_graphml_attributes[i], yed_graphml_attributes[i + 1]);		
	}
	/* add scheme name if it is available */
	if (doc->meta_info && doc->meta_info->name && *doc->meta_info->name) {
		XML_WRITE_ATTR(writer, GRAPHML_BERLOGA_SCHEMENAME_ATTR, doc->meta_info->name);
	}

	/* write graphml keys */
	for (i = 0; i < yed_graphml_keys_count; i++) {
		key = yed_graphml_keys + i;
		XML_WRITE_OPEN_E_I(writer, GRAPHML_KEY_ELEMENT, 1);
		XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, key->attr_id);
		XML_WRITE_ATTR(writer, GRAPHML_FOR_ATTRIBUTE, key->attr_for);
		if (key->attr_name && key->attr_type) {
			XML_WRITE_ATTR(writer, GRAPHML_ATTR_NAME_ATTRIBUTE, key->attr_name);
			XML_WRITE_ATTR(writer, GRAPHML_ATTR_TYPE_ATTRIBUTE, key->attr_type);
		}
		if (key->extra) {
			XML_WRITE_ATTR(writer, GRAPHML_YED_YFILES_TYPE_ATTR, key->extra);
		}
		XML_WRITE_CLOSE_E(writer);
	}
	
	/* the root graph element */
	XML_WRITE_OPEN_E_I(writer, GRAPHML_GRAPH_ELEMENT, 1);
	XML_WRITE_ATTR(writer, GRAPHML_ID_ATTRIBUTE, GRAPHML_YED_ROOT_GRAPH_ID);
	XML_WRITE_ATTR(writer, GRAPHML_EDGEDEFAULT_ATTRIBUTE, GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE);

	XML_WRITE_OPEN_E_I(writer, GRAPHML_DATA_ELEMENT, 2);
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_YED_KEY_GRAPH_DESCR);
	XML_WRITE_ATTR(writer, "xml:space", "preserve");
	XML_WRITE_CLOSE_E(writer);

	/* write nodes */
	for (cur_node = sm->nodes; cur_node; cur_node = cur_node->next) {
		res = cyberiada_write_node_yed(writer, cur_node, 2);
		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error while writing node %s\n", cur_node->id);
			return CYBERIADA_XML_ERROR;
		}
	}
		
	/* write edges */
	for (cur_edge = sm->edges; cur_edge; cur_edge = cur_edge->next) {
		res = cyberiada_write_edge_yed(writer, cur_edge, 2);
		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error while writing edge %s\n", cur_edge->id);
			return CYBERIADA_XML_ERROR;
		}
	}

	XML_WRITE_CLOSE_E_I(writer, 1);
	XML_WRITE_CLOSE_E_I(writer, 0);
	
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * GraphML writer interface
 * ----------------------------------------------------------------------------- */

static int cyberiada_process_encode_sm_document(CyberiadaDocument* doc, xmlTextWriterPtr writer,
												CyberiadaXMLFormat format, int flags)
{
	CyberiadaDocument* copy_doc = NULL;
	int res;

	if (flags & (CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY | CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY)) {
		ERROR("Geometry reconstructioin flag is not supported on export\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (flags & CYBERIADA_FLAG_SKIP_GEOMETRY) {
		if (flags != CYBERIADA_FLAG_SKIP_GEOMETRY) {
			ERROR("The skip geometry flag is not compatible with other flags\n");
			return CYBERIADA_BAD_PARAMETER;
		}
		if (format == cybxmlYED) {
			ERROR("Skip geometry flag is not allowed for YED export\n");
			return CYBERIADA_BAD_PARAMETER;
		}
	}
	
	if (flags & CYBERIADA_FLAG_ANY_GEOMETRY) {
		ERROR("Geometry flags (abs, left-top, center) & edge geometry flags are not allowed for export\n");
		return CYBERIADA_BAD_PARAMETER;
	}
	
	if (format != cybxmlCyberiada10 && format != cybxmlYED) {
		ERROR("unsupported SM format for write: %d\n", format);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (!doc) {
		ERROR("empty SM document to write\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	if (format == cybxmlYED && (!doc->state_machines ||doc->state_machines->next)) {
		ERROR("YED format supports only single SM documents\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	res = cyberiada_check_graphs(doc, flags & CYBERIADA_FLAG_SKIP_GEOMETRY, flags & CYBERIADA_FLAG_CHECK_INITIAL);
	if (res != CYBERIADA_NO_ERROR) {
		ERROR("error while checking graphs\n");
		return res;
	}

	do {
		res = xmlTextWriterStartDocument(writer, NULL, GRAPHML_XML_ENCODING, NULL);
		if (res < 0) {
			ERROR("error writing xml start document: %d\n", res);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		copy_doc = cyberiada_copy_sm_document(doc);

		if (flags & CYBERIADA_FLAG_SKIP_GEOMETRY) {
			cyberiada_clean_document_geometry(copy_doc);
		} else {
			cyberiada_export_document_geometry(copy_doc, flags, format);
		}
		
		if (format == cybxmlYED) {
			res = cyberiada_write_sm_document_yed(copy_doc, writer);
		} else if (format == cybxmlCyberiada10) {
			res = cyberiada_write_sm_document_cyberiada(copy_doc, writer);
		}
		cyberiada_destroy_sm_document(copy_doc);

		if (res != CYBERIADA_NO_ERROR) {
			ERROR("error writing xml %d\n", res);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		res = xmlTextWriterEndDocument(writer);
		if (res < 0) {
			ERROR("error writing xml end document: %d\n", res);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		res = CYBERIADA_NO_ERROR;
	} while (0);
	
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	} else {
		return CYBERIADA_NO_ERROR;
	}
}

int cyberiada_write_sm_document(CyberiadaDocument* doc, const char* filename,
								CyberiadaXMLFormat format, int flags)
{
	int res;
	xmlTextWriterPtr writer = NULL;	
	xmlInitParser();
	writer = xmlNewTextWriterFilename(filename, 0);
	if (!writer) {
		ERROR("cannot open xml writter for file %s\n", filename);
		xmlCleanupParser();
		return CYBERIADA_XML_ERROR;
	}

	res = cyberiada_process_encode_sm_document(doc, writer, format, flags);
	
	xmlFreeTextWriter(writer);
	xmlCleanupParser();

	return res;
}

int cyberiada_encode_sm_document(CyberiadaDocument* doc, char** buffer, size_t* buffer_size,
								 CyberiadaXMLFormat format, int flags)
{
	int res;
	xmlBufferPtr xml_buffer;
	xmlTextWriterPtr writer = NULL;	

	xmlInitParser();
	xml_buffer = xmlBufferCreate();
	if (!xml_buffer) {
		ERROR("cannot create xml buffer\n");
		xmlCleanupParser();
		return CYBERIADA_XML_ERROR;
	}
	writer = xmlNewTextWriterMemory(xml_buffer, 0);
	if (!writer) {
		ERROR("cannot create buffer writter\n");
		xmlBufferFree(xml_buffer);
		xmlCleanupParser();
		return CYBERIADA_XML_ERROR;
	}

 	res = cyberiada_process_encode_sm_document(doc, writer, format, flags);

	if (res == CYBERIADA_NO_ERROR) {
		size_t size = xml_buffer->use;
		*buffer = (char*)malloc(size + 1);
		memcpy(*buffer, xml_buffer->content, size);
		(*buffer)[size] = 0;
		*buffer_size = size;
	} else {
		*buffer = NULL;
		*buffer_size = 0;
	}
	
	xmlFreeTextWriter(writer);
	xmlBufferFree(xml_buffer);
	xmlCleanupParser();

	return res;
}
