/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * UTF-8 string encoding functions
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include "utf8enc.h"

static char encode_digit(int num)
{
	if (num < 10) {
		return (char)('0' + num);
	} else {
		return (char)('A' + num - 10);
	}
}

static void encode_char(unsigned char c, char* buffer)
{
	buffer[0] = '_';
	buffer[1] = '_';
	buffer[2] = 'x';
	buffer[3] = '_';
	buffer[4] = encode_digit((c >> 4) & 0xf);
	buffer[5] = encode_digit(c & 0xf);
}

static int decode_char(int c)
{
	if (c >= '0' && c <= '9') {
		c = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		c = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		c = c - 'A' + 10;
	}
	return c;
}

static char decode_number(const char* buffer, size_t buffer_len)
{
	int c1, c2;
	if (buffer_len < 6) {
		return 0;
	}
	c1 = buffer[4];
	c2 = buffer[5];
	return (char)((decode_char(c1) << 4) | (decode_char(c2) & 0xF));
}

char* utf8_encode(const char *data, size_t input_len, size_t *output_len)
{
	size_t i, o;
	char *encoded_data = NULL;
	size_t output;

	if (!data || !input_len || !output_len) {
		return NULL;
	}
	
	output = 0;
	i = 0;
	while (i < input_len) {
		unsigned char c = (unsigned char)data[i];
		if (c < 128) {
			output++;
		} else {
			output += 6;
		}
		i++;
	}
	
	encoded_data = (char*)malloc(output + 1);
	if (!encoded_data) return NULL;
	memset(encoded_data, 0, output + 1);
	
	i = o = 0;
	while (i < input_len) {
		unsigned char c = (unsigned char)data[i];
		if (c < 128) {
			encoded_data[o] = (char)c;
			o++;
		} else {
			encode_char(c, encoded_data + o);
			o += 6;
		}
		i++;
	}

	*output_len = output;
	return encoded_data;
}

char* utf8_decode(const char *data, size_t input_len, size_t *output_len)
{
	size_t i, o;
	char *decoded_data = NULL;
	size_t output;

	if (!data || !input_len || !output_len) {
		return NULL;
	}
	
	output = 0;
	i = 0;
	while (i < input_len) {
		int c = data[i];
		if (c == '_' && i + 6 <= input_len &&
			data[i + 1] == '_' &&
			data[i + 2] == 'x' &&
			data[i + 3] == '_') {
			i += 6;
		} else {
			i++;
		}
		output++;
	}

	decoded_data = (char*)malloc(output + 1);
	if (!decoded_data) return NULL;
	memset(decoded_data, 0, output + 1);

	i = o = 0;
	while (i < input_len) {
		unsigned char c = (unsigned char)data[i];
		if (c == '_' && i + 6 <= input_len && data[i + 1] == '_' && data[i + 2] == 'x' && data[i + 3] == '_') {
			decoded_data[o] = decode_number(data + i, input_len - i);
			i += 6;
		} else {
			decoded_data[o] = (char)c;
			i++;
		}
		o++;
	}
	
	decoded_data[output] = 0;
	*output_len = output;
	return decoded_data;
}
