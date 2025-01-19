/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The command line GraphML parser program
 *
 * Copyright (C) 2024-25 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include "cyberiadaml.h"

#define CMD_PRINT                  1
#define CMD_CONVERT                2
#define CMD_DIFF                   3

#define CMD_PARAM_INDEX_FROM_TYPE  0
#define CMD_PARAM_INDEX_TO_TYPE    1
#define CMD_PARAM_INDEX_GRAPH      2
#define CMD_PARAM_INDEX_GRAPH2     3
#define CMD_PARAM_INDEX_SILENT     4

#define CMD_PARAMETER_FROM_TYPE    1
#define CMD_PARAMETER_TO_TYPE      2
#define CMD_PARAMETER_GRAPH        4
#define CMD_PARAMETER_GRAPH2       8
#define CMD_PARAMETER_SILENT       16

const char* formats[] = {
	"cyberiada",    /* cybxmlCyberiada10 */
	"yed"           /* cybxmlYED */
};

const char* format_descr[] = {
	"Cyberiada-GraphML 1.0 format",
	"yEd editor format used by Ostranna projects and the Orbita Simulator"
};

size_t format_count = sizeof(formats) / sizeof(char*);

typedef enum {
	argNone = 0,
	argFile,
	argFormat
} CommandArgumentType;

typedef struct {
	int                 code;
	const char*         short_name;
	const char*         long_name;
	CommandArgumentType argument;
	const char*         descr;
	int                 present;
	const char*         arg_value;
	CyberiadaXMLFormat  arg_format;
} CyberiadaCommandParameters;

CyberiadaCommandParameters parameters[] = {
	{CMD_PARAMETER_FROM_TYPE, "-f",  "--file-format",   argFormat, "source graph format (see below)", 0, NULL, cybxmlUnknown},
	{CMD_PARAMETER_TO_TYPE,   "-t",  "--output-format", argFormat, "target/compared graph format (see below)", 0, NULL, cybxmlUnknown},
	{CMD_PARAMETER_GRAPH,     "-g",  "--graph",         argFile,   "path to the source graph file", 0, NULL, -1},
	{CMD_PARAMETER_GRAPH2,    "-o",  "--output-graph",  argFile,   "path to the target/compared graph file", 0, NULL, -1},
	{CMD_PARAMETER_SILENT,    "-s",  "--silent",        argNone,   "do not print information to stdout", 0, NULL, -1},
};

size_t parameters_count = sizeof(parameters) / sizeof(CyberiadaCommandParameters);

typedef struct {
	int         code;
	const char* name;
	int         single_parameter;
	int         required_parameters;
	int         optional_parameters;
	const char* descr;
} CyberiadaCommand;

CyberiadaCommand commands[] = {
	{CMD_PRINT,   "print", CMD_PARAMETER_GRAPH, CMD_PARAMETER_GRAPH, CMD_PARAMETER_FROM_TYPE | CMD_PARAMETER_SILENT,
	 "read the HSM diagram and print its content to stdout; use -f key to set the graph format (default - unknown)"},
	{CMD_CONVERT, "convert", 0, CMD_PARAMETER_GRAPH | CMD_PARAMETER_GRAPH2, CMD_PARAMETER_FROM_TYPE | CMD_PARAMETER_TO_TYPE | CMD_PARAMETER_SILENT,
	 "convert HSM from -f <from-format> to -t <output-format> into the file named -o <output-graph>"},
	{CMD_DIFF,    "diff", 0, CMD_PARAMETER_GRAPH | CMD_PARAMETER_GRAPH2, CMD_PARAMETER_FROM_TYPE | CMD_PARAMETER_TO_TYPE | CMD_PARAMETER_SILENT,
	 "compare HSMs from <graph> and <output-graph> and print the difference"}
};

size_t commands_count = sizeof(commands) / sizeof(CyberiadaCommand);
	
static void print_usage(const char* name)
{
	size_t i;
	fprintf(stderr, "%s <command> <command-parameters>\n", name);
#ifdef __DEBUG__
	fprintf(stderr, "Debug verision\n");
#endif
	fprintf(stderr, "\nSupported commands:\n");
	for (i = 0; i < commands_count; i++) {
		fprintf(stderr, "  %-20s %s\n", commands[i].name, commands[i].descr);
	}
	fprintf(stderr, "\nSupported parameters:\n");
	for (i = 0; i < parameters_count; i++) {
		fprintf(stderr, "  %-3s %-15s %s\n", parameters[i].short_name, parameters[i].long_name, parameters[i].descr);
	}
	fprintf(stderr, "\nSupported formats:\n");
	for (i = 0; i < format_count; i++) {
		fprintf(stderr, "  %-20s %s\n", formats[i], format_descr[i]);
	}
	fprintf(stderr, "\n");
}

int parse_arguments(int argc, char** argv)
{
	CyberiadaCommand* cmd = NULL;
	int a;
	size_t i, j;
	
	for (i = 0; i < commands_count; i++) {
		if (strcmp(argv[1], commands[i].name) == 0) {
			cmd = commands + i;
			break;
		}
	}
	if (!cmd) {
		return 0;
	}

	/* the only parameter w/o key */
	if (argc == 3 && cmd->single_parameter) {
		for (i = 0; i < parameters_count; i++) {
			if (parameters[i].code == cmd->single_parameter) {
				parameters[i].present = 1;
				parameters[i].arg_value = argv[2];
				return cmd->code;
			}
		}
	} else {
		for (a = 2; a < argc; a++) {
			const char* arg_str = argv[a];
			for (i = 0; i < parameters_count; i++) {
				if (strcmp(arg_str, parameters[i].short_name) == 0 ||
					strcmp(arg_str, parameters[i].long_name) == 0) {
					
					parameters[i].present = 1;
					if (parameters[i].argument != argNone) {
						if (a + 1 == argc) {
							fprintf(stderr, "Argument required!\n");
							return 0;
						}
						parameters[i].arg_value = argv[a + 1];
						a++;
						if (parameters[i].argument == argFormat) {
							int found = 0;
							for(j = 0; j < format_count; j++) {
								if (strcmp(parameters[i].arg_value, formats[j]) == 0) {
									found = 1;
									parameters[i].arg_format = (CyberiadaXMLFormat)j; 
									break;
								}
							}
							if (!found) {
								fprintf(stderr, "Wrong graphml format specified: %s\n\n", parameters[i].arg_value);
								return 0;
							}
						}
					}
					break;
				}
			}
		}
	}
	
	return cmd->code;
}

int main(int argc, char** argv)
{
	int command = 0;
    const char *source_filename, *dest_filename;
	int silent = 0;
	CyberiadaXMLFormat source_format, dest_format;
	CyberiadaDocument doc;
	size_t i;
	
	int res = CYBERIADA_NO_ERROR;

	if (argc < 3) {
		print_usage(argv[0]);
		return 1;
	}

	command = parse_arguments(argc, argv);
	if (!command) {
		print_usage(argv[0]);
		return 1;
	}
    source_filename = parameters[CMD_PARAM_INDEX_GRAPH].arg_value;
    dest_filename = parameters[CMD_PARAM_INDEX_GRAPH2].arg_value;
	source_format = parameters[CMD_PARAM_INDEX_FROM_TYPE].arg_format;
	dest_format = parameters[CMD_PARAM_INDEX_TO_TYPE].arg_format;
	silent = parameters[CMD_PARAM_INDEX_SILENT].present;

	cyberiada_init_sm_document(&doc);
	
	if ((res = cyberiada_read_sm_document(&doc, source_filename, source_format, CYBERIADA_FLAG_NO)) != CYBERIADA_NO_ERROR) {
		fprintf(stderr, "Error while reading %s file: %d\n",
				source_filename, res);
		cyberiada_cleanup_sm_document(&doc);
		return 2;
	}

	if (command == CMD_PRINT && !silent) {
		cyberiada_print_sm_document(&doc);
	} else if (command == CMD_CONVERT) {
		if ((res = cyberiada_write_sm_document(&doc, dest_filename, dest_format, CYBERIADA_FLAG_NO)) != CYBERIADA_NO_ERROR) {
			fprintf(stderr, "Error while writing %s file: %d\n",
					dest_filename, res);
			return 3;
		}
	} else if (command == CMD_DIFF) {
		CyberiadaDocument doc2;
		int result_flags;
		size_t sm_diff_nodes_size = 0, sm2_new_nodes_size = 0, sm1_missing_nodes_size = 0,
			sm_diff_edges_size = 0, sm2_new_edges_size = 0, sm1_missing_edges_size = 0;
		CyberiadaNode *new_initial = NULL, **sm_diff_nodes = NULL, **sm1_missing_nodes = NULL, **sm2_new_nodes = NULL;
		CyberiadaEdge **sm_diff_edges, **sm2_new_edges = NULL, **sm1_missing_edges = NULL;
		size_t *sm_diff_nodes_flags = NULL, *sm_diff_edges_flags = NULL;
		
		if (!doc.state_machines || doc.state_machines->next) {
			fprintf(stderr, "The graph %s should contain a single state machine\n", source_filename);

			cyberiada_cleanup_sm_document(&doc);

			return 4;
		}
		
		cyberiada_init_sm_document(&doc2);
		
		if ((res = cyberiada_read_sm_document(&doc2, dest_filename, dest_format, CYBERIADA_FLAG_NO)) != CYBERIADA_NO_ERROR) {
			fprintf(stderr, "Error while reading %s file: %d\n",
					dest_filename, res);

			cyberiada_cleanup_sm_document(&doc);
			cyberiada_cleanup_sm_document(&doc2);

			return 5;
		}

		if (!doc2.state_machines || doc2.state_machines->next) {
			fprintf(stderr, "The graph %s should contain a single state machine\n", dest_filename);

			cyberiada_cleanup_sm_document(&doc);
			cyberiada_cleanup_sm_document(&doc2);

			return 6;
		}

		/* ignore comments and do not require the initial state on the top level */
		res = cyberiada_check_isomorphism(doc.state_machines, doc2.state_machines, 1, 0,
										  &result_flags, &new_initial,
										  &sm_diff_nodes_size, &sm_diff_nodes, &sm_diff_nodes_flags,
										  &sm2_new_nodes_size, &sm2_new_nodes,
										  &sm1_missing_nodes_size, &sm1_missing_nodes,
										  &sm_diff_edges_size, &sm_diff_edges, &sm_diff_edges_flags,
										  &sm2_new_edges_size, &sm2_new_edges,
										  &sm1_missing_edges_size, &sm1_missing_edges);

		if (res == CYBERIADA_NO_ERROR) {
			if (!silent) {
				printf("Graph comparison result: ");
				if (result_flags == CYBERIADA_ISOMORPH_FLAG_IDENTICAL) {
					printf("the SM graphs are identical");
				} else if (result_flags == CYBERIADA_ISOMORPH_FLAG_EQUAL) {
					printf("the SM graphs are equal");
				} else if (result_flags == CYBERIADA_ISOMORPH_FLAG_ISOMORPHIC) {
					printf("the SM graphs are isomorphic");			
				} else {
					printf("the SM graphs are not isomorphic - ");
					if (result_flags & CYBERIADA_ISOMORPH_FLAG_DIFF_STATES) {
						printf("have different states, ");
					}
					if (result_flags & CYBERIADA_ISOMORPH_FLAG_DIFF_INITIAL) {
						printf("have different initial pseudostates, ");
					}
					if (result_flags & CYBERIADA_ISOMORPH_FLAG_DIFF_EDGES) {
						printf("have different edges");
					}
				}
				printf("\n");
				if (new_initial) {
					printf("'\nNew initial pseudostate: ");
					cyberiada_print_node(new_initial, 0);
				}
				if (sm_diff_nodes_size > 0) {
					printf("\nThere are %lu different nodes in the second graph:\n", sm_diff_nodes_size);
					if (sm_diff_nodes_flags) {
						for (i = 0; i < sm_diff_nodes_size; i++) {
							size_t flag = sm_diff_nodes_flags[i];
							printf(" %lu. ", i + 1);
							if (flag & CYBERIADA_NODE_DIFF_ID) {
								printf("id ");
							}
							if (flag & CYBERIADA_NODE_DIFF_TYPE) {
								printf("type ");
							}
							if (flag & CYBERIADA_NODE_DIFF_TITLE) {
								printf("title ");
							}
							if (flag & CYBERIADA_NODE_DIFF_ACTIONS) {
								printf("actions ");
							}
							if (flag & CYBERIADA_NODE_DIFF_SM_LINK) {
								printf("SM-links ");
							}
							if (flag & CYBERIADA_NODE_DIFF_CHILDREN) {
								printf("children ");
							}
							if (flag & CYBERIADA_NODE_DIFF_EDGES) {
								printf("edges ");
							}
							printf("\n");
						}
					}
					if (sm_diff_nodes) {
						printf("\n The different nodes (version from the second graph):\n");
						for (i = 0; i < sm_diff_nodes_size; i++) {
							printf(" %lu:\n", i + 1);
							cyberiada_print_node(sm_diff_nodes[i], 1);
						}
					}
				}
				if (sm2_new_nodes_size > 0 && sm2_new_nodes) {
					printf("\nThe new nodes added in the second graph:\n");
					for (i = 0; i < sm2_new_nodes_size; i++) {
						cyberiada_print_node(sm2_new_nodes[i], 0);
					}
				}
				if (sm1_missing_nodes_size > 0 && sm1_missing_nodes) {
					printf("\nThe nodes missing in the first graph:\n");
					for (i = 0; i < sm1_missing_nodes_size; i++) {
						cyberiada_print_node(sm1_missing_nodes[i], 0);
					}
				}
				if (sm_diff_edges_size > 0) {
					printf("\nThere are %lu different edges in the second graph:\n", sm_diff_edges_size);
					if (sm_diff_edges_flags) {
						for (i = 0; i < sm_diff_edges_size; i++) {
							size_t flag = sm_diff_edges_flags[i];
							printf(" %lu. ", i + 1);
							if (flag & CYBERIADA_EDGE_DIFF_ID) {
								printf("id ");
							}
							if (flag & CYBERIADA_EDGE_DIFF_ACTION) {
								printf("action ");
							}
							printf("\n");
						}
					}
					if (sm_diff_edges) {
						printf("\n The different edges (version from the second graph):\n");
						for (i = 0; i < sm_diff_edges_size; i++) {
							printf(" %lu: ", i + 1);
							cyberiada_print_edge(sm_diff_edges[i]);
						}
					}
				}
				if (sm2_new_edges_size > 0 && sm2_new_edges) {
					printf("\nThe new edges added in the second graph:\n");
					for (i = 0; i < sm2_new_edges_size; i++) {
						cyberiada_print_edge(sm2_new_edges[i]);
					}
				}
				if (sm1_missing_edges_size > 0 && sm1_missing_edges) {
					printf("\nThe edges missing in the first graph:\n");
					for (i = 0; i < sm1_missing_edges_size; i++) {
						cyberiada_print_edge(sm1_missing_edges[i]);
					}
				}
			}
		} else {
			fprintf(stderr, "Error while comparing graphs: %d\n", res);			
		}

		if (sm_diff_nodes) free(sm_diff_nodes);
		if (sm_diff_nodes_flags) free(sm_diff_nodes_flags);
		if (sm1_missing_nodes) free(sm1_missing_nodes);
		if (sm2_new_nodes) free(sm2_new_nodes);
		if (sm_diff_edges) free(sm_diff_edges);
		if (sm_diff_edges_flags) free(sm_diff_edges_flags);
		if (sm2_new_edges) free(sm2_new_edges);
		if (sm1_missing_edges) free(sm1_missing_edges);

		cyberiada_cleanup_sm_document(&doc2);
	}

	cyberiada_cleanup_sm_document(&doc);

	if (res == CYBERIADA_NO_ERROR) {
		return 0;
	} else {
		return 7;
	}
}
