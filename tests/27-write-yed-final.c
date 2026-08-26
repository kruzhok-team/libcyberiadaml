/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The yEd final state writing test
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

#include <string.h>

#include <cyberiadaml.h>
#include "testutils.h"

int main(void)
{
	CyberiadaDocument *doc, *written;
	CyberiadaNode* node;
	CyberiadaIsomorphismResult iso;

	/* the final state is the BPMN event with the end characteristic */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/ostranna-final.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->format);
	TEST_ASSERT(strcmp(doc->format, "yEd Ostranna") == 0);
	TEST_ASSERT(doc->state_machines && doc->state_machines->nodes);
	node = cyberiada_graph_find_node_by_type(doc->state_machines->nodes, cybNodeFinal);
	TEST_ASSERT(node);
	/* the vertexes are stored as points */
	TEST_ASSERT(node->geometry_point && !node->geometry_rect);

	TEST_ASSERT(cyberiada_write_sm_document(doc, "27-out.graphml",
											cybxmlYEDOstranna,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	written = cyberiada_new_sm_document();
	TEST_ASSERT(written);
	TEST_ASSERT(cyberiada_read_sm_document(written, "27-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	/* the final state survives the writing */
	node = cyberiada_graph_find_node_by_type(written->state_machines->nodes, cybNodeFinal);
	TEST_ASSERT(node);
	TEST_ASSERT(node->geometry_point);

	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											   written->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(written) == CYBERIADA_NO_ERROR);
	return 0;
}
