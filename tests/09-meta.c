/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The metainformation test
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
	CyberiadaMetaStringList* string;
	char* body = NULL;
	size_t body_len = 0;
	int found = 0;

	/* the metainformation of the minimal document */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->meta_info);
	TEST_ASSERT(doc->meta_info->standard_version);
	TEST_ASSERT(strcmp(doc->meta_info->standard_version, "1.0") == 0);
	TEST_ASSERT(doc->meta_info->transition_order_flag == 1);
	TEST_ASSERT(doc->meta_info->event_propagation_flag == 1);
	for (string = doc->meta_info->strings; string; string = string->next) {
		if (strcmp(string->name, "name") == 0) {
			TEST_ASSERT(strcmp(string->value, "minimal") == 0);
			found = 1;
		}
	}
	TEST_ASSERT(found);

	/* metainformation encoding */
	TEST_ASSERT(cyberiada_encode_meta(doc->meta_info, &body, &body_len) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(body);
	TEST_ASSERT(body_len == strlen(body));
	TEST_ASSERT(strstr(body, "standardVersion/ 1.0"));
	TEST_ASSERT(strstr(body, "name/ minimal"));
	TEST_ASSERT(strstr(body, "transitionOrder/ transitionFirst"));
	TEST_ASSERT(strstr(body, "eventPropagation/ block"));
	free(body);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the wrong standard version is rejected */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/bad-version.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the metainformation node is ignored with the skip-meta flag */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "fixtures/minimal.graphml",
										   cybxmlUnknown,
										   CYBERIADA_FLAG_SKIP_META) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
