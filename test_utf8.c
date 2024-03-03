/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The UTF-8 encoder testing program
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
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
#include <string.h>
#include <stdlib.h>
#include "utf8enc.h"

int main(void)
{
	const char* a = "Hello! Съешь еще этих мягких французских булок и выпей чаю";
	char *b, *c;
	size_t b_l, c_l;
	printf("String len %ld\n", strlen(a));
	b = utf8_encode(a, strlen(a), &b_l);
	if (!b) {
		printf("String encoding error\n");
		return 1;
	}
	printf("Encoded len %ld %ld\n", strlen(b), b_l);
	c = utf8_decode(b, strlen(b), &c_l);
	if (!c) {
		printf("String decoding error\n");
		free(b);
		return 1;
	}
	printf("Decoded len %ld %ld\n", strlen(c), c_l);
	printf("Orig: %s\n", a);
	printf("Enc:  %s\n", b);
	printf("Dec:  %s\n", c);
	if (strcmp(a, c) != 0) {
		printf("Strings don't match\n");
	}
	free(b);
	free(c);
}
