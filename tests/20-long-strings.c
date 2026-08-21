/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The long document content test
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

#include <stdio.h>
#include <cyberiadaml.h>
#include "testutils.h"

/* build count lines of the given numbered pattern joined with newlines */
static char* build_lines(const char* fmt, size_t count)
{
	size_t i, len = 0, cap = count * 32 + 1;
	char* result = (char*)malloc(cap);
	TEST_ASSERT(result);
	for (i = 0; i < count; i++) {
		len += (size_t)snprintf(result + len, cap - len, fmt, i);
		if (i + 1 < count) {
			result[len++] = '\n';
		}
	}
	result[len] = 0;
	return result;
}

int main(void)
{
	CyberiadaDocument *doc, *reread;
	CyberiadaNode* node;
	CyberiadaEdge* edge;
	CyberiadaIsomorphismResult iso;
	char *expected, *big;
	size_t i;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/long-actions.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);

	/* the 7.5K entry behavior is read completely */
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	TEST_ASSERT(node->actions);
	TEST_ASSERT(node->actions->type == cybActionEntry);
	expected = build_lines("entry_line_%04zu();", 400);
	TEST_ASSERT(strlen(node->actions->behavior) > (size_t)4096);
	TEST_ASSERT(node->actions->behavior_len == strlen(node->actions->behavior));
	TEST_ASSERT(strcmp(node->actions->behavior, expected) == 0);
	free(expected);

	/* the long guard and the 5K transition behavior are intact */
	edge = doc->state_machines->edges->next;
	TEST_ASSERT(edge && edge->action);
	TEST_ASSERT(strcmp(edge->action->trigger, "GO") == 0);
	TEST_ASSERT(strlen(edge->action->guard) > 1000);
	TEST_ASSERT(strncmp(edge->action->guard, "cond000 && ", 11) == 0);
	TEST_ASSERT(strcmp(edge->action->guard + strlen(edge->action->guard) - 7,
					   "cond099") == 0);
	expected = build_lines("step_%04zu();", 400);
	TEST_ASSERT(strcmp(edge->action->behavior, expected) == 0);
	free(expected);

	/* the 8K unicode comment ends with the exact final characters */
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes,
										   "nComment");
	TEST_ASSERT(node);
	TEST_ASSERT(node->comment_data);
	TEST_ASSERT(node->comment_data->body);
	TEST_ASSERT(strlen(node->comment_data->body) > (size_t)4096);
	TEST_ASSERT(strcmp(node->comment_data->body +
					   strlen(node->comment_data->body) - strlen("конец✓"),
					   "конец✓") == 0);

	/* the content survives the write/read round trip byte-exactly */
	TEST_ASSERT(cyberiada_write_sm_document(doc, "20-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, "20-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	node = cyberiada_graph_find_node_by_id(reread->state_machines->nodes, "n0");
	TEST_ASSERT(node && node->actions);
	expected = build_lines("entry_line_%04zu();", 400);
	TEST_ASSERT(strcmp(node->actions->behavior, expected) == 0);
	free(expected);
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											   reread->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);

	/* the string utility accepts arbitrary lengths */
	big = (char*)malloc(100001);
	TEST_ASSERT(big);
	for (i = 0; i < 100000; i++) {
		big[i] = 'a' + (char)(i % 26);
	}
	big[100000] = 0;
	expected = NULL;
	TEST_ASSERT(cyberiada_copy_string(&expected, &i, big) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(i == 100000);
	TEST_ASSERT(strcmp(expected, big) == 0);
	free(expected);
	free(big);

	return 0;
}
