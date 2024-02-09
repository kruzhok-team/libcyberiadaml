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
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cyberiadaml.h"

#define GRAPHML_NAMESPACE_URI 				"http://graphml.graphdrawing.org/xmlns"
#define GRAPHML_NAMESPACE_URI_YED			"http://www.yworks.com/xml/graphml"
#define GRAPHML_GRAPHML_ELEMENT				"graphml"
#define GRAPHML_BERLOGA_SCHEMENAME_ATTR 	"SchemeName"
#define GRAPHML_GRAPH_ELEMENT				"graph"
#define GRAPHML_NODE_ELEMENT				"node"
#define GRAPHML_EDGE_ELEMENT				"edge"
#define GRAPHML_DATA_ELEMENT				"data"
#define GRAPHML_KEY_ELEMENT					"key"
#define GRAPHML_ID_ATTRIBUTE				"id"
#define GRAPHML_KEY_ATTRIBUTE				"key"
#define GRAPHML_FOR_ATTRIBUTE				"for"
#define GRAPHML_ATTR_NAME_ATTRIBUTE			"attr.name"
#define GRAPHML_ATTR_TYPE_ATTRIBUTE			"attr.type"
#define GRAPHML_ALL_ATTRIBUTE_VALUE         "all"
#define GRAPHML_SOURCE_ATTRIBUTE			"source"
#define GRAPHML_TARGET_ATTRIBUTE			"target"
#define GRAPHML_GEOM_X_ATTRIBUTE			"x"
#define GRAPHML_GEOM_Y_ATTRIBUTE			"y"
#define GRAPHML_GEOM_WIDTH_ATTRIBUTE		"width"
#define GRAPHML_GEOM_HEIGHT_ATTRIBUTE		"height"
#define GRAPHML_YED_GEOMETRYNODE			"Geometry"
#define GRAPHML_YED_PATHNODE				"Path"
#define GRAPHML_YED_POINTNODE				"Point"
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
#define GRAPHML_CYB_GRAPH_FORMAT			"gFormat"
#define GRAPHML_CYB_DATA_NAME				"dName"
#define GRAPHML_CYB_DATA_DATA				"dData"
#define GRAPHML_CYB_DATA_INITIAL			"dInitial"
#define GRAPHML_CYB_DATA_GEOMETRY			"dGeometry"
#define GRAPHML_CYB_DATA_COLOR			    "dColor"

#define CYBERIADA_HOLLOW_NODE				"<node>"
#define CYBERIADA_VERSION_CYBERIADAML		"Cyberiada-GraphML"
#define CYBERIADA_VERSION_BERLOGA			"yEd Berloga 1.4"
#define CYBERIADA_VERSION_OSTRANNA			"yEd Ostranna"

#define MAX_STR_LEN							4096

#define PSEUDO_NODE_SIZE					20
#define COMMENT_TITLE                       ""

#ifdef __DEBUG__
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

/* -----------------------------------------------------------------------------
 * Utility functions
 * ----------------------------------------------------------------------------- */

static int cyberiada_copy_string(char** target, size_t* size, const char* source)
{
	char* target_str;
	size_t strsize;
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

static CyberiadaNode* cyberiada_graph_find_node(CyberiadaNode* root, const char* id)
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

static CyberiadaNode* cyberiada_new_node(const char* id)
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
	new_node->geometry_rect = NULL;
	return new_node;
}

static CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target)
{
	CyberiadaEdge* new_edge = (CyberiadaEdge*)malloc(sizeof(CyberiadaEdge));
	new_edge->type = cybEdgeTransition;
	cyberiada_copy_string(&(new_edge->id), &(new_edge->id_len), id);
	cyberiada_copy_string(&(new_edge->source_id), &(new_edge->source_id_len), source);
	cyberiada_copy_string(&(new_edge->target_id), &(new_edge->target_id_len), target);
	new_edge->source = NULL;
	new_edge->target = NULL;
	new_edge->action = NULL;
	new_edge->next = NULL;
	new_edge->geometry_source_point = NULL;
	new_edge->geometry_target_point = NULL;
	new_edge->geometry_polyline = NULL;
	new_edge->geometry_label = NULL;
	new_edge->color = NULL;
	return new_edge;
}

/*static CyberiadaExtension* cyberiada_new_extension(const char* id, const char* title, const char* data)
{
	CyberiadaExtension* new_ext = (CyberiadaExtension*)malloc(sizeof(CyberiadaExtension));
	cyberiada_copy_string(&(new_ext->id), &(new_ext->id_len), id);
	cyberiada_copy_string(&(new_ext->title), &(new_ext->title_len), title);
	cyberiada_copy_string(&(new_ext->data), &(new_ext->data_len), data);
	new_ext->next = NULL;
	return new_ext;
}

static int cyberiada_graph_add_extension(CyberiadaSM* sm, const char* id, const char* title, const char* data)
{
	CyberiadaExtension* last_ext;
	CyberiadaExtension* new_ext;
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_ext = cyberiada_new_extension(id, title, data);
	last_ext = sm->extensions;
	if (last_ext == NULL) {
		sm->extensions = new_ext;
	} else {
		while (last_ext->next) last_ext = last_ext->next;
		last_ext->next = new_ext;
	}
	return CYBERIADA_NO_ERROR;
}*/

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

static int cyberiada_graph_reconstruct_edges(CyberiadaSM* sm)
{
	CyberiadaEdge* edge = sm->edges;
	while (edge) {
		CyberiadaNode* source = cyberiada_graph_find_node(sm->nodes, edge->source_id);
		CyberiadaNode* target = cyberiada_graph_find_node(sm->nodes, edge->target_id);
		if (!source || !target) {
			fprintf(stderr, "cannot find source/target node for edge %s %s\n", edge->source_id, edge->target_id);
			return CYBERIADA_FORMAT_ERROR;
		}
		edge->source = source;
		edge->target = target;
		edge = edge->next;
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library fucntions declarations
 * ----------------------------------------------------------------------------- */

CyberiadaSM* cyberiada_create_sm(void)
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
		sm->info = NULL;
		sm->info_len = 0;
		sm->nodes = NULL;
		sm->edges = NULL;
		/*sm->extensions = NULL;*/
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_destroy_all_nodes(CyberiadaNode* node);

static int cyberiada_destroy_node(CyberiadaNode* node)
{
	if(node != NULL) {
		if(node->id) free(node->id);
		if(node->title) free(node->title);
		if(node->children) {
			cyberiada_destroy_all_nodes(node->children);
		}
		if(node->action) free(node->action);
		if(node->geometry_rect) free(node->geometry_rect);
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

static int cyberiada_destroy_edge(CyberiadaEdge* e)
{
	CyberiadaPolyline *polyline, *pl;
	if (e->id) free(e->id);
	if (e->source_id) free(e->source_id);
	if (e->target_id) free(e->target_id);
	if (e->action) free(e->action);
	if (e->geometry_source_point) free(e->geometry_source_point); 
	if (e->geometry_target_point) free(e->geometry_target_point); 
	if (e->geometry_polyline) {
		polyline = e->geometry_polyline;
		do {
			pl = polyline;
			polyline = polyline->next;
			free(pl);
		} while (polyline);
	}
	if (e->geometry_label) free(e->geometry_label);
	if (e->color) free(e->color);
	free(e);
	return CYBERIADA_NO_ERROR;
}

/*static int cyberiada_find_and_remove_node(CyberiadaNode* current, CyberiadaNode* target)
{
	int res;
	CyberiadaNode* next = NULL;
	CyberiadaNode* prev = NULL;
	if (current != NULL) {
		do {
			next = current->next;
			if (current == target) {
				if (prev) {
					prev->next = next;
					target->next = NULL;
					DEBUG("remove prev = next\n");
					return CYBERIADA_NO_ERROR;
				}
			} else if (current->children == target) {
				current->children = current->children->next;
				target->next = NULL;
				DEBUG("remove current children\n");
				return CYBERIADA_NO_ERROR;
			} else if (current->children == NULL) {
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
}

static int cyberiada_remove_node(CyberiadaSM* sm, CyberiadaNode* node)
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

int cyberiada_cleanup_sm(CyberiadaSM* sm)
{
	CyberiadaEdge *edge, *e;
    /*CyberiadaExtension *extension, *ext;*/
	
	if (sm == NULL) {
		return CYBERIADA_NO_ERROR;
	}

	if (sm->name) free(sm->name);
	if (sm->version) free(sm->version);
	if (sm->info) free(sm->info);
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
/*	if (sm->extensions) {
		extension = sm->extensions;
		do {
			ext = extension;
			extension = extension->next;
			if (ext->id) free(ext->id);
			if (ext->title) free(ext->title);
			if (ext->data) free(ext->data);
			free(ext);
		} while(ext);
		}*/

	cyberiada_init_sm(sm);

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
	gpsFakeNode,
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
	"FakeNode",
	"NodeGeometry",
	"NodeTitle",
	"NodeAction",
	"NodeStart",
	"Edge",
	"EdgePath",
	"Invalid"
};

typedef struct _NodeStack {
	const char*        xml_element;
	CyberiadaNode*     node;
	struct _NodeStack* next;
} NodeStack;

static int node_stack_push(NodeStack** stack)
{
	NodeStack* new_item = (NodeStack*)malloc(sizeof(NodeStack));
	new_item->xml_element = NULL;
	new_item->node = NULL;
	new_item->next = (*stack);
	*stack = new_item;
	return CYBERIADA_NO_ERROR;
}

static int node_stack_set_top_node(NodeStack** stack, CyberiadaNode* node)
{
	NodeStack* to_pop = *stack;
	if (to_pop == NULL) {
		return CYBERIADA_BAD_PARAMETER;
	}
	to_pop->node = node;
	return CYBERIADA_NO_ERROR;
}

static int node_stack_set_top_element(NodeStack** stack, const char* element)
{
	NodeStack* to_pop = *stack;
	if (to_pop == NULL) {
		return CYBERIADA_BAD_PARAMETER;
	}
	to_pop->xml_element = element;
	return CYBERIADA_NO_ERROR;	
}

static CyberiadaNode* node_stack_current_node(NodeStack** stack)
{
	NodeStack* s = *stack;
	while (s) {
		if (s->node)
			return s->node;
		s = s->next;
	}
	return NULL;
}

/* static CyberiadaNode* node_stack_parent_node(NodeStack** stack) */
/* { */
/* 	NodeStack* s = *stack; */
/* 	int first = 1; */
/* 	while (s) { */
/* 		if (s->node) { */
/* 			if (first) { */
/* 				first = 0; */
/* 			} else { */
/* 				return s->node; */
/* 			} */
/* 		} */
/* 		s = s->next; */
/* 	} */
/* 	return NULL; */
/* } */

static int node_stack_pop(NodeStack** stack, CyberiadaNode** node, const char** element)
{
	NodeStack* to_pop = *stack;
	if (to_pop == NULL) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (element)
		*element = to_pop->xml_element;
	if (node)
		*node = to_pop->node;
	*stack = to_pop->next;
	free(to_pop);
	return CYBERIADA_NO_ERROR;
}

/* static int node_stack_empty(NodeStack** stack) */
/* { */
/* 	return *stack == NULL; */
/* } */

static GraphProcessorState handle_new_graph(xmlNode* xml_node,
											CyberiadaSM* sm,
											NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	/* process the top graph element only */
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	DEBUG("found graph %s \n", buffer);
	if (sm->nodes == NULL) {
		sm->nodes = cyberiada_new_node(CYBERIADA_HOLLOW_NODE);
		node_stack_set_top_node(stack, sm->nodes);
	}
	return gpsGraph;
}

static GraphProcessorState handle_new_node(xmlNode* xml_node,	
										   CyberiadaSM* sm,
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
	DEBUG("found node %s\n", buffer);
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
	DEBUG("sm version %s\n", sm->version);

	if (*(node->id) != 0 || sm->version) {
		return gpsNode;
	} else {
		return gpsFakeNode;
	}
}

static GraphProcessorState handle_group_node(xmlNode* xml_node,
											 CyberiadaSM* sm,
											 NodeStack** stack)
{
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	current->type = cybNodeComposite;
	return gpsNodeGeometry;
}

static GraphProcessorState handle_comment_node(xmlNode* xml_node,
											   CyberiadaSM* sm,
											   NodeStack** stack)
{
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	current->type = cybNodeComment;
	cyberiada_copy_string(&(current->title), &(current->title_len), COMMENT_TITLE);
	return gpsNodeGeometry;
}

static GraphProcessorState handle_generic_node(xmlNode* xml_node,
											   CyberiadaSM* sm,
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
		current->type = cybNodeSimple;
	}
	return gpsNodeGeometry;
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
	*result = atof(buffer);
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

static GraphProcessorState handle_node_geometry(xmlNode* xml_node,
												CyberiadaSM* sm,
												NodeStack** stack)
{
	CyberiadaNodeType type;
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}
	type = current->type;
	if (cyberiada_xml_read_rect(xml_node,
								&(current->geometry_rect)) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	if (type == cybNodeInitial) {
		current->geometry_rect->x += current->geometry_rect->width / 2.0;
		current->geometry_rect->y += current->geometry_rect->height / 2.0;
		current->geometry_rect->width = PSEUDO_NODE_SIZE;
		current->geometry_rect->height = PSEUDO_NODE_SIZE;
		return gpsNodeStart;
	} else if (type == cybNodeComment) {
		return gpsNodeAction;
	} else {
		return gpsNodeTitle;
	}
}

static GraphProcessorState handle_property(xmlNode* xml_node,
										   CyberiadaSM* sm,
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
											CyberiadaSM* sm,
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
	DEBUG("Set node %s title %s\n", current->id, buffer);
	cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
	return gpsNodeAction;
}

static GraphProcessorState handle_node_action(xmlNode* xml_node,
											  CyberiadaSM* sm,
											  NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
	if (current == NULL) {
		ERROR("current node invalid\n");
		return gpsInvalid;
	}	
	if (current->action != NULL) {
		ERROR("Trying to set node %s action twice\n", current->id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	DEBUG("Set node %s action %s\n", current->id, buffer);
	cyberiada_copy_string(&(current->action), &(current->action_len), buffer);
	return gpsGraph;
}

static GraphProcessorState handle_new_edge(xmlNode* xml_node,
										   CyberiadaSM* sm,
										   NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	char source_buffer[MAX_STR_LEN];
	size_t source_buffer_len = sizeof(source_buffer) - 1;
	char target_buffer[MAX_STR_LEN];
	size_t target_buffer_len = sizeof(target_buffer) - 1;
	if(cyberiada_get_attr_value(buffer, buffer_len,
								xml_node,
								GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		buffer[0] = 0;
	}
	DEBUG("found edge %s\n", buffer);
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
	DEBUG("add edge %s %s -> %s\n", buffer, source_buffer, target_buffer);
	cyberiada_graph_add_edge(sm, buffer, source_buffer, target_buffer);
	return gpsEdge;
}

static GraphProcessorState handle_edge_geometry(xmlNode* xml_node,
												CyberiadaSM* sm,
												NodeStack** stack)
{
	CyberiadaEdge *current = cyberiada_graph_find_last_edge(sm);
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
		return gpsInvalid;
	}
	return gpsEdgePath;
}

static GraphProcessorState handle_edge_point(xmlNode* xml_node,
											 CyberiadaSM* sm,
											 NodeStack** stack)
{
	CyberiadaEdge *current = cyberiada_graph_find_last_edge(sm);
	CyberiadaPoint* p;
	CyberiadaPolyline *pl, *last_pl;
	if (current == NULL) {
		ERROR("no current edge\n");
		return gpsInvalid;
	}
	if (cyberiada_xml_read_point(xml_node, &p) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
	pl = (CyberiadaPolyline*)malloc(sizeof(CyberiadaPolyline));
	pl->point.x = p->x;
	pl->point.y = p->y;
	free(p);
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
											NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaEdge *current;
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
	DEBUG("add edge %s:%s action %s\n",
		  current->source_id, current->target_id, buffer);
	cyberiada_copy_string(&(current->action), &(current->action_len), buffer);
	return gpsGraph;
}

static GraphProcessorState handle_new_init_data(xmlNode* xml_node,
												CyberiadaSM* sm,
												NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_KEY_ATTRIBUTE) == CYBERIADA_NO_ERROR &&
		strcmp(buffer, GRAPHML_CYB_GRAPH_FORMAT) == 0) {

		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		if (strcmp(buffer, CYBERIADA_VERSION_CYBERIADAML) == 0) {
			cyberiada_copy_string(&(sm->version), &(sm->version_len),
								  CYBERIADA_VERSION_CYBERIADAML);
			return gpsInit;
		} else {
			ERROR("Bad Cyberida-GraphML version: %s\n", buffer);
		}
	} else {
		ERROR("No graph version node\n");
	}
	return gpsInvalid;
}

static GraphProcessorState handle_new_init_key(xmlNode* xml_node,
											   CyberiadaSM* sm,
											   NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	const char* allowed_node_ids[] = { GRAPHML_CYB_DATA_NAME,
									   GRAPHML_CYB_DATA_DATA,
									   GRAPHML_CYB_DATA_INITIAL,
									   GRAPHML_CYB_DATA_GEOMETRY };
	size_t allowed_node_ids_size = sizeof(allowed_node_ids) / sizeof(const char*);
	const char* allowed_edge_ids[] = { GRAPHML_CYB_DATA_DATA,
									   GRAPHML_CYB_DATA_GEOMETRY,
									   GRAPHML_CYB_DATA_COLOR };
	size_t allowed_edge_ids_size = sizeof(allowed_edge_ids) / sizeof(const char*);
	const char** allowed_ids = NULL;
	size_t allowed_ids_size = 0;
	size_t i;
	int found;

	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_FOR_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		ERROR("Cannot find 'for' attribute of the key node\n");
		return gpsInvalid;
	}
	
	if (strcmp(buffer, GRAPHML_ALL_ATTRIBUTE_VALUE) == 0) {
		return gpsInit;
	} else if (strcmp(buffer, GRAPHML_NODE_ELEMENT) == 0) {
		allowed_ids = allowed_node_ids;
		allowed_ids_size = allowed_node_ids_size;
	} else if (strcmp(buffer, GRAPHML_EDGE_ELEMENT) == 0) {
		allowed_ids = allowed_edge_ids;
		allowed_ids_size = allowed_edge_ids_size;
	} else {
		ERROR("Cannot find proper for attribute of the key: %s found\n", buffer);
		return gpsInvalid;
	}
	
	found = 0;

	if (cyberiada_get_attr_value(buffer, buffer_len,
								 xml_node,
								 GRAPHML_ID_ATTRIBUTE) != CYBERIADA_NO_ERROR) {
		return gpsInvalid;
	}
		
	for (i = 0; i < allowed_ids_size; i++) {
		if (strcmp(buffer, allowed_ids[i]) == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		ERROR("Cannot find proper id attribute of the key %s\n", buffer);
		return gpsInvalid;
	}

	return gpsInit;
}

static GraphProcessorState handle_node_data(xmlNode* xml_node,
											CyberiadaSM* sm,
											NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
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
		
	if (strcmp(buffer, GRAPHML_CYB_DATA_NAME) == 0 ||
		strcmp(buffer, GRAPHML_CYB_DATA_DATA) == 0) {
		int title = strcmp(buffer, GRAPHML_CYB_DATA_NAME) == 0;
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		if (title) {
			if (current->title != NULL) {
				ERROR("Trying to set node %s label twice\n", current->id);
				return gpsInvalid;
			}
			DEBUG("Set node %s title %s\n", current->id, buffer);
			cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
		} else {
			if (current->action != NULL) {
				ERROR("Trying to set node %s action twice\n", current->id);
				return gpsInvalid;
			}
			DEBUG("Set node %s action %s\n", current->id, buffer);
			cyberiada_copy_string(&(current->action), &(current->action_len), buffer);
		}
	} else if (strcmp(buffer, GRAPHML_CYB_DATA_INITIAL) == 0) {
		current->type = cybNodeInitial;
	} else if (strcmp(buffer, GRAPHML_CYB_DATA_GEOMETRY) == 0) {
		if (cyberiada_xml_read_rect(xml_node,
									&(current->geometry_rect)) != CYBERIADA_NO_ERROR) {
			return gpsInvalid;
		}
	} else {
		ERROR("bad data key attribute %s\n", buffer);
		return gpsInvalid;
	}
	return gpsNode;
}

static GraphProcessorState handle_fake_node_data(xmlNode* xml_node,
												 CyberiadaSM* sm,
												 NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaNode* current = node_stack_current_node(stack);
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
	if (strcmp(buffer, GRAPHML_CYB_DATA_NAME) == 0 ||
		strcmp(buffer, GRAPHML_CYB_DATA_DATA) == 0) {
		
		int title = strcmp(buffer, GRAPHML_CYB_DATA_NAME) == 0;
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		if (title) {
			DEBUG("Set SM name %s\n", buffer);
			cyberiada_copy_string(&(sm->name), &(sm->name_len), buffer);
		} else {
			DEBUG("Set SM info %s\n", buffer);
			cyberiada_copy_string(&(sm->info), &(sm->info_len), buffer);
		}
	} else {
		ERROR("bad fake node data key attribute %s\n", buffer);
		return gpsInvalid;
	}
	
	return gpsFakeNode;
}

static GraphProcessorState handle_edge_data(xmlNode* xml_node,
											CyberiadaSM* sm,
											NodeStack** stack)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	CyberiadaEdge *current;
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
	if (strcmp(buffer, GRAPHML_CYB_DATA_DATA) == 0) {
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		DEBUG("Set edge %s action %s\n", current->id, buffer);
		cyberiada_copy_string(&(current->action), &(current->action_len), buffer);
	} else if (strcmp(buffer, GRAPHML_CYB_DATA_GEOMETRY) == 0) {
		if (cyberiada_xml_read_point(xml_node,
									 &(current->geometry_label)) != CYBERIADA_NO_ERROR) {
			return gpsInvalid;
		}
	} else if (strcmp(buffer, GRAPHML_CYB_DATA_COLOR) == 0) {
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		cyberiada_copy_string(&(current->color), &(current->color_len), buffer);
	} else {
		ERROR("bad data key attribute %s\n", buffer);
		return gpsInvalid;
	}
	return gpsEdge;
}

typedef GraphProcessorState (*Handler)(xmlNode* xml_root,
									   CyberiadaSM* sm,
									   NodeStack** stack);

typedef struct {
	GraphProcessorState		state;
	const char* 			symbol;
	Handler					handler;
} ProcessorTransition;

static ProcessorTransition yed_processor_state_table[] = {
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
	{gpsEdge, GRAPHML_YED_PATHNODE, &handle_edge_geometry},
	{gpsEdgePath, GRAPHML_YED_POINTNODE, &handle_edge_point},
	{gpsEdgePath, GRAPHML_YED_EDGELABEL, &handle_edge_label},
	{gpsEdgePath, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
};
const size_t yed_processor_state_table_size = sizeof(yed_processor_state_table) / sizeof(ProcessorTransition);

static ProcessorTransition cyb_processor_state_table[] = {
	{gpsInit, GRAPHML_DATA_ELEMENT, &handle_new_init_data},
	{gpsInit, GRAPHML_KEY_ELEMENT, &handle_new_init_key},
	{gpsInit, GRAPHML_GRAPH_ELEMENT, &handle_new_graph},
	{gpsGraph, GRAPHML_NODE_ELEMENT, &handle_new_node},
	{gpsNode, GRAPHML_DATA_ELEMENT, &handle_node_data},
	{gpsNode, GRAPHML_NODE_ELEMENT, &handle_new_node},
	{gpsFakeNode, GRAPHML_DATA_ELEMENT, &handle_fake_node_data},
	{gpsFakeNode, GRAPHML_NODE_ELEMENT, &handle_new_node},
	{gpsGraph, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
	{gpsNode, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
	{gpsEdge, GRAPHML_DATA_ELEMENT, &handle_edge_data},
	{gpsEdge, GRAPHML_EDGE_ELEMENT, &handle_new_edge},
};
const size_t cyb_processor_state_table_size = sizeof(cyb_processor_state_table) / sizeof(ProcessorTransition);

static int dispatch_processor(xmlNode* xml_node,
							  CyberiadaSM* sm,
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
				*gps = (*(processor_state_table[i].handler))(xml_node, sm, stack);
				return CYBERIADA_NO_ERROR;
			}
		}
	}
	return CYBERIADA_NOT_FOUND;
}

static int cyberiada_build_graph(xmlNode* xml_root,
								 CyberiadaSM* sm,
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
		DEBUG("xml node %s sm root %s gps %s\n",
			  cur_xml_node->name,
			  sm->nodes ? sm->nodes->id : "none",
			  debug_state_names[*gps]);
		node_stack_push(stack);
		dispatch_processor(cur_xml_node, sm, stack, gps,
						   processor_state_table, processor_state_table_size);
		if (*gps == gpsInvalid) {
			return CYBERIADA_FORMAT_ERROR;
		}
		if (cur_xml_node->children) {
			int res = cyberiada_build_graph(cur_xml_node->children, sm, stack, gps,
											processor_state_table, processor_state_table_size);
			if (res != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
		node_stack_pop(stack, NULL, NULL);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_yed_xml(xmlNode* root, CyberiadaSM* sm)
{
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	GraphProcessorState gps = gpsInit;
	NodeStack* stack = NULL;
	int res;

	if (cyberiada_get_attr_value(buffer, buffer_len,
								 root,
								 GRAPHML_BERLOGA_SCHEMENAME_ATTR) == CYBERIADA_NO_ERROR) {
		cyberiada_copy_string(&(sm->name), &(sm->name_len), buffer);
	}
	cyberiada_copy_string(&(sm->version), &(sm->version_len), CYBERIADA_VERSION_BERLOGA);
	
	if ((res = cyberiada_build_graph(root, sm, &stack, &gps,
									 yed_processor_state_table,
									 yed_processor_state_table_size)) != CYBERIADA_NO_ERROR) {
		return res;
	}
	
	if (stack != NULL) {
		fprintf(stderr, "error with node stack\n");
		return CYBERIADA_FORMAT_ERROR;
	}
	
	if (sm->name == NULL) {
		if (sm->nodes && sm->nodes->children) {
			cyberiada_copy_string(&(sm->name), &(sm->name_len), sm->nodes->children->title);
		} else {
			cyberiada_copy_string(&(sm->name), &(sm->name_len), "");
		}
		free(sm->version);
		cyberiada_copy_string(&(sm->version), &(sm->version_len), CYBERIADA_VERSION_OSTRANNA);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_cyberiada_xml(xmlNode* root, CyberiadaSM* sm)
{
	GraphProcessorState gps = gpsInit;
	NodeStack* stack = NULL;
	int res;
/*	CyberiadaNode *meta_node, *ext_node;
	CyberiadaEdge *edge, *prev_edge;*/

	if ((res = cyberiada_build_graph(root, sm, &stack, &gps,
									 cyb_processor_state_table,
									 cyb_processor_state_table_size)) != CYBERIADA_NO_ERROR) {
		return res;
	}

	if (stack != NULL) {
		ERROR("error with node stack\n");
		return CYBERIADA_FORMAT_ERROR;
	}

	/*meta_node = cyberiada_graph_find_node(sm->nodes, "");

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
				ext_node = cyberiada_graph_find_node(sm->nodes, edge->target_id);
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
			*format = cybxmlCyberiada;
		}
	} else if (*format == cybxmlYED && !yed_found) {
		ERROR("no GraphML YED NS href\n");
		return CYBERIADA_XML_ERROR;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_read_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format)
{
	int res;
	xmlDoc* doc = NULL;
	xmlNode* root = NULL;

	cyberiada_init_sm(sm);
	
	/* parse the file and get the DOM */
	if ((doc = xmlReadFile(filename, NULL, 0)) == NULL) {
		ERROR("error: could not parse file %s\n", filename);
		return CYBERIADA_XML_ERROR;
	}

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

		DEBUG("reading format %d\n", format);
		if (format == cybxmlYED) {
			res = cyberiada_decode_yed_xml(root, sm);
		} else if (format == cybxmlCyberiada) {
			res = cyberiada_decode_cyberiada_xml(root, sm);
		} else {
			ERROR("error: unsupported GraphML format of file %s\n",
				  filename);
			res = CYBERIADA_XML_ERROR;
			break;
		}

		if (res != CYBERIADA_NO_ERROR) {
			break;
		}
		
		if ((res = cyberiada_graph_reconstruct_edges(sm)) != CYBERIADA_NO_ERROR) {
			ERROR("error: cannot reconstruct graph edges from file %s\n",
				  filename);
			break;
		}
	} while(0);
		
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
    return res;
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
	if (node->geometry_rect) {
		printf("%sGeometry: (%lf, %lf, %lf, %lf)\n",
			   levelspace,
			   node->geometry_rect->x,
			   node->geometry_rect->y,
			   node->geometry_rect->width,
			   node->geometry_rect->height);
	}
	
	printf("%sActions:\n", levelspace);
	if(node->action) {
		printf("%s\"%s\"\n", levelspace, node->action);
	}

	printf("%sChildren:\n", levelspace);
	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		cyberiada_print_node(cur_node, level + 1);
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_edge(CyberiadaEdge* edge)
{
	CyberiadaPolyline* polyline;
	printf(" Edge %s [%s %s]->[%s %s] type %d\n",
		   edge->id,
		   edge->source_id,
		   edge->source->type == cybNodeInitial ? "INIT" : edge->source->title,
		   edge->target_id,
		   edge->target->type == cybNodeInitial ? "INIT" : edge->target->title,
		   edge->type);
	if (edge->color) {
		printf("  Color: %s\n", edge->color);
	}
	printf("  Geometry: ");
	if (edge->geometry_source_point && edge->geometry_target_point) {
		if (edge->geometry_polyline == NULL) {
			printf("  (%lf, %lf)->(%lf, %lf)\n",
				   edge->geometry_source_point->x,
				   edge->geometry_source_point->y,
				   edge->geometry_target_point->x,
				   edge->geometry_target_point->y);
		} else {
			printf("  (\n");
			printf("   (%lf, %lf)\n",
				   edge->geometry_source_point->x,
				   edge->geometry_source_point->y);
			for (polyline = edge->geometry_polyline; polyline; polyline = polyline->next) {
				printf("   (%lf, %lf)\n",
					   polyline->point.x,
					   polyline->point.y);
			}
			printf("   (%lf, %lf)\n",
				   edge->geometry_target_point->x,
				   edge->geometry_target_point->y);		
			printf("  )\n");
		}
	}
	if (edge->geometry_label) {
		printf("  label: (%lf, %lf)\n",
			   edge->geometry_label->x,
			   edge->geometry_label->y);
	}
	if (edge->action) {
		printf("  Action:\n   %s\n", edge->action);
	}
	return CYBERIADA_NO_ERROR;
}

/*static int cyberiada_print_extension(CyberiadaExtension* ext)
{
	printf(" Extension %s %s:\n %s\n", ext->id, ext->title, ext->data);
	return CYBERIADA_NO_ERROR;
	}*/

int cyberiada_print_sm(CyberiadaSM* sm)
{
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;
	/*CyberiadaExtension* cur_ext;*/

	printf("\nState Machine {name: %s, version: %s}\n", sm->name, sm->version);
	printf("  Info: %s\n", sm->info);

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
