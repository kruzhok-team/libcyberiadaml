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
#define __CYBERIADA_GEOMRTRY_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif
	
/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML geometry utilities
 * ----------------------------------------------------------------------------- */

	CyberiadaPoint*    cyberiada_new_point(void);
	CyberiadaPoint*    cyberiada_copy_point(CyberiadaPoint* src);

	CyberiadaRect*     cyberiada_new_rect(void);
	CyberiadaRect*     cyberiada_copy_rect(CyberiadaRect* src);
	
	CyberiadaPolyline* cyberiada_new_polyline(void);
	CyberiadaPolyline* cyberiada_copy_polyline(CyberiadaPolyline* src);
	int                cyberiada_destroy_polyline(CyberiadaPolyline* polyline);

	int                cyberiada_clean_document_geometry(CyberiadaDocument* doc);
	int                cyberiada_convert_document_geometry(CyberiadaDocument* doc,
														   CyberiadaGeometryCoordFormat new_format,
														   CyberiadaGeometryEdgeFormat new_edge_format);
	int                cyberiada_import_document_geometry(CyberiadaDocument* doc,
														  int flags, CyberiadaXMLFormat file_format);
	int                cyberiada_export_document_geometry(CyberiadaDocument* doc,
														  int flags, CyberiadaXMLFormat file_format);
	
#ifdef __cplusplus
}
#endif
    
#endif
