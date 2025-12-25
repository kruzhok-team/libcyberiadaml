/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The graph reconstruction functions
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

#include <stdio.h>
#include <stdlib.h>

#include "cyb_graph_recon.h"
#include "cyb_string.h"
#include "cyb_error.h"
#include "cyb_graph.h"

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

void cyberiada_free_name_list(NamesList** nl)
{
	if (nl) {
		NamesList* list = *nl;
		while(list) {
			if (list->key) free(list->key);
			if (list->data) free(list->data);			
			list = list->next;
		}
		cyberiada_list_free(nl);
	}
}

int cyberiada_graphs_reconstruct_node_identifiers(CyberiadaNode* root, NamesList** nl, int rename)
{
	CyberiadaNode *node;
	unsigned int num = 0;
	
	node = root;
	while (node) {
		if (!node->id) {
			ERROR("Found null node id\n");
			return CYBERIADA_FORMAT_ERROR;
		}
		if (rename || !*(node->id)) {
			char buffer[MAX_STR_LEN];
			size_t buffer_len = sizeof(buffer) - 1;
			char *key, *data;

			do {
				if (node->parent && node->parent->parent) {
					snprintf(buffer, buffer_len, "%s::n%u",
							 node->parent->id, num);
				} else {
					if (!node->parent) {
						snprintf(buffer, buffer_len, "g%u", num);
					} else {
						snprintf(buffer, buffer_len, "n%u", num);
					}
				}
				num++;
			} while (cyberiada_graph_find_node_by_id(root, buffer));

			cyberiada_copy_string(&key, NULL, node->id);
			cyberiada_copy_string(&data, NULL, buffer);
			cyberiada_add_name_to_list(nl, key, data);

			/*DEBUG("rename %s -> %s\n", key, data);*/

			free(node->id);
			node->id = NULL;
			cyberiada_copy_string(&(node->id), &(node->id_len), buffer);
		}
		if (node->children) {
			cyberiada_graphs_reconstruct_node_identifiers(node->children, nl, rename);
		}
		node = node->next;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_graphs_reconstruct_edge_identifiers(CyberiadaDocument* doc, NamesList** nl, int rename)
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
			if (rename || !*(edge->source_id)) {
				new_id = cyberiada_find_name_in_list(nl, edge->source_id);
				if (!new_id) {
					ERROR("Cannot find replacement for source id %s\n", edge->source_id);
					return CYBERIADA_FORMAT_ERROR;
				}
				free(edge->source_id);
				edge->source_id = NULL;
				cyberiada_copy_string(&(edge->source_id), &(edge->source_id_len), new_id);
			}
			if (rename || !*(edge->target_id)) {
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
			if (rename || !edge->id || !*(edge->id)) {
				snprintf(buffer, buffer_len, "%s-%s", edge->source_id, edge->target_id);
				while (cyberiada_graph_find_edge_by_id(sm->edges, buffer)) {
					snprintf(buffer, buffer_len, "%s-%s#%u",
							 edge->source_id, edge->target_id, num);
					num++;
				}
				if (edge->id) free(edge->id);
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
