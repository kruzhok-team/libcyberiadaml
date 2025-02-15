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

#ifndef __CYBERIADA_GRAPH_RECON_H
#define __CYBERIADA_GRAPH_RECON_H

#include "cyberiadaml.h"
#include "cyb_structs.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef CyberiadaList NamesList;
	void cyberiada_free_name_list(NamesList** nl);
	int cyberiada_graphs_reconstruct_node_identifiers(CyberiadaNode* root, NamesList** nl);
	int cyberiada_graphs_reconstruct_edge_identifiers(CyberiadaDocument* doc, NamesList** nl);
	
#ifdef __cplusplus
}
#endif
    
#endif
