/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The yEd writing restrictions test
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

	/* the vertexes the yEd format cannot express are reported, not dropped */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/terminate.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "25-out.graphml",
											cybxmlYEDOstranna,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_BAD_PARAMETER);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "25-out.graphml",
											cybxmlYEDBerloga16,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_BAD_PARAMETER);
	/* the same document is fine in the Cyberiada format */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "25-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	/* the yEd format requires the geometry */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "25-out.graphml",
											cybxmlYEDOstranna,
											CYBERIADA_FLAG_SKIP_GEOMETRY) ==
				CYBERIADA_BAD_PARAMETER);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the yEd format keeps a single state machine only */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/two-machines.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "25-out.graphml",
											cybxmlYEDOstranna,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_BAD_PARAMETER);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
