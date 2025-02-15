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

#ifndef __CYBERIADA_GRAPH_H
#define __CYBERIADA_GRAPH_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id);
	CyberiadaNode* cyberiada_graph_find_node_by_type(CyberiadaNode* root, CyberiadaNodeTypeMask mask);
	CyberiadaEdge* cyberiada_graph_find_edge_by_id(CyberiadaEdge* root, const char* id);
	int cyberiada_graph_add_sibling_node(CyberiadaNode* sibling, CyberiadaNode* new_node);
	int cyberiada_graph_add_edge(CyberiadaSM* sm, const char* id, const char* source, const char* target, int external);
	CyberiadaEdge* cyberiada_graph_find_last_edge(CyberiadaSM* sm);

#ifdef __cplusplus
}
#endif
    
#endif
