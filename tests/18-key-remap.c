/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The key remapping and parallel decoding test
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

#include <pthread.h>
#include <cyberiadaml.h>
#include "testutils.h"

#define READER_THREADS  4
#define READER_ROUNDS   20

static void check_remapped_document(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode* node;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/remapped-keys.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->state_machines);
	TEST_ASSERT(doc->state_machines->nodes);
	TEST_ASSERT(strcmp(doc->state_machines->nodes->title, "Remapped SM") == 0);
	TEST_ASSERT(doc->meta_info);
	TEST_ASSERT(strcmp(doc->meta_info->standard_version, "1.0") == 0);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0");
	TEST_ASSERT(node);
	TEST_ASSERT(strcmp(node->title, "Off") == 0);
	TEST_ASSERT(node->actions);
	TEST_ASSERT(node->actions->type == cybActionEntry);
	TEST_ASSERT(strcmp(node->actions->behavior, "off();") == 0);

	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "init");
	TEST_ASSERT(node);
	TEST_ASSERT(node->type == cybNodeInitial);

	TEST_ASSERT(doc->state_machines->edges);
	TEST_ASSERT(doc->state_machines->edges->next);
	TEST_ASSERT(doc->state_machines->edges->next->action);
	TEST_ASSERT(strcmp(doc->state_machines->edges->next->action->trigger,
					   "TURN_ON") == 0);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
}

static void* reader_thread(void* arg)
{
	int rounds = READER_ROUNDS;
	int failures = 0;
	const char* filename = (const char*)arg;
	while (rounds--) {
		CyberiadaDocument* doc = cyberiada_new_sm_document();
		if (!doc ||
			cyberiada_read_sm_document(doc, filename, cybxmlUnknown,
									   CYBERIADA_FLAG_NO) != CYBERIADA_NO_ERROR ||
			!doc->state_machines || !doc->state_machines->nodes) {
			failures++;
		}
		if (doc) {
			cyberiada_destroy_sm_document(doc);
		}
	}
	return failures ? (void*)1 : NULL;
}

int main(void)
{
	pthread_t threads[READER_THREADS];
	const char* files[READER_THREADS] = {
		"diagrams/remapped-keys.graphml",
		"diagrams/minimal.graphml",
		"diagrams/remapped-keys.graphml",
		"samples/standard-hoover.graphml"
	};
	void* thread_res;
	int i;

	/* the non-default key ids are resolved through the per-parse map */
	check_remapped_document();

	/* the next parse in the same process is not affected by the previous map */
	check_remapped_document();

	/* parallel decoding of remapped and standard documents */
	for (i = 0; i < READER_THREADS; i++) {
		TEST_ASSERT(pthread_create(&threads[i], NULL, reader_thread,
								   (void*)files[i]) == 0);
	}
	for (i = 0; i < READER_THREADS; i++) {
		TEST_ASSERT(pthread_join(threads[i], &thread_res) == 0);
		TEST_ASSERT(thread_res == NULL);
	}

	return 0;
}
