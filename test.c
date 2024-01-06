/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The testing program
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

#include "cyberiadaml.h"

const char* formats[] = {
	"yed",			/* cybxmlYED */
	"cyberiada"		/* cybxmlCyberiada */
};

const char* format_names[] = {
	"yEd editor format used by Ostranna projects and the Orbita Simulator",
	"Cyberiada-GraphML format"
};

unsigned int format_count = sizeof(formats) / sizeof(char*);

static void print_usage(const char* name)
{
	unsigned int i;
	fprintf(stderr, "%s [-t <format>] <path-to-graphml-file>\n\n", name);
	fprintf(stderr, "Supported formats:\n");
	for (i = 0; i < format_count; i++) {
		fprintf(stderr, "  %-20s %s\n", formats[i], format_names[i]);
	}
	fprintf(stderr, "\n");
}

int main(int argc, char** argv)
{
    char *filename;
	char *format_str = "";
	CyberiadaXMLFormat format = cybxmlUnknown;
	unsigned int i;
	int res;
	CyberiadaSM sm;

	if (argc == 4) {
		if (strcmp(argv[1], "-t") != 0) {
			print_usage(argv[0]);
			return 1;
		}
		for(i = 0; i < format_count; i++) {
			if (strcmp(argv[2], formats[i]) == 0) {
				format = (CyberiadaXMLFormat)i;
				format_str = argv[2];
				break;
			}
		}
		if(strlen(format_str) == 0) {
			fprintf(stderr, "unsupported graphml format %s\n", argv[2]);
			print_usage(argv[0]);
			return 2;
		}
		filename = argv[3];
	} else if (argc == 2) {
		filename = argv[1];
	} else {
		print_usage(argv[0]);
		return 1;
	}
	if ((res = cyberiada_read_sm(&sm, filename, format)) != CYBERIADA_NO_ERROR) {
		fprintf(stderr, "error while reading %s file: %d\n",
				filename, res);
		return 2;
	}

	cyberiada_print_sm(&sm);

	cyberiada_cleanup_sm(&sm);
	
	return 0;
}
