/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The standard requirements test
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

static int read_document(const char* path, int flags)
{
	CyberiadaDocument* doc;
	int res;
	doc = cyberiada_new_sm_document();
	if (!doc) {
		return CYBERIADA_ASSERT;
	}
	res = cyberiada_read_sm_document(doc, path, cybxmlCyberiada10, flags);
	cyberiada_destroy_sm_document(doc);
	return res;
}

int main(void)
{
	CyberiadaDocument* doc;

	/* the requirements checked in any mode */
	TEST_ASSERT(read_document("diagrams/no-meta.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_METADATA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/bad-formal-name.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/foreign-tag.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/wrong-key-for.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);

	/* the document without graphs has no metainformation either */
	TEST_ASSERT(read_document("diagrams/no-graphs.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_METADATA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/no-graphs.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);

	/* the requirements checked in the strict mode only */
	TEST_ASSERT(read_document("diagrams/bad-id-char.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/bad-id-char.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/no-marker.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/no-marker.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/no-sm-name.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/no-sm-name.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/vertex-not-first.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/vertex-not-first.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/key-attr-type.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/key-attr-type.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/two-markers.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/two-markers.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/submachine-not-first.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/submachine-not-first.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/two-else.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/two-else.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/no-chunk.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/no-chunk.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/link-self-loop.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/link-self-loop.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/link-targets-link.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/link-targets-link.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/chunk-target-point.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/chunk-target-point.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/collapsed-no-regions.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/collapsed-no-regions.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/bad-color.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/bad-color.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/color-on-graph.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/color-on-graph.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/repeated-meta-param.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/repeated-meta-param.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_METADATA_FORMAT_ERROR);
	TEST_ASSERT(read_document("diagrams/component-no-type.graphml", CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/component-no-type.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_FORMAT_ERROR);

	/* the strict mode keeps the correct documents readable */
	TEST_ASSERT(read_document("diagrams/minimal.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/two-machines.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/completeness.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/remapped-keys.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(read_document("diagrams/short-geometry.graphml", CYBERIADA_FLAG_STRICT) ==
				CYBERIADA_NO_ERROR);

	/* the empty document is reported, not written */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_write_sm_document(doc, "21-out.graphml", cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) != CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	return 0;
}
