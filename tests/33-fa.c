/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library tests
 *
 * The EXT-FA rules: the finite automaton documents of compat-tests 374e6d0
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <cyberiadaml.h>
#include "testutils.h"

#define ROUNDTRIP "33-fa-roundtrip.graphml"

static const char* positives[] = {
	"F-FA-EXAMPLE", "F-FA-MEALY", "F-FA-MOORE", "F-FA-ALPHABET", "F-FA-NESTED"
};

static const char* negatives[] = {
	"X-FA-5-2-two-machines", "X-FA-5-3-propagate",
	"X-FA-3-1-choice", "X-FA-3-1-collapsed",
	"X-FA-3.1-1-exit-action", "X-FA-3.1-1-two-line-entry",
	"X-FA-3.1.1-1-two-regions", "X-FA-3.1.1-2-composite-no-initial",
	"X-FA-3.1.1-3-two-transitions-same-symbol",
	"X-FA-3.2-1-no-initial", "X-FA-3.2-2-initial-with-event",
	"X-FA-3.3-1-nested-final", "X-FA-3.3-2-eof-to-state", "X-FA-3.3-2-symbol-to-final",
	"X-FA-3.4-1-guard", "X-FA-3.4-1-two-events", "X-FA-3.4-1-empty-event",
	"X-FA-3.4-2-two-line-action", "X-FA-3.4-3-reserved-symbol",
	"X-FA-4-1-two-input-alphabets", "X-FA-4-2-star-in-regex",
	"X-FA-4-3-symbol-not-in-alphabet"
};

static int read_document(const char* name, int flags, int* is_fa)
{
	char path[256];
	CyberiadaDocument* doc;
	int res;
	snprintf(path, sizeof(path), "diagrams/fa/%s.graphml", name);
	doc = cyberiada_new_sm_document();
	if (!doc) {
		return CYBERIADA_ASSERT;
	}
	res = cyberiada_read_sm_document(doc, path, cybxmlCyberiada10, flags);
	if (is_fa) {
		*is_fa = cyberiada_document_is_fa(doc);
	}
	cyberiada_destroy_sm_document(doc);
	return res;
}

int main(void)
{
	size_t i;
	int is_fa;
	CyberiadaDocument* doc;

	/* the valid automata read in both modes and are recognised */
	for (i = 0; i < sizeof(positives) / sizeof(const char*); i++) {
		TEST_ASSERT(read_document(positives[i], CYBERIADA_FLAG_NO, &is_fa) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(is_fa);
		TEST_ASSERT(read_document(positives[i], CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_NO_ERROR);
	}

	/* the single-fault documents are refused in both modes */
	for (i = 0; i < sizeof(negatives) / sizeof(const char*); i++) {
		TEST_ASSERT(read_document(negatives[i], CYBERIADA_FLAG_NO, NULL) == CYBERIADA_FA_ERROR);
		TEST_ASSERT(read_document(negatives[i], CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_FA_ERROR);
	}

	/* the same fault without type/ fa is an ordinary document */
	TEST_ASSERT(read_document("not-fa-guard", CYBERIADA_FLAG_NO, &is_fa) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(!is_fa);
	TEST_ASSERT(read_document("not-fa-guard", CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_NO_ERROR);

	/* the writer checks the rules too and the written automaton reads back */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, "diagrams/fa/F-FA-ALPHABET.graphml",
										   cybxmlCyberiada10, CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, ROUNDTRIP, cybxmlCyberiada10,
											CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
	cyberiada_destroy_sm_document(doc);
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, ROUNDTRIP, cybxmlCyberiada10,
										   CYBERIADA_FLAG_STRICT) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_document_is_fa(doc));
	cyberiada_destroy_sm_document(doc);

	TEST_ASSERT(strcmp(cyberiada_error_str(CYBERIADA_FA_ERROR), "Unknown error code") != 0);
	return 0;
}
