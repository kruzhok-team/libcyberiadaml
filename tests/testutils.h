/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The testing utilities
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

#ifndef __CYBERIADA_TESTUTILS_H
#define __CYBERIADA_TESTUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(x)                                                  \
	do {                                                                \
		if (!(x)) {                                                     \
			fprintf(stderr, "assertion failed at %s:%d: %s\n",          \
					__FILE__, __LINE__, #x);                            \
			exit(1);                                                    \
		}                                                               \
	} while (0)

/* redirect stdout to the given file to capture the test output */
static inline void test_capture_stdout(const char* path)
{
	if (!freopen(path, "w", stdout)) {
		fprintf(stderr, "cannot redirect stdout to %s\n", path);
		exit(1);
	}
}

/* byte-wise file comparison, returns 0 if the files are equal */
static inline int test_compare_files(const char* path1, const char* path2)
{
	FILE *f1, *f2;
	int c1, c2, result = 0;
	f1 = fopen(path1, "rb");
	if (!f1) {
		fprintf(stderr, "cannot open %s\n", path1);
		return 1;
	}
	f2 = fopen(path2, "rb");
	if (!f2) {
		fprintf(stderr, "cannot open %s\n", path2);
		fclose(f1);
		return 1;
	}
	do {
		c1 = fgetc(f1);
		c2 = fgetc(f2);
		if (c1 != c2) {
			result = 1;
			break;
		}
	} while (c1 != EOF);
	fclose(f1);
	fclose(f2);
	return result;
}

/* finish the stdout capture and compare it with the golden file */
static inline int test_check_golden(const char* out_path, const char* golden_path)
{
	fflush(stdout);
	if (test_compare_files(out_path, golden_path)) {
		fprintf(stderr, "output %s differs from golden %s\n",
				out_path, golden_path);
		return 1;
	}
	return 0;
}

#endif
