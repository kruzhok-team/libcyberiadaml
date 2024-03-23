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
#define GRAPHML_ID_ATTRIBUTE				     "id"
#define GRAPHML_KEY_ATTRIBUTE				     "key"
#define GRAPHML_FOR_ATTRIBUTE				     "for"
#define GRAPHML_PORT_ATTRIBUTE                   "port"
#define GRAPHML_EDGEDEFAULT_ATTRIBUTE            "edgedefault"
#define GRAPHML_ATTR_NAME_ATTRIBUTE			     "attr.name"
#define GRAPHML_ATTR_TYPE_ATTRIBUTE			     "attr.type"
#define GRAPHML_ALL_ATTRIBUTE_VALUE              "all"
#define GRAPHML_EDGEDEFAULT_ATTRIBUTE_VALUE      "directed"
#define GRAPHML_SOURCE_ATTRIBUTE			     "source"
#define GRAPHML_TARGET_ATTRIBUTE			     "target"

/* Common GraphML constants */

#define GRAPHML_GEOM_X_ATTRIBUTE			     "x"
#define GRAPHML_GEOM_Y_ATTRIBUTE			     "y"
#define GRAPHML_GEOM_WIDTH_ATTRIBUTE		     "width"
#define GRAPHML_GEOM_HEIGHT_ATTRIBUTE		     "height"

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

/* Cyberiada-GraphML format constants */

#define GRAPHML_CYB_GRAPH_FORMAT			     "gFormat"
#define GRAPHML_CYB_DATA_NAME				     "dName"
#define GRAPHML_CYB_DATA_DATA				     "dData"
#define GRAPHML_CYB_DATA_COMMENT                 "dNote"
#define GRAPHML_CYB_DATA_COMMENT_SUBJECT         "dPivot"
#define GRAPHML_CYB_DATA_INITIAL			     "dInitial"
#define GRAPHML_CYB_DATA_GEOMETRY			     "dGeometry"
#define GRAPHML_CYB_DATA_COLOR			         "dColor"
#define GRAPHML_CYB_COMMENT_FORMAL               "formal"
#define GRAPHML_CYB_COMMENT_INFORMAL             "informal"

/* HSM format / standard constants */

#define CYBERIADA_FORMAT_CYBERIADAML             "Cyberiada-GraphML-1.0"
#define CYBERIADA_FORMAT_BERLOGA                 "yEd Berloga"
#define CYBERIADA_FORMAT_OSTRANNA                "yEd Ostranna"
#define CYBERIADA_STANDARD_VERSION_CYBERIADAML   "1.0"
#define CYBERIADA_STANDARD_VERSION_YED           ""

/* HSM behavior constants */

#define CYBERIADA_BEHAVIOR_NEWLINE               "\n\n"
#define CYBERIADA_BEHAVIOR_NEWLINE_RN            "\r\n\r\n"
#define CYBERIADA_BEHAVIOR_TRIGGER_ENTRY         "entry"
#define CYBERIADA_BEHAVIOR_TRIGGER_EXIT          "exit"
#define CYBERIADA_BEHAVIOR_TRIGGER_DO            "do"
#define CYBERIADA_BEHAVIOR_EDGE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)?\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*))?\\s*$"
#define CYBERIADA_BEHAVIOR_NODE_REGEXP           "^\\s*(\\w((\\w| |\\.)*\\w)?(\\(\\w+\\))?)\\s*(\\[([^]]+)\\])?\\s*(propagate|block)?\\s*(/\\s*(.*)?)\\s*$"
#define CYBERIADA_BEHAVIOR_REGEXP_MATCHES        10
#define CYBERIADA_BEHAVIOR_REGEXP_MATCH_TRIGGER  1
#define CYBERIADA_BEHAVIOR_REGEXP_MATCH_GUARD    6
#define CYBERIADA_BEHAVIOR_REGEXP_MATCH_PROP     7
#define CYBERIADA_BEHAVIOR_REGEXP_MATCH_ACTION   9
#define CYBERIADA_BEHAVIOR_SPACES_REGEXP         "^\\s*$"
/*#define CYBERIADA_BEHAVIOR_NEWLINE_REGEXP        "^([^\n]*(\n[ \t\r]*[^\\s])?)*\n\\s*\n(.*)?$"
  #define CYBERIADA_BEHAVIOR_NL_REGEXP_MATCHES     4*/

/* HSM metadata constants */

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
#define CYBERIADA_META_ACTIONS_ORDER             "actionsOrder"
#define CYBERIADA_META_AO_TRANSITION             "transitionFirst"
#define CYBERIADA_META_AO_EXIT                   "exitFirst"
#define CYBERIADA_META_EVENT_PROPAGATION         "eventPropagation"
#define CYBERIADA_META_EP_PROPAGATE              "propagate"
#define CYBERIADA_META_EP_BLOCK                  "block"

/* Misc. constants */

#define CYBERIADA_ROOT_NODE                      "__ROOT__"
#define MAX_STR_LEN							     4096
#define PSEUDO_NODE_SIZE					     20
#define EMPTY_TITLE                              ""

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
	*size = strsize;
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * Graph manipulation functions
 * ----------------------------------------------------------------------------- */

static CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id)
{
	CyberiadaNode* node;
	CyberiadaNode* found;
	if (strcmp(root->id, id) == 0) {
		return root;
	}
	for (node = root->next; node; node = node->next) {
		found = cyberiada_graph_find_node_by_id(node, id);
		if (found)
			return found;
	}
	if (root->children) {
		return cyberiada_graph_find_node_by_id(root->children, id);
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

static CyberiadaNode* cyberiada_new_node(const char* id)
{
	CyberiadaNode* new_node = (CyberiadaNode*)malloc(sizeof(CyberiadaNode));
	cyberiada_copy_string(&(new_node->id), &(new_node->id_len), id);
	new_node->type = cybNodeSimpleState;
	new_node->title = NULL;
	new_node->title_len = 0;
	new_node->behavior = NULL;
	new_node->next = NULL;
	new_node->parent = NULL;
	new_node->children = NULL;
	new_node->geometry_rect = NULL;
	return new_node;
}

static int cyberiada_destroy_behavior(CyberiadaBehavior* behavior)
{
	CyberiadaBehavior* b;
	if (behavior != NULL) {
		do {
			b = behavior;
			if (b->trigger) free(b->trigger);
			if (b->guard) free(b->guard);
			if (b->action) free(b->action);
			behavior = b->next;
			free(b);
		} while (behavior);
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
		if(node->behavior) cyberiada_destroy_behavior(node->behavior);
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

static CyberiadaEdge* cyberiada_new_edge(const char* id, const char* source, const char* target)
{
	CyberiadaEdge* new_edge = (CyberiadaEdge*)malloc(sizeof(CyberiadaEdge));
	new_edge->type = cybEdgeTransition;
	cyberiada_copy_string(&(new_edge->id), &(new_edge->id_len), id);
	cyberiada_copy_string(&(new_edge->source_id), &(new_edge->source_id_len), source);
	cyberiada_copy_string(&(new_edge->target_id), &(new_edge->target_id_len), target);
	new_edge->source = NULL;
	new_edge->target = NULL;
	new_edge->behavior = NULL;
	new_edge->next = NULL;
	new_edge->geometry_source_point = NULL;
	new_edge->geometry_target_point = NULL;
	new_edge->geometry_polyline = NULL;
	new_edge->geometry_label = NULL;
	new_edge->color = NULL;
	return new_edge;
}

static CyberiadaBehavior* cyberiada_new_behavior(CyberiadaBehaviorType type,
												 const char* trigger,
												 const char* guard,
												 const char* action)
{
	CyberiadaBehavior* behavior = (CyberiadaBehavior*)malloc(sizeof(CyberiadaBehavior));
	behavior->type = type;
	cyberiada_copy_string(&(behavior->trigger), &(behavior->trigger_len), trigger);
	cyberiada_copy_string(&(behavior->guard), &(behavior->guard_len), guard);
	cyberiada_copy_string(&(behavior->action), &(behavior->action_len), action);
	behavior->next = NULL;
	return behavior;
}

static regex_t cyberiada_edge_behavior_regexp;
static regex_t cyberiada_node_behavior_regexp;
/*static regex_t cyberiada_newline_regexp;*/
static regex_t cyberiada_spaces_regexp;

static int cyberiada_init_behavior_regexps(void)
{
	if (regcomp(&cyberiada_edge_behavior_regexp, CYBERIADA_BEHAVIOR_EDGE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile edge behavior regexp\n");
		return CYBERIADA_ASSERT;
	}
	if (regcomp(&cyberiada_node_behavior_regexp, CYBERIADA_BEHAVIOR_NODE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile edge behavior regexp\n");
		return CYBERIADA_ASSERT;
	}
/*	if (regcomp(&cyberiada_newline_regexp, CYBERIADA_BEHAVIOR_NEWLINE_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile new line regexp\n");
		return CYBERIADA_ASSERT;
		}*/
	if (regcomp(&cyberiada_spaces_regexp, CYBERIADA_BEHAVIOR_SPACES_REGEXP, REG_EXTENDED)) {
		ERROR("cannot compile new line regexp\n");
		return CYBERIADA_ASSERT;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_free_behavior_regexps(void)
{
	regfree(&cyberiada_edge_behavior_regexp);
	regfree(&cyberiada_node_behavior_regexp);
/*	regfree(&cyberiada_newline_regexp);*/
	regfree(&cyberiada_spaces_regexp);
	return CYBERIADA_NO_ERROR;
}


static int cyberiaga_matchres_behavior_regexps(const char* text,
											   const regmatch_t* pmatch, size_t pmatch_size,
											   char** trigger, char** guard, char** action)
{
	size_t i;
	char* part;
	int start, end;
		
	if (pmatch_size != CYBERIADA_BEHAVIOR_REGEXP_MATCHES) {
		ERROR("bad behavior regexp match array size\n");
		return CYBERIADA_ASSERT;
	}

	for (i = 0; i < pmatch_size; i++) {
		if (i != CYBERIADA_BEHAVIOR_REGEXP_MATCH_TRIGGER &&
			i != CYBERIADA_BEHAVIOR_REGEXP_MATCH_GUARD &&
			i != CYBERIADA_BEHAVIOR_REGEXP_MATCH_ACTION) {
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
		if (i == CYBERIADA_BEHAVIOR_REGEXP_MATCH_TRIGGER) {
			*trigger = part;
		} else if (i == CYBERIADA_BEHAVIOR_REGEXP_MATCH_GUARD) {
			*guard = part;
		} else {
			/* i == BEHAVIOR_REGEXP_MATCH_ACTION */
			*action = part;
		}
	}

	return CYBERIADA_NO_ERROR;
}

/*static int cyberiaga_matchres_newline(const regmatch_t* pmatch, size_t pmatch_size,
									  size_t* next_block)
{
	if (pmatch_size != BEHAVIOR_NL_REGEXP_MATCHES) {
		ERROR("bad new line regexp match array size\n");
		return CYBERIADA_ASSERT;
	}
	if (next_block) {
		*next_block = (size_t)pmatch[pmatch_size - 1].rm_so;
	}

	return CYBERIADA_NO_ERROR;
	}*/

static int decode_utf8_strings(char** trigger, char** guard, char** action)
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
	if (*action && **action) {
		oldptr = *action;
		*action = utf8_decode(*action, strlen(*action), &len);
		free(oldptr);
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_edge_behavior(const char* text, CyberiadaBehavior** behavior)
{
	int res;
	size_t buffer_len;
	char *trigger = "", *guard = "", *action = "";
	char *buffer;
	regmatch_t pmatch[CYBERIADA_BEHAVIOR_REGEXP_MATCHES];

	buffer = utf8_encode(text, strlen(text), &buffer_len);
	
	if ((res = regexec(&cyberiada_edge_behavior_regexp, buffer,
					   CYBERIADA_BEHAVIOR_REGEXP_MATCHES, pmatch, 0)) != 0) {
		if (res == REG_NOMATCH) {
			ERROR("edge behavior text didn't match the regexp\n");
			return CYBERIADA_BEHAVIOR_FORMAT_ERROR;
		} else {
			ERROR("edge behavior regexp error %d\n", res);
			return CYBERIADA_ASSERT;
		}
	}
	if (cyberiaga_matchres_behavior_regexps(buffer,
											pmatch, CYBERIADA_BEHAVIOR_REGEXP_MATCHES,
											&trigger, &guard, &action) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_ASSERT;
	}

	decode_utf8_strings(&trigger, &guard, &action);
	
/*	DEBUG("\n");
	DEBUG("edge behavior:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("action: %s\n", action);*/
	
	*behavior = cyberiada_new_behavior(cybBehaviorTransition, trigger, guard, action);
	
	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*action) free(action);

	free(buffer);
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_add_behavior(const char* trigger, const char* guard, const char* action,
								  CyberiadaBehavior** behavior)
{
	CyberiadaBehavior* new_behavior;
	CyberiadaBehaviorType type;

	if (trigger) {
		if (strcmp(trigger, CYBERIADA_BEHAVIOR_TRIGGER_ENTRY) == 0) {
			type = cybBehaviorEntry;
		} else if (strcmp(trigger, CYBERIADA_BEHAVIOR_TRIGGER_EXIT) == 0) {
			type = cybBehaviorExit;
		} else if (strcmp(trigger, CYBERIADA_BEHAVIOR_TRIGGER_DO) == 0) {
			type = cybBehaviorDo;
		} else {
			type = cybBehaviorTransition;
		}
	}
	
	/*DEBUG("\n");
	DEBUG("node behavior:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("action: %s\n", action);
	DEBUG("type: %d\n", type);*/

	new_behavior = cyberiada_new_behavior(type, trigger, guard, action);
	if (*behavior) {
		CyberiadaBehavior* b = *behavior;
		while (b->next) b = b->next;
		b->next = new_behavior;
	} else {
		*behavior = new_behavior;
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_state_block_behavior(const char* text, CyberiadaBehavior** behavior)
{
	int res;
	char *trigger = "", *guard = "", *action = "";
	regmatch_t pmatch[CYBERIADA_BEHAVIOR_REGEXP_MATCHES];
	if ((res = regexec(&cyberiada_node_behavior_regexp, text,
					   CYBERIADA_BEHAVIOR_REGEXP_MATCHES, pmatch, 0)) != 0) {
		if (res == REG_NOMATCH) {
			ERROR("node block behavior text didn't match the regexp\n");
			return CYBERIADA_BEHAVIOR_FORMAT_ERROR;
		} else {
			ERROR("node block behavior regexp error %d\n", res);
			return CYBERIADA_ASSERT;
		}
	}
	if (cyberiaga_matchres_behavior_regexps(text,
											pmatch, CYBERIADA_BEHAVIOR_REGEXP_MATCHES,
											&trigger, &guard, &action) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_ASSERT;
	}

	decode_utf8_strings(&trigger, &guard, &action);
	cyberiada_add_behavior(trigger, guard, action, behavior);

	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*action) free(action);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_decode_state_behavior(const char* text, CyberiadaBehavior** behavior)
{
	int res;
	size_t buffer_len;
	char *buffer, *start, *block, *block2, *next;
/*	regmatch_t pmatch[BEHAVIOR_NL_REGEXP_MATCHES];*/
	
	buffer = utf8_encode(text, strlen(text), &buffer_len);
	next = buffer;

	*behavior = NULL;
	
	while (*next) {
		start = next;
		block = strstr(start, CYBERIADA_BEHAVIOR_NEWLINE);
		block2 = strstr(start, CYBERIADA_BEHAVIOR_NEWLINE_RN);
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
						   BEHAVIOR_NL_REGEXP_MATCHES, pmatch, 0)) != 0) {
			if (res != REG_NOMATCH) {
				ERROR("newline regexp error %d\n", res);
				return CYBERIADA_ASSERT;
			}
		}
		if (res == REG_NOMATCH) {
			block = start;
			start = start + strlen(block);
		} else {
			if (cyberiaga_matchres_newline(pmatch, BEHAVIOR_NL_REGEXP_MATCHES,
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

		if ((res = cyberiada_decode_state_block_behavior(start, behavior)) != CYBERIADA_NO_ERROR) {
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

/*static int cyberiada_graph_add_node_behavior(CyberiadaNode* node,
											 CyberiadaBehaviorType type,
											 const char* trigger,
											 const char* guard,
											 const char* action)
{
	CyberiadaBehavior *behavior, *new_behavior;
	if (!node) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_behavior = cyberiada_new_behavior(type, trigger, guard, action);
	if (node->behavior) {
		node->behavior = new_behavior;
	} else {
		behavior = node->behavior;
		while (behavior->next) behavior = behavior->next;
		behavior->next = new_behavior;
	}
	return CYBERIADA_NO_ERROR;
	}*/

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

/*static int cyberiada_graph_add_edge_behavior(CyberiadaEdge* edge,
															CyberiadaBehaviorType type,
															const char* trigger,
															const char* guard,
															const char* action)
{
	CyberiadaBehavior *behavior, *new_behavior;
	if (!edge) {
		return CYBERIADA_BAD_PARAMETER;
	}
	new_behavior = cyberiada_new_behavior(type, trigger, guard, action);
	if (edge->behavior) {
		edge->behavior = new_behavior;
	} else {
		behavior = edge->behavior;
		while (behavior->next) behavior = behavior->next;
		behavior->next = new_behavior;
	}
	return CYBERIADA_NO_ERROR;
	}*/

static int cyberiada_destroy_edge(CyberiadaEdge* e)
{
	CyberiadaPolyline *polyline, *pl;
	if (e->id) free(e->id);
	if (e->source_id) free(e->source_id);
	if (e->target_id) free(e->target_id);
	if (e->behavior) cyberiada_destroy_behavior(e->behavior);
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

static int cyberiada_graph_reconstruct_edges(CyberiadaSM* sm)
{
	CyberiadaEdge* edge = sm->edges;
	while (edge) {
		CyberiadaNode* source = cyberiada_graph_find_node_by_id(sm->nodes, edge->source_id);
		CyberiadaNode* target = cyberiada_graph_find_node_by_id(sm->nodes, edge->target_id);
		if (!source || !target) {
			ERROR("cannot find source/target node for edge %s %s\n", edge->source_id, edge->target_id);
			return CYBERIADA_FORMAT_ERROR;
		}
		edge->source = source;
		edge->target = target;
		edge = edge->next;
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
		free(meta);
	}
	return CYBERIADA_NO_ERROR;
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
		CYBERIADA_META_DATE,
		offsetof(CyberiadaMetainformation, date),
		offsetof(CyberiadaMetainformation, date_len),
		"actions order"
	}
};

static int cyberiada_add_default_meta(CyberiadaSM* sm, const char* sm_name)
{
	CyberiadaMetainformation* meta;
	
	if (sm->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	cyberiada_copy_string(&(meta->standard_version),
						  &(meta->standard_version_len),
						  CYBERIADA_STANDARD_VERSION_YED);
	if (*sm_name) {
		cyberiada_copy_string(&(meta->name), &(meta->name_len), sm_name);
	}
	
	meta->actions_order_flag = 1;
	meta->event_propagation_flag = 1;
	
	sm->meta_info = meta;
	return CYBERIADA_NO_ERROR;	
}
	
/*static int cyberiada_add_meta(CyberiadaSM* sm, char* metadata)
{
	CyberiadaMetainformation* meta;
	char *line, *parts;
	size_t i;
	char found;
	
	if (sm->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	if (strchr(metadata, CYBERIADA_META_NEW_LINE_CHR)) {
		line = strtok(metadata, CYBERIADA_META_NEW_LINE_STR);
	} else {
		line = metadata;
	}
	do {
		parts = strchr(metadata, CYBERIADA_META_SEPARATOR_CHR);
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
			if (strcmp(line, cyberiada_metadata[i].name) == 0) {
				if ((void*)((&meta) + cyberiada_metadata[i].value_offset) != NULL) {
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
			if (strcmp(line, CYBERIADA_META_ACTIONS_ORDER) == 0) {
				if (strcmp(parts, CYBERIADA_META_AO_TRANSITION) == 0) {
					meta->actions_order_flag = 1;
				} else if (strcmp(parts, CYBERIADA_META_AO_EXIT) == 0) {
					meta->actions_order_flag = 2;
				} else {
					ERROR("Error decoding SM metainformation: bad value of actions order flag parameter\n");
					cyberiada_destroy_meta(meta);
					return CYBERIADA_METADATA_FORMAT_ERROR;					
				}
			} else if (strcmp(line, CYBERIADA_META_EVENT_PROPAGATION) == 0) {
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
				ERROR("Error decoding SM metainformation: bad key %s\n", line);
				cyberiada_destroy_meta(meta);
				return CYBERIADA_METADATA_FORMAT_ERROR;
			}
		}

		line = strtok(NULL, CYBERIADA_META_NEW_LINE_STR);
	} while (line);

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
	if (!meta->actions_order_flag) {
		meta->actions_order_flag = 1;
	}
	if (!meta->event_propagation_flag) {
		meta->event_propagation_flag = 1;
	}
	
	sm->meta_info = meta;
	return CYBERIADA_NO_ERROR;
}*/

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
		sm->format = NULL;
		sm->meta_info = NULL;
		sm->nodes = NULL;
		sm->edges = NULL;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_cleanup_sm(CyberiadaSM* sm)
{
	CyberiadaEdge *edge, *e;
	if (sm) {
		if (sm->format) {
			free(sm->format);
		}
		if (sm->meta_info) {
			cyberiada_destroy_meta(sm->meta_info);
		}
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
		cyberiada_init_sm(sm);
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

static int node_stack_empty(NodeStack* stack)
{
	return stack == NULL;
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
	/* DEBUG("found graph %s \n", buffer); */
	if (sm->nodes == NULL) {
		sm->nodes = cyberiada_new_node(CYBERIADA_ROOT_NODE);
		sm->nodes->type = cybNodeSM;
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

	if (*(node->id) != 0 || sm->format) {
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
	current->type = cybNodeCompositeState;
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
	/* DEBUG("Set node type comment\n"); */
	/*cyberiada_copy_string(&(current->title), &(current->title_len), COMMENT_TITLE);*/
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
		current->type = cybNodeSimpleState;
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
	/* DEBUG("Set node %s title %s\n", current->id, buffer); */
	cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
	return gpsNodeAction;
}

static GraphProcessorState handle_node_behavior(xmlNode* xml_node,
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
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	if (current->type == cybNodeComment) {
		/* DEBUG("Set node %s comment text %s\n", current->id, buffer); */
		cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
	} else {
		if (current->behavior != NULL) {
			ERROR("Trying to set node %s behavior twice\n", current->id);
			return gpsInvalid;
		}
		/* DEBUG("Set node %s behavior %s\n", current->id, buffer); */
		if (cyberiada_decode_state_behavior(buffer, &(current->behavior)) != CYBERIADA_NO_ERROR) {
			ERROR("cannot decode node behavior\n");
			return gpsInvalid;
		}
	}
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
	/* DEBUG("found edge %s\n", buffer); */
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
	/* DEBUG("add edge %s %s -> %s\n", buffer, source_buffer, target_buffer); */
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
	if (current->behavior != NULL) {
		ERROR("Trying to set edge %s:%s label twice\n",
			  current->source_id, current->target_id);
		return gpsInvalid;
	}
	cyberiada_get_element_text(buffer, buffer_len, xml_node);
	/* DEBUG("add edge %s:%s behavior %s\n",
	   current->source_id, current->target_id, buffer); */
	if (cyberiada_decode_edge_behavior(buffer, &(current->behavior)) != CYBERIADA_NO_ERROR) {
		ERROR("cannot decode edge behavior\n");
		return gpsInvalid;
	}
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
		if (strcmp(buffer, CYBERIADA_FORMAT_CYBERIADAML) == 0) {
			cyberiada_copy_string(&(sm->format), &(sm->format_len),
								  CYBERIADA_FORMAT_CYBERIADAML);
			/* DEBUG("sm format %s\n", sm->format); */
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
			/* DEBUG("Set node %s title %s\n", current->id, buffer); */
			cyberiada_copy_string(&(current->title), &(current->title_len), buffer);
		} else {
			if (current->behavior != NULL) {
				ERROR("Trying to set node %s behavior twice\n", current->id);
				return gpsInvalid;
			}
			/* DEBUG("Set node %s behavior %s\n", current->id, buffer); */
			if (cyberiada_decode_state_behavior(buffer, &(current->behavior)) != CYBERIADA_NO_ERROR) {
				ERROR("cannot decode node behavior\n");
				return gpsInvalid;
			}
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
		
/*		int title = strcmp(buffer, GRAPHML_CYB_DATA_NAME) == 0;
		cyberiada_get_element_text(buffer, buffer_len, xml_node);
		if (title) {
			DEBUG("Set SM name %s\n", buffer);
			cyberiada_copy_string(&(sm->name), &(sm->name_len), buffer);
		} else {
			DEBUG("Set SM info %s\n", buffer);
			cyberiada_copy_string(&(sm->info), &(sm->info_len), buffer);
		}*/
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
		/* DEBUG("Set edge %s behavior %s\n", current->id, buffer); */
		if (cyberiada_decode_edge_behavior(buffer, &(current->behavior)) != CYBERIADA_NO_ERROR) {
			ERROR("cannot decode edge behavior\n");
			return gpsInvalid;
		}
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
	{gpsNodeAction, GRAPHML_YED_LABELNODE, &handle_node_behavior},
	{gpsNodeAction, GRAPHML_NODE_ELEMENT, &handle_new_node},
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
		/* DEBUG("xml node %s sm root %s gps %s\n",
			  cur_xml_node->name,
			  sm->nodes ? sm->nodes->id : "none",
			  debug_state_names[*gps]); */
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
	CyberiadaNode* node = NULL;
	NodeStack* stack = NULL;
	char berloga_format = 0;
	int res;

	if (cyberiada_get_attr_value(buffer, buffer_len,
								 root,
								 GRAPHML_BERLOGA_SCHEMENAME_ATTR) == CYBERIADA_NO_ERROR) {
		cyberiada_copy_string(&(sm->format), &(sm->format_len), CYBERIADA_FORMAT_BERLOGA);
		berloga_format = 1;
	} else {
		cyberiada_copy_string(&(sm->format), &(sm->format_len), CYBERIADA_FORMAT_OSTRANNA);
		berloga_format = 0;
	}
	/* DEBUG("sm format %s\n", sm->format); */
	
	if ((res = cyberiada_build_graph(root, sm, &stack, &gps,
									 yed_processor_state_table,
									 yed_processor_state_table_size)) != CYBERIADA_NO_ERROR) {
		return res;
	}
	
	if (!node_stack_empty(stack)) {
		ERROR("error with node stack\n");
		return CYBERIADA_FORMAT_ERROR;
	}

	if (berloga_format) {
		cyberiada_add_default_meta(sm, buffer);
	} else {
		if (sm->nodes) {
			node = cyberiada_graph_find_node_by_type(sm->nodes, cybNodeCompositeState);
		}
		if (node) {
			cyberiada_add_default_meta(sm, node->title);
		} else {
			cyberiada_add_default_meta(sm, EMPTY_TITLE);
		}
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

	if (!node_stack_empty(stack)) {
		ERROR("error with node stack\n");
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

	cyberiada_init_behavior_regexps();

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

		/* DEBUG("reading format %d\n", format); */
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

	cyberiada_free_behavior_regexps();
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
    return res;
}

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
	
		if (meta->actions_order_flag) {
			printf(" actions order flag: %d\n", meta->actions_order_flag);
		}
		if (meta->event_propagation_flag) {
			printf(" event propagation flag: %d\n", meta->event_propagation_flag);
		}
	}
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_print_behavior(CyberiadaBehavior* behavior, int level)
{
	char levelspace[16];
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}
	
	printf("%sBehaviors:\n", levelspace);
	while (behavior) {
		printf("%s Behavior (type %d):\n", levelspace, behavior->type);
		if(behavior->trigger) {
			printf("%s  Trigger: \"%s\"\n", levelspace, behavior->trigger);
		}		
		if(behavior->guard) {
			printf("%s  Guard: \"%s\"\n", levelspace, behavior->guard);
		}
		if(behavior->action) {
			printf("%s  Action: \"%s\"\n", levelspace, behavior->action);
		}
		behavior = behavior->next;
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
	if (node->geometry_rect) {
		printf("%sGeometry: (%lf, %lf, %lf, %lf)\n",
			   levelspace,
			   node->geometry_rect->x,
			   node->geometry_rect->y,
			   node->geometry_rect->width,
			   node->geometry_rect->height);
	}
	
	cyberiada_print_behavior(node->behavior, level + 1);
	
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

	cyberiada_print_behavior(edge->behavior, 2);
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

	printf("\nState Machine [%s]\n", sm->format);
	cyberiada_print_meta(sm->meta_info);
	
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
	const char* attr_id;
	const char* attr_for;
	const char* attr_name;
	const char* attr_type;
	const char* attr_yfiles;
} GraphMLKey;

static int cyberiada_write_sm_cyberiada(CyberiadaSM* sm, xmlTextWriterPtr writer)
{
	return CYBERIADA_NO_ERROR;
}

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
	
	if (*title) {
		XML_WRITE_TEXT(writer, title);
	}
	XML_WRITE_CLOSE_E(writer);
	
	return CYBERIADA_NO_ERROR;	
}

static int cyberiada_write_behavior_text(xmlTextWriterPtr writer, CyberiadaBehavior* behavior)
{
	int res;
	char buffer[MAX_STR_LEN];
	size_t buffer_len = sizeof(buffer) - 1;
	
	while (behavior) {

		if (*(behavior->trigger) || *(behavior->action) || *(behavior->guard)) { 
			if (*(behavior->trigger)) {
				if (behavior->type != cybBehaviorTransition) {
					snprintf(buffer, buffer_len, "%s/", behavior->trigger);
				} else {
					if (*(behavior->guard)) {
						snprintf(buffer, buffer_len, "%s [%s]/", behavior->trigger, behavior->guard);
					} else {
						snprintf(buffer, buffer_len, "%s/", behavior->trigger);
					}
				}
				XML_WRITE_TEXT(writer, buffer);
				if (behavior->next || *(behavior->action)) {
					XML_WRITE_TEXT(writer, "\n");
				}
			} else {
				if (*(behavior->action)) {
					XML_WRITE_TEXT(writer, "/\n");
				} else if (behavior->next) {
					XML_WRITE_TEXT(writer, "\n");
				}
			}
		
			if (*(behavior->action)) {
				XML_WRITE_TEXT(writer, behavior->action);
				XML_WRITE_TEXT(writer, "\n");
			}
			
			if (behavior->next) {
				XML_WRITE_TEXT(writer, "\n");
			}
		}

		behavior = behavior->next;
	}

	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_node_behavior_yed(xmlTextWriterPtr writer, CyberiadaBehavior* behavior, size_t indent)
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

	if (cyberiada_write_behavior_text(writer, behavior) != CYBERIADA_NO_ERROR) {
		ERROR("error while writing node bevavior text\n");
		return CYBERIADA_XML_ERROR;
	}
	
	XML_WRITE_CLOSE_E(writer);
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_write_edge_behavior_yed(xmlTextWriterPtr writer, CyberiadaBehavior* behavior, size_t indent)
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
	
	if (cyberiada_write_behavior_text(writer, behavior) != CYBERIADA_NO_ERROR) {
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

			if (cyberiada_write_node_behavior_yed(writer, node->behavior, indent + 3) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing simple node behavior\n");
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

			if (cyberiada_write_node_behavior_yed(writer, node->behavior, indent + 5) != CYBERIADA_NO_ERROR) {
				ERROR("error while writing composite node behavior\n");
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

	if (cyberiada_write_edge_behavior_yed(writer, edge->behavior, indent + 3) != CYBERIADA_NO_ERROR) {
		ERROR("error while writing edge behavior\n");
		return CYBERIADA_XML_ERROR;
	}
	    			 
	XML_WRITE_CLOSE_E_I(writer, indent + 2);
	XML_WRITE_CLOSE_E_I(writer, indent + 1);
	XML_WRITE_CLOSE_E_I(writer, indent);
	
	return CYBERIADA_NO_ERROR;	
}

static GraphMLKey yed_graphml_keys[] = {
	{ GRAPHML_YED_KEY_GRAPH_DESCR,    GRAPHML_GRAPH_ELEMENT,   "description", "string", NULL           },
	{ GRAPHML_YED_KEY_PORT_GRAPHICS,  GRAPHML_PORT_ATTRIBUTE,  NULL,          NULL,     "portgraphics" },
	{ GRAPHML_YED_KEY_PORT_GEOMETRY,  GRAPHML_PORT_ATTRIBUTE,  NULL,          NULL,     "portgeometry" },
	{ GRAPHML_YED_KEY_PORT_USER_DATA, GRAPHML_PORT_ATTRIBUTE,  NULL,          NULL,     "portuserdata" },
	{ GRAPHML_YED_KEY_NODE_URL,       GRAPHML_NODE_ELEMENT,    "url",         "string", NULL           },
	{ GRAPHML_YED_KEY_NODE_DESCR,     GRAPHML_NODE_ELEMENT,    "description", "string", NULL           },
	{ GRAPHML_YED_KEY_NODE_GRAPHICS,  GRAPHML_NODE_ELEMENT,    NULL,          NULL,     "nodegraphics" },
	{ GRAPHML_YED_KEY_GRAPHML_RES,    GRAPHML_GRAPHML_ELEMENT, NULL,          NULL,     "resources"    },
	{ GRAPHML_YED_KEY_EDGE_URL,       GRAPHML_EDGE_ELEMENT,    "url",         "string", NULL           },
	{ GRAPHML_YED_KEY_EDGE_DESCR,     GRAPHML_EDGE_ELEMENT,    "description", "string", NULL           },
	{ GRAPHML_YED_KEY_EDGE_GRAPHICS,  GRAPHML_EDGE_ELEMENT,    NULL,          NULL,     "edgegraphics" }
};
const size_t yed_graphml_keys_count = sizeof(yed_graphml_keys) / sizeof(GraphMLKey); 

static int cyberiada_write_sm_yed(CyberiadaSM* sm, xmlTextWriterPtr writer)
{
	size_t i;
	int res;
	GraphMLKey* key;
	CyberiadaNode* cur_node;
	CyberiadaEdge* cur_edge;
	
	XML_WRITE_OPEN_E(writer, GRAPHML_GRAPHML_ELEMENT);

	/* add YED graphml attributes */
	for (i = 0; i < yed_graphml_attributes_count; i += 2) {
		XML_WRITE_ATTR(writer, yed_graphml_attributes[i], yed_graphml_attributes[i + 1]);		
	}
	/* add scheme name if it is available */
	if (sm->meta_info && sm->meta_info->name && *sm->meta_info->name) {
		XML_WRITE_ATTR(writer, GRAPHML_BERLOGA_SCHEMENAME_ATTR, sm->meta_info->name);
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
		if (key->attr_yfiles) {
			XML_WRITE_ATTR(writer, GRAPHML_YED_YFILES_TYPE_ATTR, key->attr_yfiles);
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

int cyberiada_write_sm(CyberiadaSM* sm, const char* filename, CyberiadaXMLFormat format)
{
	xmlTextWriterPtr writer;
	int res;

	if (format != cybxmlYED) {
		ERROR("unsupported SM format for write: %d\n", format);
		return CYBERIADA_BAD_PARAMETER;
	}

	if (!sm) {
		ERROR("empty SM to write\n");
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
		res = cyberiada_write_sm_yed(sm, writer);
	} else if (format == cybxmlCyberiada) {
		res = cyberiada_write_sm_cyberiada(sm, writer);
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
