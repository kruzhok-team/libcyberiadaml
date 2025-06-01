/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The geometry utilities
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

#ifndef __CYBERIADA_GEOMETRY_H
#define __CYBERIADA_GEOMETRY_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif
	
/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML geometry utilities
 * ----------------------------------------------------------------------------- */

#define NODE_GEOMETRY_RECT   (cybNodeSM | cybNodeSimpleState | cybNodeCompositeState | cybNodeRegion | \
	                          cybNodeSubmachineState | cybNodeComment | cybNodeFormalComment | \
                              cybNodeChoice)
#define NODE_GEOMETRY_POINT  (cybNodeInitial | cybNodeFinal | cybNodeTerminate | \
	                          cybNodeEntryPoint | cybNodeExitPoint | cybNodeShallowHistory | cybNodeDeepHistory | \
	                          cybNodeFork | cybNodeJoin)
	
	int                cyberiada_document_no_geometry(CyberiadaDocument* doc);
	int                cyberiada_clean_document_geometry(CyberiadaDocument* doc);
	int                cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
	int                cyberiada_convert_document_geometry(CyberiadaDocument* doc,
														   CyberiadaGeometryCoordFormat new_node_coord_format,
														   CyberiadaGeometryCoordFormat new_edge_coord_format,
														   CyberiadaGeometryCoordFormat new_edge_pl_coord_format,
														   CyberiadaGeometryEdgeFormat new_edge_format);
	int                cyberiada_import_document_geometry(CyberiadaDocument* doc,
														  int flags, CyberiadaXMLFormat file_format);
	int                cyberiada_export_document_geometry(CyberiadaDocument* doc,
														  int flags, CyberiadaXMLFormat file_format);
	int                cyberiada_document_has_geometry(CyberiadaDocument* doc);
	int                cyberiada_check_nodes_geometry(CyberiadaNode* nodes);
	
#ifdef __cplusplus
}
#endif
    
#endif
