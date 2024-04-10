/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The C library implementation
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <regex.h>
#include <stddef.h>

#include "cyberiadaml.h"
#include "utf8enc.h"
#include "cyb_types.h"

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
#define CYBERIADA_STANDARD_VERSION_CYBERIADAML   "1.0"

/* HSM action constants */

#define CYBERIADA_NEWLINE                      "\n\n"
#define CYBERIADA_NEWLINE_RN                   "\r\n\r\n"
#define CYBERIADA_ACTION_TRIGGER_ENTRY         "entry"
#define CYBERIADA_ACTION_TRIGGER_EXIT          "exit"
#define CYBERIADA_ACTION_TRIGGER_DO            "do"
#define CYBERIADA_ACTION_EDGE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*))?\\s*$"
#define CYBERIADA_ACTION_NODE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*)?)\\s*$"
#define CYBERIADA_ACTION_REGEXP_MATCHES        10
#define CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER  1
#define CYBERIADA_ACTION_REGEXP_MATCH_GUARD    6
#define CYBERIADA_ACTION_REGEXP_MATCH_PROP     7
#define CYBERIADA_ACTION_REGEXP_MATCH_ACTION   9
#define CYBERIADA_ACTION_SPACES_REGEXP         "^\\s*$"
/*#define CYBERIADA_ACTION_NEWLINE_REGEXP        "^([^\n]*(\n[ \t\r]*[^\\s])?)*\n\\s*\n(.*)?$"
  #define CYBERIADA_ACTION_NL_REGEXP_MATCHES     4*/

/* CybediadaML metadata constants */

#define CYBERIADA_META_NODE_DEFAULT_ID           "nMeta"
#define CYBERIADA_META_NODE_TITLE                "CGML_META"
#define CYBERIADA_META_SEPARATOR_CHR             '/'
#define CYBERIADA_META_NEW_LINE_CHR              '\n'
#define CYBERIADA_META_NEW_LINE_STR              "\n"
#define CYBERIADA_META_STANDARD_VERSION          "standardVersion"
#define CYBERIADA_META_PLATFORM_NAME             "platform"
#define CYBERIADA_META_PLATFORM_VERSION          "platformVersion"
#define CYBERIADA_META_PLATFORM_LANGUAGE         "platformLanguage"
#define CYBERIADA_META_TARGET_SYSTEM             "target"
#define CYBERIADA_META_NAME                      "name"
#define CYBERIADA_META_AUTHOR                    "author"
#define CYBERIADA_META_CONTACT                   "contact"
#define CYBERIADA_META_DESCRIPTION               "description"
#define CYBERIADA_META_VERSION                   "version"
#define CYBERIADA_META_DATE                      "date"
#define CYBERIADA_META_TRANSITION_ORDER          "transitionOrder"
#define CYBERIADA_META_AO_TRANSITION             "transitionFirst"
#define CYBERIADA_META_AO_EXIT                   "exitFirst"
#define CYBERIADA_META_EVENT_PROPAGATION         "eventPropagation"
#define CYBERIADA_META_EP_PROPAGATE              "propagate"
#define CYBERIADA_META_EP_BLOCK                  "block"
#define CYBERIADA_META_MARKUP_LANGUAGE           "markupLanguage"

/* Misc. constants */

#define MAX_STR_LEN							     4096
#define PSEUDO_NODE_SIZE					     20
#define EMPTY_TITLE                              ""

#ifdef __DEBUG__
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

/* Types & macros for XML/GraphML processing */

#define INDENT_STR      "  "
#define XML_WRITE_OPEN_E(w, e)                         if ((res = xmlTextWriterStartElement(w, (const xmlChar *)e)) < 0) {	\
		                                                   fprintf(stderr, "xml open element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
#define XML_WRITE_OPEN_E_I(w, e, indent)               xmlTextWriterWriteRaw(w, (const xmlChar *)"\n"); \
                                                       for (size_t _i = 0; _i < indent; _i++) { \
                                                           xmlTextWriterWriteRaw(w, (const xmlChar *)INDENT_STR); \
                                                       } \
                                                       if ((res = xmlTextWriterStartElement(w, (const xmlChar *)e)) < 0) {	\
		                                                   fprintf(stderr, "xml open element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }
#define XML_WRITE_OPEN_E_NS_I(w, e, ns, indent)        xmlTextWriterWriteRaw(w, (const xmlChar *)"\n"); \
                                                       for (size_t _i = 0; _i < indent; _i++) { \
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
                                                       for (size_t _i = 0; _i < indent; _i++) { \
                                                           xmlTextWriterWriteRaw(w, (const xmlChar *)INDENT_STR); \
                                                       } \
                                                       if ((res = xmlTextWriterEndElement(w)) < 0) { \
		                                                   fprintf(stderr, "xml close element error %d at %s:%d", res, __FILE__, __LINE__); \
												           return CYBERIADA_XML_ERROR; \
                                                       }

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
 * Utility functions
 * ----------------------------------------------------------------------------- */

static int cyberiada_copy_string(char** target, size_t* size, const char* source)
{
	char* target_str;
	size_t strsize;
	if (!source) {
		*target = NULL;
		*size = 0;
		return CYBERIADA_NO_ERROR;
	}
	strsize = strlen(source);  
	if (strsize > MAX_STR_LEN - 1) {
		strsize = MAX_STR_LEN - 1;
	}
	target_str = (char*)malloc(strsize + 1);
	strncpy(target_str, source, strsize);
	target_str[strsize] = 0;
	*target = target_str;
	if (size) {
		*size = strsize;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_string_is_empty(const char* s)
{
	while(*s) {
		if (!isspace(*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

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

static CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id)
{
	CyberiadaNode* node;
	CyberiadaNode* found;
	for (node = root; node; node = node->next) {
		if (strcmp(node->id, id) == 0) {
			return node;
		}
		if (node->children) {
			found = cyberiada_graph_find_node_by_id(node->children, id);
			if (found) return found;
		}
	}
	return NULL;
}

static CyberiadaNode* cyberiada_graph_find_node_by_type(CyberiadaNode* root, CyberiadaNodeTypeMask mask)
{
	CyberiadaNode* node;
	CyberiadaNode* found;
	if (root->type & mask) {
		return root;
	}
	for (node = root->next; node; node = node->next) {
		found = cyberiada_graph_find_node_by_type(node, mask);
		if (found)
			return found;
	}
	if (root->children) {
		return cyberiada_graph_find_node_by_type(root->children, mask);
	}
	return NULL;
}

static CyberiadaEdge* cyberiada_graph_find_edge_by_id(CyberiadaEdge* root, const char* id)
{
	CyberiadaEdge* edge;
	for (edge = root; edge; edge = edge->next) {
		if (edge->id && strcmp(edge->id, id) == 0) {
			return edge;
		}
	}
	return NULL;
}

static CyberiadaCommentData* cyberiada_new_comment_data(void)
{
	CyberiadaCommentData* cd = (CyberiadaCommentData*)malloc(sizeof(CyberiadaCommentData));
	memset(cd, 0, sizeof(CyberiadaCommentData));
	return cd;
}

static CyberiadaLink* cyberiada_new_link(const char* ref)
{
	CyberiadaLink* link = (CyberiadaLink*)malloc(sizeof(CyberiadaLink));
	memset(link, 0, sizeof(CyberiadaLink));
	cyberiada_copy_string(&(link->ref), &(link->ref_len), ref);
	return link;
}

static CyberiadaNode* cyberiada_new_node(const char* id)
{
	CyberiadaNode* new_node = (CyberiadaNode*)malloc(sizeof(CyberiadaNode));
	memset(new_node, 0, sizeof(CyberiadaNode));
	cyberiada_copy_string(&(new_node->id), &(new_node->id_len), id);
	new_node->type = cybNodeSimpleState;
	return new_node;
}

static int cyberiada_destroy_action(CyberiadaAction* action)
{
	CyberiadaAction* a;
	if (action != NULL) {
		do {
			a = action;
			if (a->trigger) free(a->trigger);
			if (a->guard) free(a->guard);
			if (a->behavior) free(a->behavior);
			action = a->next;
			free(a);
		} while (action);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_all_nodes(CyberiadaNode* node);

static int cyberiada_destroy_node(CyberiadaNode* node)
{
	if(node != NULL) {
		if (node->id) free(node->id);
		if (node->title) free(node->title);
		if (node->children) {
			cyberiada_destroy_all_nodes(node->children);
		}
		if (node->actions) cyberiada_destroy_action(node->actions);
		if (node->geometry_point) free(node->geometry_point);
		if (node->geometry_rect) free(node->geometry_rect);
		if (node->color) free(node->color);
		if (node->link) {
			if (node->link->ref) free(node->link->ref);
			free(node->link);
		}
		if (node->comment_data) {
			if (node->comment_data->body) free(node->comment_data->body);
			if (node->comment_data->markup) free(node->comment_data->markup);
			free(node->comment_data);
		}
		free(node);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_all_nodes(CyberiadaNode* node)
{
	CyberiadaNode* n;
	if(node != NULL) {
		do {
			n = node;
			node = node->next;
			cyberiada_destroy_node(n);
		} while (node);
	}
	return CYBERIADA_NO_ERROR;
}

static CyberiadaCommentSubject* cyberiada_new_comment_subject(CyberiadaCommentSubjectType type)
{
	CyberiadaCommentSubject* cs = (CyberiadaCommentSubject*)malloc(sizeof(CyberiadaCommentSubject));
	memset(cs, 0, sizeof(CyberiadaCommentSubject));
	cs->type = type;
	return cs;
}

static CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target)
{
	CyberiadaEdge* new_edge = (CyberiadaEdge*)malloc(sizeof(CyberiadaEdge));
	memset(new_edge, 0, sizeof(CyberiadaEdge));
	new_edge->type = cybEdgeTransition;
	cyberiada_copy_string(&(new_edge->id), &(new_edge->id_len), id);
	cyberiada_copy_string(&(new_edge->source_id), &(new_edge->source_id_len), source);
	cyberiada_copy_string(&(new_edge->target_id), &(new_edge->target_id_len), target);
	return new_edge;
}

static CyberiadaAction* cyberiada_new_action(CyberiadaActionType type,
												 const char* trigger,
												 const char* guard,
												 const char* behavior)
{
	CyberiadaAction* action = (CyberiadaAction*)malloc(sizeof(CyberiadaAction));
	memset(action, 0, sizeof(CyberiadaAction));
	action->type = type;
	cyberiada_copy_string(&(action->trigger), &(action->trigger_len), trigger);
	cyberiada_copy_string(&(action->guard), &(action->guard_len), guard);
	cyberiada_copy_string(&(action->behavior), &(action->behavior_len), behavior);
	return action;
}

static regex_t cyberiada_edge_action_regexp;
static regex_t cyberiada_node_action_regexp;
/*static regex_t cyberiada_newline_regexp;*/
static regex_t cyberiada_spaces_regexp;

static int cyberiada_init_action_regexps(void)
{
	if (regcomp(&cyberiada_edge_action_regexp, CYBERIADA_ACTION_EDGE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile edge action regexp\n");
		return CYBERIADA_ASSERT;
	}
	if (regcomp(&cyberiada_node_action_regexp, CYBERIADA_ACTION_NODE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile edge action regexp\n");
		return CYBERIADA_ASSERT;
	}
/*	if (regcomp(&cyberiada_newline_regexp, CYBERIADA_ACTION_NEWLINE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile new line regexp\n");
		return CYBERIADA_ASSERT;
		}*/
	if (regcomp(&cyberiada_spaces_regexp, CYBERIADA_ACTION_SPACES_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile new line regexp\n");
		return CYBERIADA_ASSERT;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_free_action_regexps(void)
{
	regfree(&cyberiada_edge_action_regexp);
	regfree(&cyberiada_node_action_regexp);
/*	regfree(&cyberiada_newline_regexp);*/
	regfree(&cyberiada_spaces_regexp);
	return CYBERIADA_NO_ERROR;
}

static int cyberiaga_matchres_action_regexps(const char* text,
											   const regmatch_t* pmatch, size_t pmatch_size,
											   char** trigger, char** guard, char** behavior)
{
	size_t i;
	char* part;
	int start, end;
		
	if (pmatch_size != CYBERIADA_ACTION_REGEXP_MATCHES) {
		ERROR("bad action regexp match array size\n");
		return CYBERIADA_ASSERT;
	}

	for (i = 0; i < pmatch_size; i++) {
		if (i != CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER &&
			i != CYBERIADA_ACTION_REGEXP_MATCH_GUARD &&
			i != CYBERIADA_ACTION_REGEXP_MATCH_ACTION) {
			continue;
		}
		start = pmatch[i].rm_so;
		end = pmatch[i].rm_eo;
		if (end > start && text[start] != 0) {
			part = (char*)malloc((size_t)(end - start + 1));
			strncpy(part, &text[start], (size_t)(end - start));
			part[(size_t)(end - start)] = 0;
		} else {
			part = "";
		}
		if (i == CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER) {
			*trigger = part;
		} else if (i == CYBERIADA_ACTION_REGEXP_MATCH_GUARD) {
			*guard = part;
		} else {
			/* i == ACTION_REGEXP_MATCH_ACTION */
			*behavior= part;
		}
	}

	return CYBERIADA_NO_ERROR;
}

/*static int cyberiaga_matchres_newline(const regmatch_t* pmatch, size_t pmatch_size,
									  size_t* next_block)
{
	if (pmatch_size != ACTION_NL_REGEXP_MATCHES) {
		ERROR("bad new line regexp match array size\n");
		return CYBERIADA_ASSERT;
	}
	if (next_block) {
		*next_block = (size_t)pmatch[pmatch_size - 1].rm_so;
	}

	return CYBERIADA_NO_ERROR;
	}*/

static int decode_utf8_strings(char** trigger, char** guard, char** behavior)
{
	char* oldptr;
	size_t len;
	if (*trigger && **trigger) {
		oldptr = *trigger;
		*trigger = utf8_decode(*trigger, strlen(*trigger), &len);
		free(oldptr);
	}	
	if (*guard && **guard) {
		oldptr = *guard;
		*guard = utf8_decode(*guard, strlen(*guard), &len);
		free(oldptr);
	}	
	if (*behavior && **behavior) {
		oldptr = *behavior;
		*behavior = utf8_decode(*behavior, strlen(*behavior), &len);
		free(oldptr);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_edge_action(const char* text, CyberiadaAction** action)
{
	int res;
	size_t buffer_len;
	char *trigger = "", *guard = "", *behavior = "";
	char *buffer;
	regmatch_t pmatch[CYBERIADA_ACTION_REGEXP_MATCHES];

	buffer = utf8_encode(text, strlen(text), &buffer_len);
	
	if ((res = regexec(&cyberiada_edge_action_regexp, buffer,
					   CYBERIADA_ACTION_REGEXP_MATCHES, pmatch, 0)) != 0) {
		if (res == REG_NOMATCH) {
			ERROR("edge action text didn't match the regexp\n");
			return CYBERIADA_ACTION_FORMAT_ERROR;
		} else {
			ERROR("edge action regexp error %d\n", res);
			return CYBERIADA_ASSERT;
		}
	}
	if (cyberiaga_matchres_action_regexps(buffer,
											pmatch, CYBERIADA_ACTION_REGEXP_MATCHES,
											&trigger, &guard, &behavior) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_ASSERT;
	}

	decode_utf8_strings(&trigger, &guard, &behavior);
	
/*	DEBUG("\n");
	DEBUG("edge action:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("action: %s\n", action);*/

	if (*trigger || *guard || *behavior) {
		*action = cyberiada_new_action(cybActionTransition, trigger, guard, behavior);
	} else {
		*action = NULL;
	}
	
	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*behavior) free(behavior);

	free(buffer);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_add_action(const char* trigger, const char* guard, const char* behavior,
								  CyberiadaAction** action)
{
	CyberiadaAction* new_action;
	CyberiadaActionType type;

	if (trigger) {
		if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_ENTRY) == 0) {
			type = cybActionEntry;
		} else if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_EXIT) == 0) {
			type = cybActionExit;
		} else if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_DO) == 0) {
			type = cybActionDo;
		} else {
			type = cybActionTransition;
		}
	}
	
	/*DEBUG("\n");
	DEBUG("node action:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("behavior: %s\n", behavior);
	DEBUG("type: %d\n", type);*/

	new_action = cyberiada_new_action(type, trigger, guard, behavior);
	if (*action) {
		CyberiadaAction* b = *action;
		while (b->next) b = b->next;
		b->next = new_action;
	} else {
		*action = new_action;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_state_block_action(const char* text, CyberiadaAction** action)
{
	int res;
	char *trigger = "", *guard = "", *behavior = "";
	regmatch_t pmatch[CYBERIADA_ACTION_REGEXP_MATCHES];
	if ((res = regexec(&cyberiada_node_action_regexp, text,
					   CYBERIADA_ACTION_REGEXP_MATCHES, pmatch, 0)) != 0) {
		if (res == REG_NOMATCH) {
			ERROR("node block action text didn't match the regexp\n");
			return CYBERIADA_ACTION_FORMAT_ERROR;
		} else {
			ERROR("node block action regexp error %d\n", res);
			return CYBERIADA_ASSERT;
		}
	}
	if (cyberiaga_matchres_action_regexps(text,
											pmatch, CYBERIADA_ACTION_REGEXP_MATCHES,
											&trigger, &guard, &behavior) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_ASSERT;
	}

	decode_utf8_strings(&trigger, &guard, &behavior);
	cyberiada_add_action(trigger, guard, behavior, action);

	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*behavior) free(behavior);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_state_actions(const char* text, CyberiadaAction** action)
{
	int res;
	size_t buffer_len;
	char *buffer, *start, *block, *block2, *next;
/*	regmatch_t pmatch[ACTION_NL_REGEXP_MATCHES];*/
	
	buffer = utf8_encode(text, strlen(text), &buffer_len);
	next = buffer;

	*action = NULL;
	
	while (*next) {
		start = next;
		block = strstr(start, CYBERIADA_NEWLINE);
		block2 = strstr(start, CYBERIADA_NEWLINE_RN);
		if (block2 && ((block && (block > block2)) || !block)) {
			block = block2;
			*block2 = 0;
			next = block2 + 4;
		} else if (block) {
			*block = 0;
			next = block + 2;
		} else {
			block = start;
			next = start + strlen(block);
		}
/*		if ((res = regexec(&cyberiada_newline_regexp, start,
						   ACTION_NL_REGEXP_MATCHES, pmatch, 0)) != 0) {
			if (res != REG_NOMATCH) {
				ERROR("newline regexp error %d\n", res);
				return CYBERIADA_ASSERT;
			}
		}
		if (res == REG_NOMATCH) {
			block = start;
			start = start + strlen(block);
		} else {
			if (cyberiaga_matchres_newline(pmatch, ACTION_NL_REGEXP_MATCHES,
										   &next_block) != CYBERIADA_NO_ERROR) {
				return CYBERIADA_ASSERT;
			}
			block = start;
			start[next_block - 1] = 0;
			start = start + next_block;
			DEBUG("first part: '%s'\n", block);
			DEBUG("second part: '%s'\n", start);
			}*/
		
		if (regexec(&cyberiada_spaces_regexp, start, 0, NULL, 0) == 0) {
			continue ;
		}

		if ((res = cyberiada_decode_state_block_action(start, action)) != CYBERIADA_NO_ERROR) {
			ERROR("error while decoding state block %s: %d\n", start, res);
			return res;
		}
	}
	
	free(buffer);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_graph_add_sibling_node(CyberiadaNode* sibling, CyberiadaNode* new_node)
{
	CyberiadaNode* node = sibling;
	if (!new_node) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_node->parent = sibling->parent;
	while (node->next) node = node->next;
	node->next = new_node;
	return CYBERIADA_NO_ERROR;
}

/*static int cyberiada_graph_add_node_action(CyberiadaNode* node,
											 CyberiadaActionType type,
											 const char* trigger,
											 const char* guard,
											 const char* action)
{
	CyberiadaAction *action, *new_action;
	if (!node) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_action = cyberiada_new_action(type, trigger, guard, action);
	if (node->action) {
		node->action = new_action;
	} else {
		action = node->action;
		while (action->next) action = action->next;
		action->next = new_action;
	}
	return CYBERIADA_NO_ERROR;
	}*/

 /*static int cyberiada_find_and_remove_node(CyberiadaNode* current, CyberiadaNode* target)
{
	int res;
	CyberiadaNode* next = NULL;
	CyberiadaNode* prev = NULL;
	if (current)
	{
		do {
			next = current->next;
			if (current == target) {
				if (!prev) {
					return CYBERIADA_BAD_PARAMETER;
				}
				prev->next = next;
				target->next = NULL;
				return CYBERIADA_NO_ERROR;
			} else if (current->children == target) {
				current->children = current->children->next;
				target->next = NULL;
				return CYBERIADA_NO_ERROR;
			} else if (current->children) {
				res = cyberiada_find_and_remove_node(current->children, target);
				if (res == CYBERIADA_NO_ERROR) {
					return res;
				}
			}
			prev = current;
			current = next;
		} while (current);
	}
	return CYBERIADA_NOT_FOUND;
	}*/

  /*static int cyberiada_remove_node(CyberiadaSM* sm, CyberiadaNode* node)
{
	if (node == NULL) {
		return CYBERIADA_NO_ERROR;
	} else if (sm->nodes == node) {
		sm->nodes = NULL;
		return CYBERIADA_NO_ERROR;
	} else {
		return cyberiada_find_and_remove_node(sm->nodes, node);
	}
	}*/

static int cyberiada_graph_add_edge(CyberiadaSM* sm, const char* id, const char* source, const char* target)
{
	CyberiadaEdge* last_edge;
	CyberiadaEdge* new_edge;
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_edge = cyberiada_new_edge(id, source, target);
	last_edge = sm->edges;
	if (last_edge == NULL) {
		sm->edges = new_edge;
	} else {
		while (last_edge->next) last_edge = last_edge->next;
		last_edge->next = new_edge;
	}
	return CYBERIADA_NO_ERROR;
}

static CyberiadaEdge* cyberiada_graph_find_last_edge(CyberiadaSM* sm)
{
	CyberiadaEdge* edge;
	if (!sm) {
		return NULL;
	}
	edge = sm->edges;
	while (edge && edge->next) edge = edge->next;	
	return edge;
}

/*static int cyberiada_graph_add_edge_action(CyberiadaEdge* edge,
															CyberiadaActionType type,
															const char* trigger,
															const char* guard,
															const char* action)
{
	CyberiadaAction *action, *new_action;
	if (!edge) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_action = cyberiada_new_action(type, trigger, guard, action);
	if (edge->action) {
		edge->action = new_action;
	} else {
		action = edge->action;
		while (action->next) action = action->next;
		action->next = new_action;
	}
	return CYBERIADA_NO_ERROR;
	}*/

static int cyberiada_destroy_edge(CyberiadaEdge* e)
{
	CyberiadaPolyline *polyline, *pl;
	if (e->id) free(e->id);
	if (e->source_id) free(e->source_id);
	if (e->target_id) free(e->target_id);
	if (e->action) cyberiada_destroy_action(e->action);
	if (e->comment_subject) {
		if (e->comment_subject->fragment) free(e->comment_subject->fragment);
		free(e->comment_subject);
	}
	if (e->geometry_polyline) {
		polyline = e->geometry_polyline;
		do {
			pl = polyline;
			polyline = polyline->next;
			free(pl);
		} while (polyline);
	}
	if (e->geometry_source_point) free(e->geometry_source_point); 
	if (e->geometry_target_point) free(e->geometry_target_point);
	if (e->geometry_label_point) free(e->geometry_label_point);
	if (e->color) free(e->color);
	free(e);
	return CYBERIADA_NO_ERROR;
}

typedef CyberiadaList NamesList;

static int cyberiada_add_name_to_list(NamesList** nl, const char* from, const char* to)
{
	if (!nl) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_list_add(nl, from, (void*)to);
	return CYBERIADA_NO_ERROR;
}

static const char* cyberiada_find_name_in_list(NamesList** nl, const char* name)
{
	void* result;
	if (!nl) {
		return NULL;
	}
	result = cyberiada_list_find(nl, name);
	if (result == NULL) {
		return NULL;
	} else {
		return (const char*)result;
	}
}

static void cyberiada_free_name_list(NamesList** nl)
{
	cyberiada_list_free(nl);
}

static int cyberiada_graphs_reconstruct_nodes(CyberiadaNode* root, NamesList** nl)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode *node;
	unsigned int num = 0;
	
	node = root;
	while (node) {
		if (!node->id) {
			ERROR("Found null node id\n");
			return CYBERIADA_FORMAT_ERROR;
		}
		if (!*(node->id)) {
			do {
				if (node->parent && node->parent->parent) {
					snprintf(buffer, buffer_len, "%s::n%u",
							 node->parent->id, num);
				} else {
					snprintf(buffer, buffer_len, "n%u", num);
				}
				num++;
			} while (cyberiada_graph_find_node_by_id(root, buffer));
			free(node->id);
			node->id = NULL;
			cyberiada_copy_string(&(node->id), &(node->id_len), buffer);
			cyberiada_add_name_to_list(nl, "", node->id);
		}
		if (node->children) {
			cyberiada_graphs_reconstruct_nodes(node->children, nl);
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_graphs_reconstruct_edges(CyberiadaDocument* doc, NamesList** nl)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaEdge *edge;
	CyberiadaSM* sm;
	unsigned int num = 0;
	const char* new_id;
	
	for (sm = doc->state_machines; sm; sm = sm->next) {

		edge = sm->edges;
		while (edge) {
			if (!*(edge->source_id)) {
				new_id = cyberiada_find_name_in_list(nl, edge->source_id);
				if (!new_id) {
					ERROR("Cannot find replacement for source id %s\n", edge->source_id);
					return CYBERIADA_FORMAT_ERROR;
				}
				free(edge->source_id);
				edge->source_id = NULL;
				cyberiada_copy_string(&(edge->source_id), &(edge->source_id_len), new_id);
			}
			if (!*(edge->target_id)) {
				new_id = cyberiada_find_name_in_list(nl, edge->target_id);
				if (!new_id) {
					ERROR("Cannot find replacement for target id %s\n", edge->target_id);
					return CYBERIADA_FORMAT_ERROR;
				}
				free(edge->target_id);
				edge->target_id = NULL;
				cyberiada_copy_string(&(edge->target_id), &(edge->target_id_len), new_id);
			}
			edge = edge->next;
		}
		
		edge = sm->edges;
		while (edge) {
			CyberiadaNode* source = cyberiada_graph_find_node_by_id(sm->nodes, edge->source_id);
			CyberiadaNode* target = cyberiada_graph_find_node_by_id(sm->nodes, edge->target_id);
			if (!source || !target) {
				ERROR("cannot find source/target node for edge %s %s\n", edge->source_id, edge->target_id);
				return CYBERIADA_FORMAT_ERROR;
			}
			if (!edge->id || !*(edge->id)) {
				snprintf(buffer, buffer_len, "%s-%s", edge->source_id, edge->target_id);
				while (cyberiada_graph_find_edge_by_id(sm->edges, buffer)) {
					snprintf(buffer, buffer_len, "%s-%s#%u",
							 edge->source_id, edge->target_id, num);
					num++;
				}
				free(edge->id);
				edge->id = NULL;
				cyberiada_copy_string(&(edge->id), &(edge->id_len), buffer);
			}
			edge->source = source;
			edge->target = target;
			edge = edge->next;
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_meta(CyberiadaMetainformation* meta)
{
	if (meta) {
		if (meta->standard_version) free(meta->standard_version);
		if (meta->platform_name) free(meta->platform_name);
		if (meta->platform_version) free(meta->platform_version);
		if (meta->platform_language) free(meta->platform_language);
		if (meta->target_system) free(meta->target_system);
		if (meta->name) free(meta->name);
		if (meta->author) free(meta->author);
		if (meta->contact) free(meta->contact);
		if (meta->description) free(meta->description);
		if (meta->version) free(meta->version);
		if (meta->date) free(meta->date);
		if (meta->markup_language) free(meta->markup_language);
		free(meta);
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library fucntions declarations
 * ----------------------------------------------------------------------------- */

static int cyberiada_init_sm(CyberiadaSM* sm)
{
	if (sm) {
		sm->nodes = NULL;
		sm->edges = NULL;
		sm->next = NULL;
	}
	return CYBERIADA_NO_ERROR;
}

static CyberiadaSM* cyberiada_create_sm(void)
{
	CyberiadaSM* sm = (CyberiadaSM*)malloc(sizeof(CyberiadaSM));
	cyberiada_init_sm(sm);
	return sm;
}

CyberiadaDocument* cyberiada_create_sm_document(void)
{
	CyberiadaDocument* doc = (CyberiadaDocument*)malloc(sizeof(CyberiadaDocument));
	cyberiada_init_sm_document(doc);
	return doc;
}

int cyberiada_init_sm_document(CyberiadaDocument* doc)
{
	if (doc) {
		doc->format = NULL;
		doc->meta_info = NULL;
		doc->state_machines = NULL;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_sm(CyberiadaSM* sm)
{
	CyberiadaEdge *edge, *e;
	if (sm) {
		if (sm->nodes) {
			cyberiada_destroy_all_nodes(sm->nodes);
		}
		if (sm->edges) {
			edge = sm->edges;
			do {
				e = edge;
				edge = edge->next;
				cyberiada_destroy_edge(e);
			} while (edge);
		}
		free(sm);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_cleanup_sm_document(CyberiadaDocument* doc)
{
	CyberiadaSM *sm, *sm2;
	if (doc) {
		if (doc->format) {
			free(doc->format);
		}
		if (doc->meta_info) {
			cyberiada_destroy_meta(doc->meta_info);
		}
		if (doc->state_machines) {
			sm = doc->state_machines;
			do {
				sm2 = sm;
				sm = sm->next;
				cyberiada_destroy_sm(sm2);
			} while (sm);
		}
		cyberiada_init_sm_document(doc);
	}
	return CYBERIADA_NO_ERROR;	
}

int cyberiada_destroy_sm_document(CyberiadaDocument* doc)
{
	int res = cyberiada_cleanup_sm_document(doc);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}
	free(doc);
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
									float* result)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 attr_name) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_BAD_PARAMETER;
	}
	*result = (float)atof(buffer);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_xml_read_point(xmlNode* xml_node,
									CyberiadaPoint** point)
{
	CyberiadaPoint* p = (CyberiadaPoint*)malloc(sizeof(CyberiadaPoint));
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
	CyberiadaRect* r = (CyberiadaRect*)malloc(sizeof(CyberiadaRect));
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
											NodeStack** stack)
{
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
			sm->next = cyberiada_create_sm();
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
										   NodeStack** stack)
{
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
										   NodeStack** stack)
{
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
	DEBUG("add edge '%s' '%s' -> '%s'\n", buffer, source_buffer, target_buffer);
	cyberiada_graph_add_edge(sm, buffer, source_buffer, target_buffer);
	return gpsEdge;
}

static GraphProcessorState handle_edge_point(xmlNode* xml_node,
											 CyberiadaDocument* doc,
											 NodeStack** stack)
{
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
	pl = (CyberiadaPolyline*)malloc(sizeof(CyberiadaPolyline));
	memset(pl, 0, sizeof(CyberiadaPolyline));
	pl->point.x = p->x;
	pl->point.y = p->y;
	free(p);
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
											 NodeStack** stack)
{
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
											   NodeStack** stack)
{
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
											   NodeStack** stack)
{
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
												NodeStack** stack)
{
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
	if (type == cybNodeInitial) {
		current->geometry_point = (CyberiadaPoint*)malloc(sizeof(CyberiadaPoint));
		current->geometry_point->x = rect->x + rect->width / 2.0f;
		current->geometry_point->y = rect->y + rect->height / 2.0f;
		free(rect);
		return gpsNodeStart;
	} else if (type == cybNodeComment) {
		current->geometry_rect = rect;
		return gpsNodeAction;
	} else {
		current->geometry_rect = rect;
		return gpsNodeTitle;
	}
}

static GraphProcessorState handle_property(xmlNode* xml_node,
										   CyberiadaDocument* doc,
										   NodeStack** stack)
{
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
											NodeStack** stack)
{
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
	/* DEBUG("Set node %s title %s\n", current->id, buffer); */
	cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
	return gpsNodeAction;
}

static GraphProcessorState handle_node_action(xmlNode* xml_node,
												CyberiadaDocument* doc,
												NodeStack** stack)
{
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
		if (cyberiada_decode_state_actions(buffer, &(current->actions)) != CYBERIADA_NO_ERROR) {
			ERROR("cannot decode node action\n");
			return gpsInvalid;
		}
	}
	return gpsGraph;
}

static GraphProcessorState handle_edge_geometry(xmlNode* xml_node,
												CyberiadaDocument* doc,
												NodeStack** stack)
{
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	current->geometry_source_point = (CyberiadaPoint*)malloc(sizeof(CyberiadaPoint));
	current->geometry_target_point = (CyberiadaPoint*)malloc(sizeof(CyberiadaPoint));
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
		free(current->geometry_source_point);
		free(current->geometry_target_point);
		current->geometry_source_point = NULL;
		current->geometry_target_point = NULL;
		return gpsInvalid;
	}
	return gpsEdgeGeometry;
}

static GraphProcessorState handle_edge_label(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
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
	if (cyberiada_decode_edge_action(buffer, &(current->action)) != CYBERIADA_NO_ERROR) {
		ERROR("cannot decode edge action\n");
		return gpsInvalid;
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

typedef struct {
	const char* name;
	size_t value_offset;
	size_t len_offset;
	const char* title;
} MetainfoDeclaration;

static MetainfoDeclaration cyberiada_metadata[] = {
	{
		CYBERIADA_META_STANDARD_VERSION,
		offsetof(CyberiadaMetainformation, standard_version),
		offsetof(CyberiadaMetainformation, standard_version_len),
		"standard version"
	}, {
		CYBERIADA_META_PLATFORM_NAME,
		offsetof(CyberiadaMetainformation, platform_name),
		offsetof(CyberiadaMetainformation, platform_name_len),
		"platform name"
	}, {
		CYBERIADA_META_PLATFORM_VERSION,
		offsetof(CyberiadaMetainformation, platform_version),
		offsetof(CyberiadaMetainformation, platform_version_len),
		"platform version"
	}, {
		CYBERIADA_META_PLATFORM_LANGUAGE,
		offsetof(CyberiadaMetainformation, platform_language),
		offsetof(CyberiadaMetainformation, platform_language_len),
		"platform language"
	}, {
		CYBERIADA_META_TARGET_SYSTEM,
		offsetof(CyberiadaMetainformation, target_system),
		offsetof(CyberiadaMetainformation, target_system_len),
		"target system"
	}, {
		CYBERIADA_META_NAME,
		offsetof(CyberiadaMetainformation, name),
		offsetof(CyberiadaMetainformation, name_len),
		"document name"
	}, {
		CYBERIADA_META_AUTHOR,
		offsetof(CyberiadaMetainformation, author),
		offsetof(CyberiadaMetainformation, author_len),
		"document author"
	}, {
		CYBERIADA_META_CONTACT,
		offsetof(CyberiadaMetainformation, contact),
		offsetof(CyberiadaMetainformation, contact_len),
		"document author's contact"
	}, {
		CYBERIADA_META_DESCRIPTION,
		offsetof(CyberiadaMetainformation, description),
		offsetof(CyberiadaMetainformation, description_len),
		"document description"
	}, {
		CYBERIADA_META_VERSION,
		offsetof(CyberiadaMetainformation, version),
		offsetof(CyberiadaMetainformation, version_len),
		"document version"
	}, {
		CYBERIADA_META_DATE,
		offsetof(CyberiadaMetainformation, date),
		offsetof(CyberiadaMetainformation, date_len),
		"document date"
	}, {
		CYBERIADA_META_MARKUP_LANGUAGE,
		offsetof(CyberiadaMetainformation, markup_language),
		offsetof(CyberiadaMetainformation, markup_language_len),
		"markup language"
	}
};

static int cyberiada_add_default_meta(CyberiadaDocument* doc, const char* sm_name)
{
	CyberiadaMetainformation* meta;
	
	if (doc->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	cyberiada_copy_string(&(meta->standard_version),
						  &(meta->standard_version_len),
						  CYBERIADA_STANDARD_VERSION_CYBERIADAML);
	if (*sm_name) {
		cyberiada_copy_string(&(meta->name), &(meta->name_len), sm_name);
	}
	
	meta->transition_order_flag = 1;
	meta->event_propagation_flag = 1;
	
	doc->meta_info = meta;
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_encode_meta(CyberiadaDocument* doc, char** meta_body, size_t* meta_body_len)
{
	size_t i, buffer_len;
	int written;
	char *buffer, *value;
	CyberiadaMetainformation* meta = doc->meta_info;

	buffer_len = 1;
	for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
		value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
		if (value) {
			buffer_len += (strlen(cyberiada_metadata[i].name) +
						   *(size_t*)((char*)meta + cyberiada_metadata[i].len_offset) +
						   4);
		}
	}
	buffer_len += (strlen(CYBERIADA_META_TRANSITION_ORDER) +
				   (meta->transition_order_flag == 1 ? strlen(CYBERIADA_META_AO_TRANSITION) : strlen(CYBERIADA_META_AO_EXIT)) +
				   strlen(CYBERIADA_META_EVENT_PROPAGATION) +
				   (meta->event_propagation_flag == 1 ? strlen(CYBERIADA_META_EP_PROPAGATE) : strlen(CYBERIADA_META_EP_BLOCK)) + 
				   8);		
	buffer = (char*)malloc(buffer_len);
	*meta_body = buffer;
	for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
		value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
		if (value) {
			written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
							   cyberiada_metadata[i].name,
							   value);
			buffer_len -= (size_t)written;
			buffer += written;
		}
	}
	written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
					   CYBERIADA_META_TRANSITION_ORDER,
					   meta->transition_order_flag == 1 ? CYBERIADA_META_AO_TRANSITION : CYBERIADA_META_AO_EXIT);
	buffer_len -= (size_t)written;
	buffer += written;
	written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
					   CYBERIADA_META_EVENT_PROPAGATION,
					   meta->event_propagation_flag == 1 ? CYBERIADA_META_EP_PROPAGATE : CYBERIADA_META_EP_BLOCK);
	buffer_len -= (size_t)written;
	buffer += written;
	*buffer = 0;
	if (meta_body_len) {
		*meta_body_len = strlen(*meta_body);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_meta(CyberiadaDocument* doc, char* metadata)
{
	CyberiadaMetainformation* meta;
	char  *start, *block, *block2, *next, *parts;
	size_t i;
	char found;
	
	if (doc->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	next = metadata;	
	while (*next) {
		start = next;
		block = strstr(start, CYBERIADA_NEWLINE);
		block2 = strstr(start, CYBERIADA_NEWLINE_RN);
		if (block2 && ((block && (block > block2)) || !block)) {
			block = block2;
			*block2 = 0;
			next = block2 + 4;
		} else if (block) {
			*block = 0;
			next = block + 2;
		} else {
			block = start;
			next = start + strlen(block);
		}
		if (regexec(&cyberiada_spaces_regexp, start, 0, NULL, 0) == 0) {
			continue;
		}

		parts = strchr(start, CYBERIADA_META_SEPARATOR_CHR);
		if (parts == NULL) {
			ERROR("Error decoding SM metainformation: cannot find separator\n");
			cyberiada_destroy_meta(meta);
			return CYBERIADA_METADATA_FORMAT_ERROR;
		}
		*parts = 0;		
		do {
			parts++;
		} while (isspace(*parts));
		
		found = 0;
		for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
			if (strcmp(start, cyberiada_metadata[i].name) == 0) {
				if (*(char**)((char*)meta + cyberiada_metadata[i].value_offset) != NULL) {
					ERROR("Error decoding SM metainformation: double parameter %s\n",
						  cyberiada_metadata[i].title);
					cyberiada_destroy_meta(meta);
					return CYBERIADA_METADATA_FORMAT_ERROR;					
				}
				cyberiada_copy_string((char**)((char*)meta + cyberiada_metadata[i].value_offset),
									  (size_t*)((char*)meta + cyberiada_metadata[i].len_offset), parts);
				found = 1;
				break;
			}
		}
		if (!found) {
			if (strcmp(start, CYBERIADA_META_TRANSITION_ORDER) == 0) {
				if (strcmp(parts, CYBERIADA_META_AO_TRANSITION) == 0) {
					meta->transition_order_flag = 1;
				} else if (strcmp(parts, CYBERIADA_META_AO_EXIT) == 0) {
					meta->transition_order_flag = 2;
				} else {
					ERROR("Error decoding SM metainformation: bad value of actions order flag parameter\n");
					cyberiada_destroy_meta(meta);
					return CYBERIADA_METADATA_FORMAT_ERROR;					
				}
			} else if (strcmp(start, CYBERIADA_META_EVENT_PROPAGATION) == 0) {
				if (strcmp(parts, CYBERIADA_META_EP_PROPAGATE) == 0) {
					meta->event_propagation_flag = 1;
				} else if (strcmp(parts, CYBERIADA_META_EP_BLOCK) == 0) {
					meta->event_propagation_flag = 2;
				} else {
					ERROR("Error decoding SM metainformation: bad value of event propagation flag parameter\n");
					cyberiada_destroy_meta(meta);
					return CYBERIADA_METADATA_FORMAT_ERROR;					
				}
			} else {
				ERROR("Error decoding SM metainformation: bad key %s\n", start);
				cyberiada_destroy_meta(meta);
				return CYBERIADA_METADATA_FORMAT_ERROR;
			}
		}
	}
	
	if (!meta->standard_version) {
		ERROR("Error decoding SM metainformation: standard version is not set\n");
		cyberiada_destroy_meta(meta);
		return CYBERIADA_METADATA_FORMAT_ERROR;
	}

	if (strcmp(meta->standard_version, CYBERIADA_STANDARD_VERSION_CYBERIADAML) != 0) {
		ERROR("Error decoding SM metainformation: unsupported version of Cyberiada standard - %s\n",
			  meta->standard_version);
		cyberiada_destroy_meta(meta);
		return CYBERIADA_METADATA_FORMAT_ERROR;
	}
	
	// set default values
	if (!meta->transition_order_flag) {
		meta->transition_order_flag = 1;
	}
	if (!meta->event_propagation_flag) {
		meta->event_propagation_flag = 1;
	}
	
	doc->meta_info = meta;
	
	return CYBERIADA_NO_ERROR;
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
												NodeStack** stack)
{
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
											   NodeStack** stack)
{
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
											NodeStack** stack)
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
			if (current->type == cybNodeFormalComment &&
				current->title && strcmp(current->title, CYBERIADA_META_NODE_TITLE) == 0) {
				if (cyberiada_decode_meta(doc, buffer) != CYBERIADA_NO_ERROR) {
					ERROR("Error while decoging metainfo comment\n");
					return gpsInvalid;
				}
			} 
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
			if (cyberiada_decode_state_actions(buffer, &(current->actions)) != CYBERIADA_NO_ERROR) {
				ERROR("Cannot decode node action\n");
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
											 NodeStack** stack)
{
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
											NodeStack** stack)
{
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
	return gpsNode;
}

static GraphProcessorState handle_edge_data(xmlNode* xml_node,
											CyberiadaDocument* doc,
											NodeStack** stack)
{
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
		if (cyberiada_decode_edge_action(buffer, &(current->action)) != CYBERIADA_NO_ERROR) {
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
													NodeStack** stack)
{
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
													NodeStack** stack)
{
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
												   NodeStack** stack)
{
	CyberiadaEdge *current;
	CyberiadaSM* sm = doc->state_machines;
	while (sm->next) sm = sm->next;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->geometry_label_point) {
		ERROR("Trying to set edge %s geometry label point twice\n", current->id);
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node,
								 &(current->geometry_label_point)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdge;
}

typedef GraphProcessorState (*GraphProcessorHandler)(xmlNode* xml_root,
													 CyberiadaDocument* doc,
													 NodeStack** stack);

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
	{gpsEdgeGeometry,      GRAPHML_EDGE_ELEMENT,  &handle_new_edge},
	{gpsEdgeGeometry,      GRAPHML_GRAPH_ELEMENT, &handle_new_graph},	
	{gpsEdgeSourcePoint,   GRAPHML_POINT_ELEMENT, &handle_edge_source_point},
	{gpsEdgeTargetPoint,   GRAPHML_POINT_ELEMENT, &handle_edge_target_point},
	{gpsEdgeLabelGeometry, GRAPHML_POINT_ELEMENT, &handle_edge_label_point},
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
							  size_t processor_state_table_size)
{
	size_t i;
	if (xml_node->type == XML_ELEMENT_NODE) {
		const char* xml_element_name = (const char*)xml_node->name;
		node_stack_set_top_element(stack, xml_element_name);
		for (i = 0; i < processor_state_table_size; i++) {
			if (processor_state_table[i].state == *gps &&
				strcmp(xml_element_name, processor_state_table[i].symbol) == 0) {
				*gps = (*(processor_state_table[i].handler))(xml_node, doc, stack);
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
								  size_t processor_state_table_size)
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
						   processor_state_table, processor_state_table_size);
		if (*gps == gpsInvalid) {
			return CYBERIADA_FORMAT_ERROR;
		}
		if (cur_xml_node->children) {
			int res = cyberiada_build_graphs(cur_xml_node->children, doc, stack, gps,
											 processor_state_table, processor_state_table_size);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node_stack_pop(stack);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_yed_xml(xmlNode* root, CyberiadaDocument* doc)
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
	} else {
		cyberiada_copy_string(&(doc->format), &(doc->format_len), CYBERIADA_FORMAT_OSTRANNA);
		berloga_format = 0;
	}
	/* DEBUG("doc format %s\n", doc->format); */
	
	if ((res = cyberiada_build_graphs(root, doc, &stack, &gps,
									  yed_processor_state_table,
									  yed_processor_state_table_size)) != CYBERIADA_NO_ERROR) {
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

static int cyberiada_update_complex_state(CyberiadaNode* node, CyberiadaNode* parent)
{
	if (parent && parent->type == cybNodeSimpleState &&
		node->type != cybNodeComment && node->type != cybNodeFormalComment) {

		parent->type = cybNodeCompositeState;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_cyberiada_xml(xmlNode* root, CyberiadaDocument* doc)
{
	GraphProcessorState gps = gpsInit;
	CyberiadaSM* sm;
	NodeStack* stack = NULL;
	int res;
/*	CyberiadaNode *meta_node, *ext_node;
	CyberiadaEdge *edge, *prev_edge;*/
	
	if ((res = cyberiada_build_graphs(root, doc, &stack, &gps,
									 cyb_processor_state_table,
									 cyb_processor_state_table_size)) != CYBERIADA_NO_ERROR) {
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

/* -----------------------------------------------------------------------------
 * GraphML reader interface
 * ----------------------------------------------------------------------------- */

int cyberiada_read_sm_document(CyberiadaDocument* cyb_doc, const char* filename, CyberiadaXMLFormat format)
{
	int res;
	xmlDoc* doc = NULL;
	xmlNode* root = NULL;
	CyberiadaSM* sm;
	NamesList* nl = NULL;
	
	cyberiada_init_sm_document(cyb_doc);
	
	/* parse the file and get the DOM */
	if ((doc = xmlReadFile(filename, NULL, 0)) == NULL) {
		ERROR("error: could not parse file %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

	cyberiada_init_action_regexps();

	do {
		/* get the root element node */
		root = xmlDocGetRootElement(doc);

		if (strcmp((const char*)root->name, GRAPHML_GRAPHML_ELEMENT) != 0) {
			ERROR("error: could not find GraphML root node %s\n", filename);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		/* check whether the xml is graphml */
		if (cyberiada_check_graphml_ns(root, &format)) {
			ERROR("error: no valid graphml namespace in %s\n", filename);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		cyb_doc->state_machines = cyberiada_create_sm();
		
		/* DEBUG("reading format %d\n", format); */
		if (format == cybxmlYED) {
			res = cyberiada_decode_yed_xml(root, cyb_doc);
		} else if (format == cybxmlCyberiada10) {
			res = cyberiada_decode_cyberiada_xml(root, cyb_doc);
		} else {
			ERROR("error: unsupported GraphML format of file %s\n",
				  filename);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		if (res != CYBERIADA_NO_ERROR) {
			break;
		}

		for (sm = cyb_doc->state_machines; sm; sm = sm->next) {
			if ((res = cyberiada_graphs_reconstruct_nodes(sm->nodes, &nl)) != CYBERIADA_NO_ERROR) {
				ERROR("error: cannot reconstruct graph nodes file %s\n",
					  filename);
				break;
			}
		}
		
		if ((res = cyberiada_graphs_reconstruct_edges(cyb_doc, &nl)) != CYBERIADA_NO_ERROR) {
			ERROR("error: cannot reconstruct graph edges from file %s\n",
				  filename);
			break;
		}
	} while(0);

	cyberiada_free_name_list(&nl);
	cyberiada_free_action_regexps();
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
    return res;
}

/* -----------------------------------------------------------------------------
 * Printing functions 
 * ----------------------------------------------------------------------------- */

static int cyberiada_print_meta(CyberiadaMetainformation* meta)
{
	size_t i;
	char* value;
	printf("Meta information:\n");

	if (meta) {
		for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
			value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
			if (value) {
				printf(" %s: %s\n", cyberiada_metadata[i].title, value);
			}
		}
	
		if (meta->transition_order_flag) {
			printf(" trantision order flag: %d\n", meta->transition_order_flag);
		}
		if (meta->event_propagation_flag) {
			printf(" event propagation flag: %d\n", meta->event_propagation_flag);
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_action(CyberiadaAction* action, int level)
{
	char levelspace[16];
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}
	
	printf("%sActions:\n", levelspace);
	while (action) {
		printf("%s Action (type %d):\n", levelspace, action->type);
		if(action->trigger) {
			printf("%s  Trigger: \"%s\"\n", levelspace, action->trigger);
		}		
		if(action->guard) {
			printf("%s  Guard: \"%s\"\n", levelspace, action->guard);
		}
		if(action->behavior) {
			printf("%s  Behavior: \"%s\"\n", levelspace, action->behavior);
		}
		action = action->next;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_node(CyberiadaNode* node, int level)
{
	CyberiadaNode* cur_node;
	char levelspace[16];
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}

	printf("%sNode {id: %s, title: \"%s\", type: %d",
		   levelspace, node->id, node->title, (int)node->type);
	printf("}\n");
	if (node->color) {
		printf("%sColor: %s\n", levelspace, node->color);
	}
	if (node->type == cybNodeSM ||
		node->type == cybNodeSimpleState ||
		node->type == cybNodeCompositeState ||
		node->type == cybNodeSubmachineState ||
		node->type == cybNodeComment ||
		node->type == cybNodeChoice) {
		
		if (node->type == cybNodeSubmachineState && node->link && node->link->ref) {
			printf("%sLink to SM: %s\n", levelspace, node->link->ref);
		} else if (node->type == cybNodeComment && node->comment_data) {
			printf("%sComment data [markup: %s]:\n", levelspace,
				   node->comment_data->markup ? node->comment_data->markup : "");
			printf("%s%s\n", levelspace, node->comment_data->body);
		}
		if (node->geometry_rect) {
			printf("%sGeometry: (%lf, %lf, %lf, %lf)\n",
			   levelspace,
				   node->geometry_rect->x,
				   node->geometry_rect->y,
				   node->geometry_rect->width,
				   node->geometry_rect->height);
		}
	} else {
		if (node->geometry_point) {
			printf("%sGeometry: (%lf, %lf)\n",
				   levelspace,
				   node->geometry_point->x,
				   node->geometry_point->y);
		}
	}
	
	cyberiada_print_action(node->actions, level + 1);
	
	printf("%sChildren:\n", levelspace);
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, level + 1);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_edge(CyberiadaEdge* edge)
{
	CyberiadaPolyline* polyline;
	printf(" Edge %s [%s %s]->[%s %s] [type %d]\n",
		   edge->id,
		   edge->source_id,
		   edge->source->type == cybNodeInitial ? "INIT" : edge->source->title,
		   edge->target_id,
		   edge->target->type == cybNodeInitial ? "INIT" : edge->target->title,
		   edge->type);
	if (edge->color) {
		printf("  Color: %s\n", edge->color);
	}
	if (edge->comment_subject) {
		printf("  Comment subject [type: %d]\n", edge->comment_subject->type);
		if (edge->comment_subject->fragment) {
			printf("   Fragment: %s\n", edge->comment_subject->fragment);
		}
	}
	printf("  Geometry: ");
	if (edge->geometry_polyline) {
		printf("   Polyline:");
		for (polyline = edge->geometry_polyline; polyline; polyline = polyline->next) {
			printf(" (%lf, %lf)", polyline->point.x, polyline->point.y);
		}
		printf("\n");
	}
	if (edge->geometry_source_point) {
		printf("   Source point: (%lf, %lf)\n",
			   edge->geometry_source_point->x,
			   edge->geometry_source_point->y);
	}
	if (edge->geometry_target_point) {
		printf("   Target point: (%lf, %lf)\n",
			   edge->geometry_target_point->x,
			   edge->geometry_target_point->y);
	}
	if (edge->geometry_label_point) {
		printf("   Label point: (%lf, %lf)\n",
			   edge->geometry_label_point->x,
			   edge->geometry_label_point->y);
	}
	cyberiada_print_action(edge->action, 2);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_sm(CyberiadaSM* sm)
{
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;

	printf("State Machine\n");
	
	printf("Nodes:\n");
	for (cur_node = sm->nodes; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, 0);
	}
	printf("\n");

	printf("Edges:\n");
	for (cur_edge = sm->edges; cur_edge; cur_edge = cur_edge->next) {
		cyberiada_print_edge(cur_edge);
	}
	printf("\n");

	/*if (sm->extensions) {
		printf("Extensions:\n");
		for (cur_ext = sm->extensions; cur_ext; cur_ext = cur_ext->next) {
			cyberiada_print_extension(cur_ext);
		}
		printf("\n");
		}*/
	
    return CYBERIADA_NO_ERROR;
}

int cyberiada_print_sm_document(CyberiadaDocument* doc)
{
	CyberiadaSM* sm;

	printf("\nDocument:\n");
	cyberiada_print_meta(doc->meta_info);

	for (sm = doc->state_machines; sm; sm = sm->next) {
		cyberiada_print_sm(sm);
	}
	
    return CYBERIADA_NO_ERROR;
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

		if (*(action->trigger) || *(action->behavior) || *(action->guard)) { 
			if (*(action->trigger)) {
				if (action->type != cybActionTransition) {
					snprintf(buffer, buffer_len, "%s/", action->trigger);
				} else {
					if (*(action->guard)) {
						snprintf(buffer, buffer_len, "%s [%s]/", action->trigger, action->guard);
					} else {
						snprintf(buffer, buffer_len, "%s/", action->trigger);
					}
				}
				XML_WRITE_TEXT(writer, buffer);
				if (action->next || *(action->behavior)) {
					XML_WRITE_TEXT(writer, "\n");
				}
			} else {
				if (*(action->behavior)) {
					XML_WRITE_TEXT(writer, "/\n");
				} else if (action->next) {
					XML_WRITE_TEXT(writer, "\n");
				}
			}
		
			if (*(action->behavior)) {
				XML_WRITE_TEXT(writer, action->behavior);
				XML_WRITE_TEXT(writer, "\n");
			}
			
			if (action->next) {
				XML_WRITE_TEXT(writer, "\n");
			}
		}

		action = action->next;
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_geometry_rect_cyberiada(xmlTextWriterPtr writer, CyberiadaRect* rect, size_t indent)
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

static int cyberiada_write_geometry_point_cyberiada(xmlTextWriterPtr writer, CyberiadaPoint* point, size_t indent)
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

static int cyberiada_write_node_cyberiada(xmlTextWriterPtr writer, CyberiadaNode* node, size_t indent)
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

static int cyberiada_write_edge_cyberiada(xmlTextWriterPtr writer, CyberiadaEdge* edge, size_t indent)
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
	XML_WRITE_ATTR(writer, GRAPHML_KEY_ATTRIBUTE, GRAPHML_CYB_KEY_NAME);
	XML_WRITE_TEXT(writer, sm->nodes->title);	
	XML_WRITE_CLOSE_E(writer);

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
		return CYBERIADA_NO_ERROR;
	}
	sm_node = doc->state_machines->nodes;
	if (sm_node->type != cybNodeSM||
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
	cyberiada_encode_meta(doc,
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
	cyberiada_update_metainfo_comment(doc);

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

static int cyberiada_write_node_style_yed(xmlTextWriterPtr writer, CyberiadaNodeType type, size_t indent)
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

static int cyberiada_write_node_title_yed(xmlTextWriterPtr writer, const char* title, size_t indent)
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

static int cyberiada_write_node_action_yed(xmlTextWriterPtr writer, CyberiadaAction* action, size_t indent)
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

static int cyberiada_write_edge_action_yed(xmlTextWriterPtr writer, CyberiadaAction* action, size_t indent)
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

static int cyberiada_write_geometry_yed(xmlTextWriterPtr writer, CyberiadaRect* rect, size_t indent)
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

static int cyberiada_write_node_yed(xmlTextWriterPtr writer, CyberiadaNode* node, size_t indent)
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

static int cyberiada_write_edge_yed(xmlTextWriterPtr writer, CyberiadaEdge* edge, size_t indent)
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

int cyberiada_write_sm_document(CyberiadaDocument* doc, const char* filename, CyberiadaXMLFormat format)
{
	xmlTextWriterPtr writer;
	int res;

	if (format != cybxmlCyberiada10 && format != cybxmlYED) {
		ERROR("unsupported SM format for write: %d\n", format);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (!doc) {
		ERROR("empty SM document to write\n");
		return CYBERIADA_BAD_PARAMETER;
	}

	writer = xmlNewTextWriterFilename(filename, 0);
	if (!writer) {
		ERROR("cannot open xml writter for file %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

	res = xmlTextWriterStartDocument(writer, NULL, GRAPHML_XML_ENCODING, NULL);
	if (res < 0) {
		ERROR("error writing xml start document: %d\n", res);
		xmlFreeTextWriter(writer);
		return CYBERIADA_XML_ERROR;
	}

	if (format == cybxmlYED) {
		if (!doc->state_machines || doc->state_machines->next) {
			ERROR("YED format supports only single SM documents\n");
			xmlFreeTextWriter(writer);
			return CYBERIADA_BAD_PARAMETER;
		}
		res = cyberiada_write_sm_document_yed(doc, writer);
	} else if (format == cybxmlCyberiada10) {
		res = cyberiada_write_sm_document_cyberiada(doc, writer);
	}
	if (res != CYBERIADA_NO_ERROR) {
		ERROR("error writing xml %d\n", res);
		xmlFreeTextWriter(writer);
		return CYBERIADA_XML_ERROR;
	}

	res = xmlTextWriterEndDocument(writer);
	if (res < 0) {
		ERROR("error writing xml end document: %d\n", res);
		xmlFreeTextWriter(writer);
		return CYBERIADA_XML_ERROR;
	}
	
	xmlFreeTextWriter(writer);

	return CYBERIADA_NO_ERROR;
}
