/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library tests
 *
 * The EXT-FA rules: the finite automaton documents of compat-tests 374e6d0, the strict
 * mode refusing an improper automaton and the listing of its violations
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
#define DRAFT     "33-fa-draft.graphml"

static const char* positives[] = {
	"F-FA-EXAMPLE", "F-FA-MEALY", "F-FA-MOORE", "F-FA-ALPHABET", "F-FA-NESTED",
	"codes-output"
};

/* the automata written and read back */
static const char* roundtrips[] = {
	"F-FA-ALPHABET", "codes-output"
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

/* the requirement a single-fault document is named after: X-FA-<clause>-<n>-... */
static void expected_requirement(const char* name, char* requirement, size_t size)
{
	const char* clause = name + strlen("X-FA-");
	const char* dash = strchr(clause, '-');
	const char* end = dash ? strchr(dash + 1, '-') : NULL;
	int n = end ? (int)(end - clause) : (int)strlen(clause);
	snprintf(requirement, size, "CGML-FA-%.*s", n, clause);
}

static size_t findings_count(const CyberiadaFAFinding* f)
{
	size_t n = 0;
	for (; f; f = f->next) {
		n++;
	}
	return n;
}

static const CyberiadaFAFinding* findings_find(const CyberiadaFAFinding* f, const char* requirement)
{
	for (; f; f = f->next) {
		if (f->requirement && strcmp(f->requirement, requirement) == 0) {
			return f;
		}
	}
	return NULL;
}

static CyberiadaDocument* open_document(const char* name, int flags)
{
	char path[256];
	CyberiadaDocument* doc;
	snprintf(path, sizeof(path), "diagrams/fa/%s.graphml", name);
	doc = cyberiada_new_sm_document();
	if (doc && cyberiada_read_sm_document(doc, path, cybxmlCyberiada10, flags) != CYBERIADA_NO_ERROR) {
		cyberiada_destroy_sm_document(doc);
		return NULL;
	}
	return doc;
}

/* the single-fault documents whose fault is a construct outside the FA element set */
static int expected_foreign(const char* name)
{
	static const char* foreign[] = {
		"X-FA-5-2-two-machines", "X-FA-5-3-propagate", "X-FA-3-1-choice", "X-FA-3-1-collapsed",
		"X-FA-3.1-1-exit-action", "X-FA-3.1.1-1-two-regions", "X-FA-3.3-1-nested-final",
		"X-FA-3.4-1-guard"
	};
	size_t i;
	for (i = 0; i < sizeof(foreign) / sizeof(const char*); i++) {
		if (strcmp(foreign[i], name) == 0) {
			return 1;
		}
	}
	return 0;
}

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
	char requirement[64];
	CyberiadaDocument* doc;
	CyberiadaFAFinding* findings;
	const CyberiadaFAFinding* f;

	/* the valid automata read in both modes, are recognised and list nothing */
	for (i = 0; i < sizeof(positives) / sizeof(const char*); i++) {
		TEST_ASSERT(read_document(positives[i], CYBERIADA_FLAG_NO, &is_fa) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(is_fa);
		TEST_ASSERT(read_document(positives[i], CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_NO_ERROR);
		doc = open_document(positives[i], CYBERIADA_FLAG_NO);
		TEST_ASSERT(doc);
		TEST_ASSERT(cyberiada_check_fa_document(doc, &findings) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(findings == NULL);
		cyberiada_destroy_sm_document(doc);
	}

	/* a single-fault document is refused by the strict mode; the lenient mode reads and
	   writes it as a draft and the listing names its requirement once */
	for (i = 0; i < sizeof(negatives) / sizeof(const char*); i++) {
		TEST_ASSERT(read_document(negatives[i], CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_FA_ERROR);
		TEST_ASSERT(read_document(negatives[i], CYBERIADA_FLAG_NO, &is_fa) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(is_fa);
		doc = open_document(negatives[i], CYBERIADA_FLAG_NO);
		TEST_ASSERT(doc);
		TEST_ASSERT(cyberiada_check_fa_document(doc, &findings) == CYBERIADA_FA_ERROR);
		expected_requirement(negatives[i], requirement, sizeof(requirement));
		f = findings_find(findings, requirement);
		if (!f || findings_count(findings) != 1) {
			fprintf(stderr, "%s: %lu findings, %s expected:\n", negatives[i],
					(unsigned long)findings_count(findings), requirement);
			for (f = findings; f; f = f->next) {
				fprintf(stderr, "  %s %s %s\n", f->requirement, f->element_id ? f->element_id : "-",
						f->message);
			}
			TEST_ASSERT(0);
		}
		TEST_ASSERT(f->message && *f->message);
		TEST_ASSERT(f->foreign == expected_foreign(negatives[i]));
		cyberiada_destroy_fa_findings(findings);
		TEST_ASSERT(cyberiada_write_sm_document(doc, DRAFT, cybxmlCyberiada10,
												CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_write_sm_document(doc, DRAFT, cybxmlCyberiada10,
												CYBERIADA_FLAG_STRICT) == CYBERIADA_FA_ERROR);
		cyberiada_destroy_sm_document(doc);
	}

	/* the same fault without type/ fa is an ordinary document */
	TEST_ASSERT(read_document("not-fa-guard", CYBERIADA_FLAG_NO, &is_fa) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(!is_fa);
	TEST_ASSERT(read_document("not-fa-guard", CYBERIADA_FLAG_STRICT, NULL) == CYBERIADA_NO_ERROR);
	doc = open_document("not-fa-guard", CYBERIADA_FLAG_NO);
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_check_fa_document(doc, &findings) == CYBERIADA_NO_ERROR);
	TEST_ASSERT(findings == NULL);
	TEST_ASSERT(cyberiada_check_fa_document(doc, NULL) == CYBERIADA_BAD_PARAMETER);
	TEST_ASSERT(cyberiada_check_fa_document(NULL, &findings) == CYBERIADA_BAD_PARAMETER);
	cyberiada_destroy_sm_document(doc);

	/* a draft lists every violation in the order of the rules; the membership is not
	   checked against a broken alphabet (z/) */
	doc = open_document("draft-many-faults", CYBERIADA_FLAG_NO);
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_check_fa_document(doc, &findings) == CYBERIADA_FA_ERROR);
	TEST_ASSERT(findings_count(findings) == 4);
	TEST_ASSERT(strcmp(findings->requirement, "CGML-FA-4-2") == 0);
	TEST_ASSERT(strcmp(findings->element_id, "alphabet") == 0 && !findings->foreign);
	TEST_ASSERT(strcmp(findings->next->requirement, "CGML-FA-3-1") == 0);
	TEST_ASSERT(strcmp(findings->next->element_id, "c1") == 0 && findings->next->foreign);
	f = findings_find(findings, "CGML-FA-3.4-1");
	TEST_ASSERT(f && strcmp(f->element_id, "q1-q1") == 0 && f->foreign);
	f = findings_find(findings, "CGML-FA-3.1.1-3");
	TEST_ASSERT(f && strcmp(f->element_id, "q0-q1-again") == 0 && !f->foreign);
	TEST_ASSERT(!findings_find(findings, "CGML-FA-4-3"));
	cyberiada_destroy_fa_findings(findings);
	cyberiada_destroy_sm_document(doc);

	/* the written automata read back strictly */
	for (i = 0; i < sizeof(roundtrips) / sizeof(const char*); i++) {
		char path[256];
		snprintf(path, sizeof(path), "diagrams/fa/%s.graphml", roundtrips[i]);
		doc = cyberiada_new_sm_document();
		TEST_ASSERT(doc);
		TEST_ASSERT(cyberiada_read_sm_document(doc, path, cybxmlCyberiada10,
											   CYBERIADA_FLAG_NO) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_write_sm_document(doc, ROUNDTRIP, cybxmlCyberiada10,
												CYBERIADA_FLAG_STRICT) == CYBERIADA_NO_ERROR);
		cyberiada_destroy_sm_document(doc);
		doc = cyberiada_new_sm_document();
		TEST_ASSERT(doc);
		TEST_ASSERT(cyberiada_read_sm_document(doc, ROUNDTRIP, cybxmlCyberiada10,
											   CYBERIADA_FLAG_STRICT) == CYBERIADA_NO_ERROR);
		TEST_ASSERT(cyberiada_document_is_fa(doc));
		cyberiada_destroy_sm_document(doc);
	}

	TEST_ASSERT(strcmp(cyberiada_error_str(CYBERIADA_FA_ERROR), "Unknown error code") != 0);
	return 0;
}
