/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The standard appendix examples test
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

#include <math.h>
#include <cyberiadaml.h>
#include "testutils.h"

/* read a standard example, write it back and check the identical round trip */
static void check_roundtrip(const char* filename, const char* out_filename)
{
	CyberiadaDocument *doc, *reread;
	CyberiadaIsomorphismResult iso;

	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown,
										   CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, out_filename,
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	reread = cyberiada_new_sm_document();
	TEST_ASSERT(reread);
	TEST_ASSERT(cyberiada_read_sm_document(reread, out_filename, cybxmlUnknown,
										   CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_check_sm_isomorphism(doc->state_machines,
											reread->state_machines, 1, 0,
											&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(iso.flags & CYBERIADA_ISOMORPH_FLAG_IDENTICAL);
	TEST_ASSERT(cyberiada_cleanup_isomorphism_result(&iso) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(reread) == CYBERIADA_NO_ERROR);
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaNode* node;

	/* the minimal document (Г.1): loads and round trips */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/standard-minimal.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->meta_info);
	TEST_ASSERT(doc->meta_info->standard_version);
	TEST_ASSERT(strcmp(doc->meta_info->standard_version, "1.0") == 0);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "17-minimal-out.graphml",
											cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the state machine examples (Г.2-Г.4) round trip identically */
	check_roundtrip("samples/standard-hoover.graphml", "17-hoover-out.graphml");
	check_roundtrip("samples/standard-arduino.graphml", "17-arduino-out.graphml");
	check_roundtrip("samples/standard-geometry.graphml", "17-geometry-out.graphml");
	check_roundtrip("samples/standard-hoover2.graphml", "17-hoover2-out.graphml");

	/* the base format may size the rects loosely (7.2.2) */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/standard-hoover2.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(doc->geometry_format == cybgeomShort);
	node = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, "n0::n1");
	TEST_ASSERT(node && node->geometry_rect);
	/* the zero size is replaced, the coordinates of the file are kept */
	TEST_ASSERT(node->geometry_rect->width > 0.0);
	TEST_ASSERT(node->geometry_rect->height > 0.0);
	TEST_ASSERT(fabs(node->geometry_rect->x - 50.0) < 0.01);
	TEST_ASSERT(fabs(node->geometry_rect->y - 100.0) < 0.01);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the good print of the robot-vacuum example */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "samples/standard-hoover.graphml",
										   cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	test_capture_stdout("17-read-standard.out");
	TEST_ASSERT(cyberiada_print_sm_document(doc) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(test_check_good("17-read-standard.out",
								  "good/17-read-standard.txt") == 0);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
