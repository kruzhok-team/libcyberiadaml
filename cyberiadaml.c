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
 * ----------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cyberiadaml.h"

#define GRAPHML_NAMESPACE_URI 				"http://graphml.graphdrawing.org/xmlns"
#define GRAPHML_GRAPHML_ELEMENT				"graphml"
#define GRAPHML_BERLOGA_SCHEMENAME_ATTR 	"SchemeName"
#define GRAPHML_GRAPH_ELEMENT				"graph"
#define GRAPHML_NODE_ELEMENT				"node"
#define GRAPHML_EDGE_ELEMENT				"edge"
#define GRAPHML_ID_ATTRIBUTE				"id"
#define GRAPHML_SOURCE_ATTRIBUTE			"source"
#define GRAPHML_TARGET_ATTRIBUTE			"target"
#define GRAPHML_YED_GEOMETRYNODE			"Geometry"
#define GRAPHML_YED_PATHNODE				"Path"
#define GRAPHML_YED_POINTNODE				"Point"
#define GRAPHML_YED_GEOM_X_ATTRIBUTE		"x"
#define GRAPHML_YED_GEOM_Y_ATTRIBUTE		"y"
#define GRAPHML_YED_GEOM_WIDTH_ATTRIBUTE	"width"
#define GRAPHML_YED_GEOM_HEIGHT_ATTRIBUTE	"height"
#define GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE	"sx"
#define GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE	"sy"
#define GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE	"tx"
#define GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE	"ty"
#define GRAPHML_YED_COMMENTNODE				"UMLNoteNode"
#define GRAPHML_YED_GROUPNODE				"GroupNode"
#define GRAPHML_YED_GENERICNODE				"GenericNode"
#define GRAPHML_YED_LABELNODE				"NodeLabel"
#define GRAPHML_YED_NODE_CONFIG_ATTRIBUTE	"configuration"
#define GRAPHML_YED_NODE_CONFIG_START		"com.yworks.bpmn.Event"
#define GRAPHML_YED_NODE_CONFIG_START2		"com.yworks.bpmn.Event.withShadow"
#define GRAPHML_YED_PROPNODE				"Property"
#define GRAPHML_YED_PROP_VALUE_ATTRIBUTE	"value"
#define GRAPHML_YED_PROP_VALUE_START		"EVENT_CHARACTERISTIC_START"
#define GRAPHML_YED_EDGELABEL				"EdgeLabel"

#define CYBERIADA_HOLLOW_NODE				"<node>"

#define CYBERIADA_ENTRY_ACTION				"entry"
#define CYBERIADA_EXIT_ACTION				"exit"

#define MAX_STR_LEN							4096

#ifdef __DEBUG__
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

/* -----------------------------------------------------------------------------
 * Utility functions
 * ----------------------------------------------------------------------------- */

int cyberiada_copy_string(char** target, unsigned int* size, const char* source)
{
	char* target_str;
	unsigned int strsize;
	if (!source)
		return CYBERIADA_BAD_PARAMETER;
	strsize = strlen(source);  
	if (strsize > MAX_STR_LEN - 1) {
		strsize = MAX_STR_LEN - 1;
	}
	target_str = (char*)malloc(strsize + 1);
	strncpy(target_str, source, strsize);
	target_str[strsize] = 0;
	*target = target_str;
	*size = strsize;
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * Graph manipulation functions
 * ----------------------------------------------------------------------------- */

CyberiadaNode* cyberiada_graph_find_node(CyberiadaNode* root, const char* id)
{
	CyberiadaNode* node;
	CyberiadaNode* found;
	if (strcmp(root->id, id) == 0) {
		return root;
	}
	for (node = root->next; node; node = node->next) {
		found = cyberiada_graph_find_node(node, id);
		if (found)
			return found;
	}
	if (root->children) {
		return cyberiada_graph_find_node(root->children, id);
	}
	return NULL;
}

CyberiadaNode* cyberiada_new_node(const char* id)
{
	CyberiadaNode* new_node = (CyberiadaNode*)malloc(sizeof(CyberiadaNode));
	cyberiada_copy_string(&(new_node->id), &(new_node->id_len), id);
	new_node->title = NULL;
	new_node->title_len = 0;
	new_node->type = cybNodeSimple;
	new_node->next = NULL;
	new_node->parent = NULL;
	new_node->children = NULL;
	new_node->action = NULL;
	new_node->action_len = 0;
	new_node->geometry_rect.x = new_node->geometry_rect.y =
		new_node->geometry_rect.width = new_node->geometry_rect.height = 0.0;
	return new_node;
}

CyberiadaEdge* cyberiada_new_edge(const char* id, CyberiadaNode* source, CyberiadaNode* target)
{
	CyberiadaEdge* new_edge = (CyberiadaEdge*)malloc(sizeof(CyberiadaEdge));
	cyberiada_copy_string(&(new_edge->id), &(new_edge->id_len), id);
	new_edge->source = source;
	new_edge->target = target;
	new_edge->action = NULL;
	new_edge->next = NULL;
	new_edge->geometry_source_point.x = new_edge->geometry_source_point.y =
		new_edge->geometry_target_point.x = new_edge->geometry_target_point.y = 0.0;
	new_edge->geometry_polyline = NULL;
	return new_edge;
}

int cyberiada_graph_add_sibling_node(CyberiadaNode* sibling, CyberiadaNode* new_node)
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

int cyberiada_graph_add_edge(CyberiadaSM* sm, const char* id, CyberiadaNode* source, CyberiadaNode* target)
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

CyberiadaEdge* cyberiada_graph_find_last_edge(CyberiadaSM* sm)
{
	CyberiadaEdge* edge;
	if (!sm) {
		return NULL;
	}
	edge = sm->edges;
	while (edge && edge->next) edge = edge->next;	
	return edge;
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library fucntions declarations
 * ----------------------------------------------------------------------------- */

CyberiadaSM* cyberiada_create_sm()
{
	CyberiadaSM* sm = (CyberiadaSM*)malloc(sizeof(CyberiadaSM));
	cyberiada_init_sm(sm);
	return sm;
}

int cyberiada_init_sm(CyberiadaSM* sm)
{
	if (sm) {
		sm->name = NULL;
		sm->name = 0;
		sm->version = NULL;
		sm->version_len = 0;
		sm->nodes = NULL;
		sm->start = NULL;
		sm->edges = NULL;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_node(CyberiadaNode* node)
{
	CyberiadaNode* n;
	if(node != NULL) {
		do {
			n = node;
			node = node->next;
			if(n->id) free(n->id);
			if(n->title) free(n->title);
			if(n->children) {
				cyberiada_destroy_node(n->children);
			}
			if(n->action) free(n->action);
		} while (node);
	}
}

int cyberiada_cleanup_sm(CyberiadaSM* sm)
{
	CyberiadaNode* n;
	CyberiadaEdge *edge, *e;
	CyberiadaPolyline *polyline, *pl;
	
	if (sm == NULL) {
		return CYBERIADA_NO_ERROR;
	}

	if (sm->name) free(sm->name);
	if (sm->version) free(sm->version);
	if (sm->nodes) {
		cyberiada_destroy_node(sm->nodes);
	}
	if (sm->edges) {
		edge = sm->edges;
		do {
			e = edge;
			edge = edge->next;
			if (e->id) free(e->id);
			if (e->action) free(e->action);
			if (e->geometry_polyline) {
				polyline = e->geometry_polyline;
				do {
					pl = polyline;
					polyline = polyline->next;
					free(pl);
				} while (polyline);
			} 
			free(e);
		} while (edge);
	}

	return CYBERIADA_NO_ERROR;
}

int cyberiada_destroy_sm(CyberiadaSM* sm)
{
	int res = cyberiada_cleanup_sm(sm);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}
	free(sm);
	return CYBERIADA_NO_ERROR;
}	

static int cyberiada_get_attr_value(char* buffer, unsigned int buffer_len,
									xmlNode* node, const char* attrname)
{
	xmlAttr* attribute = node->properties;
	while(attribute) {
		if (strcmp(attribute->name, attrname) == 0) {
			xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
			strncpy(buffer, (char*)value, buffer_len);
			xmlFree(value);
			return CYBERIADA_NO_ERROR;
		}
		attribute = attribute->next;
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_get_element_text(char* buffer, unsigned int buffer_len,
									  xmlNode* node)
{
	xmlChar* value = xmlNodeListGetString(node->doc,
										  node->xmlChildrenNode,
										  1);
	strncpy(buffer, (char*)value, buffer_len);
	xmlFree(value);
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML XML processor state machine
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
	gpsEdgePath,
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
	"EdgePath",
	"Invalid"
};

static GraphProcessorState handle_new_graph(xmlNode* xml_node,
											CyberiadaSM* sm,
											CyberiadaNode** current)
{
	CyberiadaNode* node;
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	/* process the top graph element only */
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	node = cyberiada_new_node(CYBERIADA_HOLLOW_NODE);
	DEBUG("found graph %s \n", buffer);
	if (sm->nodes == NULL) {
		sm->nodes = node;
	} else {
		(*current)->children = node;
		node->parent = *current;
	}
	*current = node;
	return gpsGraph;
}

static GraphProcessorState handle_new_node(xmlNode* xml_node,	
										   CyberiadaSM* sm,
										   CyberiadaNode** current)
{
	CyberiadaNode* node;
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	DEBUG("found node %s\n", buffer);
	if (*current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	if(strcmp((*current)->id, CYBERIADA_HOLLOW_NODE) == 0) {
		if (*current == sm->nodes) {
			node = cyberiada_new_node(buffer);
			(*current)->children = node;
			node->parent = *current;
			*current = node;
		} else {
			free((*current)->id);
			cyberiada_copy_string(&((*current)->id), &((*current)->id_len), buffer);
		}
	} else {
		node = cyberiada_new_node(buffer);
		cyberiada_graph_add_sibling_node(*current, node);
		*current = node;
	}
	return gpsNode;
}

static GraphProcessorState handle_group_node(xmlNode* xml_node,
											 CyberiadaSM* sm,
											 CyberiadaNode** current)
{
	(*current)->type = cybNodeComplex;
	return gpsNodeGeometry;
}

static GraphProcessorState handle_comment_node(xmlNode* xml_node,
											   CyberiadaSM* sm,
											   CyberiadaNode** current)
{
	(*current)->type = cybNodeComment;
	cyberiada_copy_string(&((*current)->title), &((*current)->title_len), "COMMENT");
	return gpsNodeGeometry;
}

static GraphProcessorState handle_generic_node(xmlNode* xml_node,
											   CyberiadaSM* sm,
											   CyberiadaNode** current)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_YED_NODE_CONFIG_ATTRIBUTE) == CYBERIADA_NO_ERROR &
		(strcmp(buffer, GRAPHML_YED_NODE_CONFIG_START) == 0 ||
		 strcmp(buffer, GRAPHML_YED_NODE_CONFIG_START2) == 0)) {
		(*current)->type = cybNodeInitial;
		if ((*current)->title != NULL) {
			ERROR("Trying to set start node %s label twice\n", (*current)->id);
			return gpsInvalid;
		}
		cyberiada_copy_string(&((*current)->title), &((*current)->title_len), "");
	} else {
		(*current)->type = cybNodeSimple;
	}
	return gpsNodeGeometry;
}

static int cyberiada_xml_read_coord(xmlNode* xml_node,
									const char* attr_name,
									double* result)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 attr_name) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_BAD_PARAMETER;
	}
	*result = atof(buffer);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_xml_read_rect(xmlNode* xml_node,
								   CyberiadaRect* r)
{
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_X_ATTRIBUTE,
								 &(r->x)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_Y_ATTRIBUTE,
								 &(r->y)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_WIDTH_ATTRIBUTE,
								 &(r->width)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_HEIGHT_ATTRIBUTE,
								 &(r->height)) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_BAD_PARAMETER;
	}
	return CYBERIADA_NO_ERROR;
}

static GraphProcessorState handle_node_geometry(xmlNode* xml_node,
												CyberiadaSM* sm,
												CyberiadaNode** current)
{
	CyberiadaNodeType type = (*current)->type;
	if (cyberiada_xml_read_rect(xml_node,
								&((*current)->geometry_rect)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if (type == cybNodeInitial) {
		(*current)->geometry_rect.x += (*current)->geometry_rect.width / 2.0;
		(*current)->geometry_rect.y += (*current)->geometry_rect.height / 2.0;
		(*current)->geometry_rect.width = 0.0;
		(*current)->geometry_rect.height = 0.0;
		return gpsNodeStart;
	} else if (type == cybNodeComment) {
		return gpsNodeAction;
	} else {
		return gpsNodeTitle;
	}
}

static GraphProcessorState handle_property(xmlNode* xml_node,
										   CyberiadaSM* sm,
										   CyberiadaNode** current)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
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
											CyberiadaSM* sm,
											CyberiadaNode** current)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if ((*current)->title != NULL) {
		ERROR("Trying to set node %s label twice\n", (*current)->id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	DEBUG("Set node %s title %s\n", (*current)->id, buffer);
	cyberiada_copy_string(&((*current)->title), &((*current)->title_len), buffer);
	return gpsNodeAction;
}

static GraphProcessorState handle_node_action(xmlNode* xml_node,
											  CyberiadaSM* sm,
											  CyberiadaNode** current)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if ((*current)->action != NULL) {
		ERROR("Trying to set node %s action twice\n", (*current)->id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	DEBUG("Set node %s action %s\n", (*current)->id, buffer);
	cyberiada_copy_string(&((*current)->action), &((*current)->action_len), buffer);
	return gpsGraph;
}

static GraphProcessorState handle_new_edge(xmlNode* xml_node,
										   CyberiadaSM* sm,
										   CyberiadaNode** node)
{
	CyberiadaNode* source = NULL;
	CyberiadaNode* target = NULL;
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_SOURCE_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	source = cyberiada_graph_find_node(sm->nodes, buffer);
	if (source == NULL) {
		return gpsInvalid;
	}
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_TARGET_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	target = cyberiada_graph_find_node(sm->nodes, buffer);
	if (target == NULL) {
		return gpsInvalid;
	}
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		buffer[0] = 0;
	}
	DEBUG("found edge %s\n", buffer);
	DEBUG("add edge %s %s -> %s\n", buffer, source->id, target->id);
	cyberiada_graph_add_edge(sm, buffer, source, target);
	return gpsEdgePath;
}

static GraphProcessorState handle_edge_geometry(xmlNode* xml_node,
												CyberiadaSM* sm,
												CyberiadaNode** node)
{
	CyberiadaEdge *current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_SOURCE_X_ATTRIBUTE,
								 &(current->geometry_source_point.x)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_SOURCE_Y_ATTRIBUTE,
								 &(current->geometry_source_point.y)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_TARGET_X_ATTRIBUTE,
								 &(current->geometry_target_point.x)) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_TARGET_Y_ATTRIBUTE,
								 &(current->geometry_target_point.y)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	return gpsEdgePath;
}

static GraphProcessorState handle_edge_point(xmlNode* xml_node,
											 CyberiadaSM* sm,
											 CyberiadaNode** node)
{
	CyberiadaEdge *current = cyberiada_graph_find_last_edge(sm);
	double x, y;
	CyberiadaPolyline *pl, *last_pl;
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_X_ATTRIBUTE,
								 &x) != CYBERIADA_NO_ERROR ||
		cyberiada_xml_read_coord(xml_node,
								 GRAPHML_YED_GEOM_Y_ATTRIBUTE,
								 &y) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	pl = (CyberiadaPolyline*)malloc(sizeof(CyberiadaPolyline));
	pl->point.x = x;
	pl->point.y = y;
	pl->next = NULL;
	if (current->geometry_polyline == NULL) {
		current->geometry_polyline = pl;
	} else {
		last_pl = current->geometry_polyline;
		while (last_pl->next) last_pl = last_pl->next;
		last_pl->next = pl;
	}
	return gpsEdgePath;
}

static GraphProcessorState handle_edge_label(xmlNode* xml_node,
											CyberiadaSM* sm,
											CyberiadaNode** node)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	CyberiadaEdge *current;
	current = cyberiada_graph_find_last_edge(sm);
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (current->action != NULL) {
		ERROR("Trying to set edge %s:%s label twice\n",
			  current->source->id, current->target->id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	DEBUG("add edge %s:%s label %s\n",
		  current->source->id, current->target->id, buffer);
	cyberiada_copy_string(&(current->action), &(current->action_len), buffer);
	return gpsGraph;
}

typedef GraphProcessorState (*Handler)(xmlNode* xml_root,
									   CyberiadaSM* sm,
									   CyberiadaNode** current);

typedef struct {
	GraphProcessorState		state;
	const char* 			symbol;
	Handler					handler;
} ProcessorTransition;

static ProcessorTransition processor_state_table[] = {
	{gpsInit, GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsGraph, GRAPHML_NODE_ELEMENT, &handle_new_node},
	{gpsGraph, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
	{gpsGraph, GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsNode, GRAPHML_YED_COMMENTNODE, &handle_comment_node},
	{gpsNode, GRAPHML_YED_GROUPNODE, &handle_group_node},
	{gpsNode, GRAPHML_YED_GENERICNODE, &handle_generic_node},
	{gpsNodeGeometry, GRAPHML_YED_GEOMETRYNODE, &handle_node_geometry},
	{gpsNodeStart, GRAPHML_YED_PROPNODE, &handle_property},
	{gpsNodeStart, GRAPHML_NODE_ELEMENT, &handle_new_node},
	{gpsNodeTitle, GRAPHML_YED_LABELNODE, &handle_node_title},
	{gpsNodeAction, GRAPHML_YED_LABELNODE, &handle_node_action},
	{gpsEdge, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
	{gpsEdgePath, GRAPHML_YED_PATHNODE, &handle_edge_geometry},
	{gpsEdgePath, GRAPHML_YED_POINTNODE, &handle_edge_point},
	{gpsEdgePath, GRAPHML_YED_EDGELABEL, &handle_edge_label},
	{gpsEdgePath, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
};
const unsigned int processor_state_table_size = sizeof(processor_state_table) / sizeof(ProcessorTransition);

static int dispatch_processor(xmlNode* xml_node,
							  CyberiadaSM* sm,
							  CyberiadaNode** current,
							  GraphProcessorState* gps) {
	unsigned int i;
	if (xml_node->type == XML_ELEMENT_NODE) {
		for (i = 0; i < processor_state_table_size; i++) {
			if (processor_state_table[i].state == *gps &&
				strcmp(xml_node->name, processor_state_table[i].symbol) == 0) {
				*gps = (*(processor_state_table[i].handler))(xml_node, sm, current);
				return CYBERIADA_NO_ERROR;
			}
		}
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_build_graph(xmlNode* xml_root,
								 CyberiadaSM* sm,
								 CyberiadaNode** current,
								 GraphProcessorState* gps)
{
	xmlNode *cur_xml_node = NULL;
	for (cur_xml_node = xml_root; cur_xml_node; cur_xml_node = cur_xml_node->next) {
		DEBUG("xml node %s sm root %s current %s gps %s\n",
			  cur_xml_node->name,
			  sm->nodes ? sm->nodes->id : "none",
			  *current ? (*current)->id : "none",
			  debug_state_names[*gps]);
		dispatch_processor(cur_xml_node, sm, current, gps);
		if (*gps == gpsInvalid) {
			return CYBERIADA_FORMAT_ERROR;
		}
		if (cur_xml_node->children) {
			int res = cyberiada_build_graph(cur_xml_node->children, sm, current, gps);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_yed_xml(xmlNode* root, CyberiadaSM* sm)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	GraphProcessorState gps = gpsInit;
	CyberiadaNode* current = NULL;
	int res;
	
	if ((res = cyberiada_build_graph(root, sm, &current, &gps)) != CYBERIADA_NO_ERROR) {
		return res;
	}
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 root,
								 GRAPHML_BERLOGA_SCHEMENAME_ATTR) != CYBERIADA_NO_ERROR) {
		if (sm->nodes && sm->nodes->children) {
			cyberiada_copy_string(&(sm->name), &(sm->name_len), sm->nodes->children->title);
		} else {
			cyberiada_copy_string(&(sm->name), &(sm->name_len), "");
		}
		cyberiada_copy_string(&(sm->version), &(sm->version_len), "YED Ostranna");
	} else {
		cyberiada_copy_string(&(sm->name), &(sm->name_len), buffer);
		cyberiada_copy_string(&(sm->version), &(sm->version_len), "YED Berloga 1.4");
	}	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_cyberiada_xml(xmlNode* root, CyberiadaSM* sm)
{
	return CYBERIADA_FORMAT_ERROR;
}

static int cyberiada_check_graphml_ns(xmlNode* root)
{
	xmlNs* ns;
	if (!root || !(ns = root->nsDef)) {
		ERROR("bad GraphML XML NS: null ns ptr\n");
		return CYBERIADA_XML_ERROR;
	}
	do {
		if (strcmp(ns->href, GRAPHML_NAMESPACE_URI) == 0) {
			return CYBERIADA_NO_ERROR;
		}
		ns = ns->next;
	} while (ns);
	ERROR("no GraphML XML NS href\n");
	return CYBERIADA_XML_ERROR;
}

int cyberiada_read_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format)
{
	int res;
	xmlDoc* doc = NULL;
	xmlNode* root = NULL;
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len;

	cyberiada_init_sm(sm);
	
	/* parse the file and get the DOM */
	if ((doc = xmlReadFile(filename, NULL, 0)) == NULL) {
		ERROR("error: could not parse file %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

    /* get the root element node */
	root = xmlDocGetRootElement(doc);

	if (strcmp(root->name, GRAPHML_GRAPHML_ELEMENT) != 0) {
		ERROR("error: could not find GraphML root node %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

	/* check whether the xml is graphml */
	if (cyberiada_check_graphml_ns(root)) {
		ERROR("error: no graphml namespace in %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

	if (format == cybxmlYED) {
		res = cyberiada_decode_yed_xml(root, sm);
	} else if (format == cybxmlCyberiada) {
		res = cyberiada_decode_cyberiada_xml(root, sm);
	} else {
		ERROR("error: unsupported GraphML format %d of file %s\n",
				format, filename);
		return CYBERIADA_XML_ERROR;
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();

    return res;
}

static int cyberiada_print_node(CyberiadaNode* node, CyberiadaNode* start, int level)
{
	CyberiadaNode* cur_node;
	char levelspace[16];
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}

	printf("%sNode {id: %s, title: \"%s\", type: %d",
		   levelspace, node->id, node->title, (int)node->type);
	if (node == start) {
		printf(", S");
	}
	printf("}\n");
	printf("%sGeometry: (%lf, %lf, %lf, %lf)\n",
		   levelspace,
		   node->geometry_rect.x,
		   node->geometry_rect.y,
		   node->geometry_rect.width,
		   node->geometry_rect.height);
	
	printf("%sActions:\n", levelspace);
	if(node->action) {
		printf("%s\"%s\"\n", levelspace, node->action);
	}

	printf("%sChildren:\n", levelspace);
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, start, level + 1);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_edge(CyberiadaEdge* edge)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	CyberiadaPolyline* polyline;
	int i;
	printf(" Edge %s [%s %s]->[%s %s]\n",
		   edge->id,
		   edge->source->id,
		   edge->source->type == cybNodeInitial ? "INIT" : edge->source->title,
		   edge->target->id,
		   edge->target->title == cybNodeInitial ? "INIT" : edge->target->title);
	if (edge->geometry_polyline == NULL) {
		printf("  Geometry: (%lf, %lf)->(%lf, %lf)\n",
			   edge->geometry_source_point.x,
			   edge->geometry_source_point.y,
			   edge->geometry_target_point.x,
			   edge->geometry_target_point.y);
	} else {
		int i;
		printf("  Geometry: (\n");
		printf("   (%lf, %lf)\n", edge->geometry_source_point.x, edge->geometry_source_point.y);
		for (polyline = edge->geometry_polyline; polyline; polyline = polyline->next) {
			printf("   (%lf, %lf)\n",
				   polyline->point.x,
				   polyline->point.y);
		}
		printf("   (%lf, %lf)\n", edge->geometry_target_point.x, edge->geometry_target_point.y);		
		printf("  )\n");
	}
	if (edge->action) {
		printf("  Action:\n   %s\n", edge->action);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_print_sm(CyberiadaSM* sm)
{
	char buffer[MAX_STR_LEN];
	unsigned int buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;

	printf("\nState Machine {name: %s, version: %s}\n", sm->name, sm->version);

	printf("Nodes:\n");
	for (cur_node = sm->nodes; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, sm->start, 0);
	}
	printf("\n");

	printf("Edges:\n");
	for (cur_edge = sm->edges; cur_edge; cur_edge = cur_edge->next) {
		cyberiada_print_edge(cur_edge);
	}
	printf("\n");
	
    return CYBERIADA_NO_ERROR;
}
