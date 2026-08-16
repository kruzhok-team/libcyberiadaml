/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Cyberiada-GraphML 1.0 document reading test
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
	size_t vertexes = 0, edges = 0;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->format);
	TEST_ASSERT(strcmp(doc->format, "Cyberiada-GraphML-1.0") == 0);
	TEST_ASSERT(doc->state_machines);
	TEST_ASSERT(!doc->state_machines->next);
	TEST_ASSERT(cyberiada_sm_size(doc->state_machines, &vertexes, &edges, 1, 1) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(vertexes == 3);
	TEST_ASSERT(edges == 3);

	test_capture_stdout("03-read-minimal.out");
	TEST_ASSERT(cyberiada_print_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(test_check_golden("03-read-minimal.out",
								  "golden/03-read-minimal.txt") == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
