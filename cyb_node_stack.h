/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The node stack structure
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

#ifndef __CYBERIADA_NODE_STACK_H
#define __CYBERIADA_NODE_STACK_H

#include "cyberiadaml.h"
#include "cyb_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML XML processor node stack
 * ----------------------------------------------------------------------------- */

	typedef CyberiadaStack NodeStack;
    /* key  - const char* graph_element
       data - CyberiadaNode* node */

	int node_stack_push(NodeStack** stack);
	int node_stack_set_top_node(NodeStack** stack, CyberiadaNode* node);
	int node_stack_set_top_element(NodeStack** stack, const char* element);
	CyberiadaNode* node_stack_current_node(NodeStack** stack);
	int node_stack_pop(NodeStack** stack);
	int node_stack_empty(NodeStack** stack);
	int node_stack_free(NodeStack** stack);
	
#ifdef __cplusplus
}
#endif
    
#endif
