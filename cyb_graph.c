/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The graph manipulation functions
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

#include <string.h>

#include "cyb_graph.h"
#include "cyb_error.h"

CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id)
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

CyberiadaNode* cyberiada_graph_find_node_by_type(CyberiadaNode* root, CyberiadaNodeTypeMask mask)
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

CyberiadaEdge* cyberiada_graph_find_edge_by_id(CyberiadaEdge* root, const char* id)
{
	CyberiadaEdge* edge;
	for (edge = root; edge; edge = edge->next) {
		if (edge->id && strcmp(edge->id, id) == 0) {
			return edge;
		}
	}
	return NULL;
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

/*int cyberiada_graph_add_node_action(CyberiadaNode* node,
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

int cyberiada_graph_add_edge(CyberiadaSM* sm, const char* id, const char* source, const char* target, int external)
{
	CyberiadaEdge* last_edge;
	CyberiadaEdge* new_edge;
	if (!sm) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (*id && cyberiada_graph_find_edge_by_id(sm->edges, id) != NULL) {
		ERROR("The edge with the id %s already exists in the SM\n", id);
		return CYBERIADA_BAD_PARAMETER;
	}
	new_edge = cyberiada_new_edge(id, source, target, external);
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
