/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The legacy yEd document reading test
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

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/yed-geometry.graphml",
										   cybxmlYED, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->format);
	TEST_ASSERT(doc->state_machines);

	test_capture_stdout("05-read-yed.out");
	TEST_ASSERT(cyberiada_print_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(test_check_good("05-read-yed.out",
								  "good/05-read-yed.txt") == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	return 0;
}
