/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The UTF-8 encoder test
 *
 * Copyright (C) 2024-2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include "testutils.h"
#include "utf8enc.h"

int main(void)
{
	const char* ascii = "Hello, world!";
	const char* utf8 = "Hello! Съешь еще этих мягких французских булок и выпей чаю";
	char *encoded, *decoded;
	size_t encoded_len, decoded_len, i;

	encoded = utf8_encode(ascii, strlen(ascii), &encoded_len);
	TEST_ASSERT(encoded);
	decoded = utf8_decode(encoded, strlen(encoded), &decoded_len);
	TEST_ASSERT(decoded);
	TEST_ASSERT(strcmp(ascii, decoded) == 0);
	free(encoded);
	free(decoded);

	encoded = utf8_encode(utf8, strlen(utf8), &encoded_len);
	TEST_ASSERT(encoded);
	TEST_ASSERT(encoded_len == strlen(encoded));
	for (i = 0; i < encoded_len; i++) {
		TEST_ASSERT((unsigned char)encoded[i] < 128);
	}
	decoded = utf8_decode(encoded, strlen(encoded), &decoded_len);
	TEST_ASSERT(decoded);
	TEST_ASSERT(decoded_len == strlen(utf8));
	TEST_ASSERT(strcmp(utf8, decoded) == 0);
	free(encoded);
	free(decoded);

	return 0;
}
