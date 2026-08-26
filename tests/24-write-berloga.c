/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The yEd Berloga 1.6 writing test
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
	CyberiadaIsomorphismResult iso;
	CyberiadaMetaStringList* string;
	int found = 0;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/berloga16-auto.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->format);
	TEST_ASSERT(strcmp(doc->format, "yEd Berloga-1.6") == 0);

	TEST_ASSERT(cyberiada_write_sm_document(doc, "24-out.graphml",
											cybxmlYEDBerloga16,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(test_compare_files("24-out.graphml",
								   "good/24-write-berloga.graphml") == 0);

	/* the core meta node carries the metainformation back */
	written = cyberiada_new_sm_document();
	TEST_ASSERT(written);
	TEST_ASSERT(cyberiada_read_sm_document(written, "24-out.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(written->format);
	TEST_ASSERT(strcmp(written->format, "yEd Berloga-1.6") == 0);
	TEST_ASSERT(written->meta_info);
	for (string = written->meta_info->strings; string; string = string->next) {
		if (strcmp(string->name, "target") == 0) {
			TEST_ASSERT(strcmp(string->value, "Stapler") == 0);
			found = 1;
		}
	}
	TEST_ASSERT(found);

	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											   written->state_machines, 1, 0,
											   &iso) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) == CYBERIADA_NO_ERROR);

	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(written) == CYBERIADA_NO_ERROR);
	return 0;
}
