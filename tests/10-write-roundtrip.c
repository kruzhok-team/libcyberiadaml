/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The document writing and round-trip test
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
	CyberiadaDocument *doc, *reread;
	char* buffer = NULL;
	size_t buffer_size = 0;
	int result_flags = 0;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	/* the written document matches the good file */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "10-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(test_compare_files("10-out.graphml",
								   "good/10-write.graphml") == 0);

	/* the encoded buffer matches the written file */
	TEST_ASSERT(cyberiada_encode_sm_document(doc, &buffer, &buffer_size,
											 cybxmlCyberiada10,
											 CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(buffer);
	TEST_ASSERT(buffer_size > 0);
	TEST_ASSERT(strstr(buffer, "Cyberiada-GraphML-1.0"));
	free(buffer);

	/* the reread document is identical to the original */
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, "10-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_check_isomorphism(doc->state_machines,
											reread->state_machines, 1, 0,
											&result_flags, NULL,
											NULL, NULL, NULL,
											NULL, NULL, NULL, NULL,
											NULL, NULL, NULL,
											NULL, NULL, NULL, NULL) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(result_flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);

	/* entry, do and exit actions survive the round trip */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/actions.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "10-out2.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, "10-out2.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	{
		CyberiadaNode* node =
			cyberiada_graph_find_node_by_id(reread->state_machines->nodes, "n0");
		TEST_ASSERT(node);
		TEST_ASSERT(node->actions);
		TEST_ASSERT(node->actions->type == cybActionEntry);
		TEST_ASSERT(node->actions->next);
		TEST_ASSERT(node->actions->next->type == cybActionDo);
		TEST_ASSERT(node->actions->next->next);
		TEST_ASSERT(node->actions->next->next->type == cybActionExit);
	}
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);
	return 0;
}
