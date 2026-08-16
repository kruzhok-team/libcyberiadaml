/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The identifier simplification test
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
	CyberiadaDocument *doc, *simplified;
	CyberiadaIsomorphismResult iso;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	simplified = cyberiada_new_sm_document();
	TEST_ASSERT(simplified);
	TEST_ASSERT(cyberiada_read_sm_document(simplified, "diagrams/minimal.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_SIMPLIFY_IDS) ==
				CYBERIADA_NO_ERROR);

	/* the original ids are gone after the simplification */
	TEST_ASSERT(cyberiada_graph_find_node_by_id(doc->state_machines->nodes,
												"n0"));
	TEST_ASSERT(!cyberiada_graph_find_node_by_id(simplified->state_machines->nodes,
												 "n0"));

	/* the simplified graph is still equal to the original */
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											simplified->state_machines, 1, 0,
											&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC_MASK);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(simplified) == CYBERIADA_NO_ERROR);
	return 0;
}
