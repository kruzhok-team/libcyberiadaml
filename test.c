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
	"cyberiada",    /* cybxmlCyberiada10 */
	"yed"           /* cybxmlYED */
};

const char* format_descr[] = {
	"Cyberiada-GraphML 1.0 format",
	"yEd editor format used by Ostranna projects and the Orbita Simulator"
};

size_t format_count = sizeof(formats) / sizeof(char*);

const char* commands[] = {
	"print",
	"convert"
};

const char* command_descr[] = {
	"print HSM diagram content to stdout; use -f key to set the scheme format (default - unknown)",
	"convert HSM from <from-format> to <to-format> into the file named <output-file>"
};

	
size_t command_count = sizeof(commands) / sizeof(char*);
	
static void print_usage(const char* name)
{
	unsigned int i;
	fprintf(stderr, "%s <command> [-f <from-format>] [-t <to-format> -o <dest-file>] <path-to-graphml-file>\n\n", name);
	fprintf(stderr, "Supported commands:\n");
	for (i = 0; i < command_count; i++) {
		fprintf(stderr, "  %-20s %s\n", commands[i], command_descr[i]);
	}
	fprintf(stderr, "Supported formats:\n");
	for (i = 0; i < format_count; i++) {
		fprintf(stderr, "  %-20s %s\n", formats[i], format_descr[i]);
	}
	fprintf(stderr, "\n");
}

int main(int argc, char** argv)
{
    char *source_filename, *dest_filename;
	char *source_format_str = "", *dest_format_str = "";
	CyberiadaXMLFormat source_format = cybxmlUnknown, dest_format = cybxmlUnknown;
	unsigned int i;
	int res;
	CyberiadaDocument doc;
	char print = 0;

	if (argc < 3) {
		print_usage(argv[0]);
		return 1;
	}

	if (argc == 5 || argc == 9) {
		if (strcmp(argv[2], "-f") != 0) {
			print_usage(argv[0]);
			return 1;
		}
		for(i = 0; i < format_count; i++) {
			if (strcmp(argv[3], formats[i]) == 0) {
				source_format = (CyberiadaXMLFormat)i;
				source_format_str = argv[3];
				break;
			}
		}
		if(strlen(source_format_str) == 0) {
			fprintf(stderr, "unsupported graphml format %s\n", argv[3]);
			print_usage(argv[0]);
			return 2;
		}
	}
	
	if (strcmp(argv[1], commands[0]) == 0) {
		/* print */

		if (argc == 5) {
			source_filename = argv[4];
		} else if (argc == 3) {
			source_filename = argv[2];
		} else {
			print_usage(argv[0]);
			return 1;
		}

		print = 1;
		
	} else if (strcmp(argv[1], commands[1]) == 0) {
		/* convert */

		if (argc < 7) {
			print_usage(argv[0]);
			return 1;
		}

		if (argc == 9) {
			if (strcmp(argv[4], "-t") != 0) {
				print_usage(argv[0]);
				return 1;
			}
			for(i = 0; i < format_count; i++) {
				if (strcmp(argv[5], formats[i]) == 0) {
					dest_format = (CyberiadaXMLFormat)i;
					dest_format_str = argv[5];
					break;
				}
			}
			if(strlen(dest_format_str) == 0) {
				fprintf(stderr, "unsupported graphml format %s\n", argv[5]);
				print_usage(argv[0]);
				return 2;
			}
			if (strcmp(argv[6], "-o") != 0) {
				print_usage(argv[0]);
				return 1;
			}
			dest_filename = argv[7];
			source_filename = argv[8];
		} else if (argc == 7) {
			if (strcmp(argv[2], "-t") != 0) {
				print_usage(argv[0]);
				return 1;
			}
			for(i = 0; i < format_count; i++) {
				if (strcmp(argv[3], formats[i]) == 0) {
					dest_format = (CyberiadaXMLFormat)i;
					dest_format_str = argv[3];
					break;
				}
			}
			if(strlen(dest_format_str) == 0) {
				fprintf(stderr, "unsupported graphml format %s\n", argv[3]);
				print_usage(argv[0]);
				return 2;
			}
			if (strcmp(argv[4], "-o") != 0) {
				print_usage(argv[0]);
				return 1;
			}
			dest_filename = argv[5];
			source_filename = argv[6];
		} else {
			print_usage(argv[0]);
			return 1;
		}
	}

	if ((res = cyberiada_read_sm_document(&doc, source_filename, source_format)) != CYBERIADA_NO_ERROR) {
		fprintf(stderr, "error while reading %s file: %d\n",
				source_filename, res);
		return 2;
	}

	if (print) {
		cyberiada_print_sm_document(&doc);
	} else {
		if ((res = cyberiada_write_sm_document(&doc, dest_filename, dest_format)) != CYBERIADA_NO_ERROR) {
			fprintf(stderr, "error while writing %s file: %d\n",
					dest_filename, res);
			return 3;
		}
	}

	cyberiada_cleanup_sm_document(&doc);
	
	return 0;
}
