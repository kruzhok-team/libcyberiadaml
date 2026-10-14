/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The basic graph types
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

#ifndef __CYBERIADA_TYPES_H
#define __CYBERIADA_TYPES_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif

	int cyberiada_update_complex_state(CyberiadaNode* node, CyberiadaNode* parent);
	int cyberiada_destroy_node(CyberiadaNode* node);
	int cyberiada_destroy_edge(CyberiadaEdge* e);
	
#ifdef __cplusplus
}
#endif
    
#endif
