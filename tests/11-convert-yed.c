/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The yEd to Cyberiada-GraphML conversion test
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
	CyberiadaDocument *doc, *converted;
	size_t vertexes1 = 0, edges1 = 0, vertexes2 = 0, edges2 = 0;
	int result_flags = 0;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/yed-geometry.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_write_sm_document(doc, "11-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	converted = cyberiada_new_sm_document();
	TEST_ASSERT(converted);
	TEST_ASSERT(cyberiada_read_sm_document(converted, "11-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(converted->format);
	TEST_ASSERT(strcmp(converted->format, "Cyberiada-GraphML-1.0") == 0);

	TEST_ASSERT(cyberiada_sm_size(doc->state_machines, &vertexes1, &edges1,
								  1, 1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_sm_size(converted->state_machines, &vertexes2, &edges2,
								  1, 1) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(vertexes1 == vertexes2);
	TEST_ASSERT(edges1 == edges2);

	TEST_ASSERT(cyberiada_check_isomorphism(doc->state_machines,
											converted->state_machines, 1, 0,
											&result_flags, NULL,
											NULL, NULL, NULL,
											NULL, NULL, NULL, NULL,
											NULL, NULL, NULL,
											NULL, NULL, NULL, NULL) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(result_flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(converted) == CYBERIADA_NO_ERROR);
	return 0;
}
