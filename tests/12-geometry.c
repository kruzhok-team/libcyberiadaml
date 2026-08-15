/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The document geometry test
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 * ----------------------------------------------------------------------------- */

#include <cyberiadaml.h>
#include "testutils.h"

int main(void)
{
	CyberiadaDocument* doc;

	/* the geometry is imported by default */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);

	/* format conversion keeps the geometry */
	TEST_ASSERT(cyberiada_convert_document_geometry(doc, coordAbsolute,
													coordAbsolute, coordAbsolute,
													edgeCenter) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(doc->node_coord_format == coordAbsolute);

	/* reconstruction rebuilds the geometry from scratch */
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);

	/* cleaning removes the geometry */
	TEST_ASSERT(cyberiada_clean_document_geometry(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 0);

	/* reconstruction works on a cleaned document */
	TEST_ASSERT(cyberiada_reconstruct_document_geometry(doc, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the skip-geometry flag drops the geometry on import */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/minimal.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_SKIP_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the reconstruction flag builds geometry for a document without one */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/actions.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY |
										   CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_has_geometry(doc) == 1);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
