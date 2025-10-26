/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Cyberiada GraphML actions handling
 *
 * Copyright (C) 2024-2025 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "cyb_actions.h"
#include "cyb_error.h"
#include "cyb_string.h"
#include "cyb_structs.h"
#include "utf8enc.h"

/* HSM action constants */

#define CYBERIADA_ACTION_TRIGGER_ENTRY         "entry"
#define CYBERIADA_ACTION_TRIGGER_EXIT          "exit"
#define CYBERIADA_ACTION_TRIGGER_DO            "do"
#define CYBERIADA_ACTION_SEPARATOR_CHR         '/'
#define CYBERIADA_ACTION_ENDING_CHR            ')'
#define CYBERIADA_ACTION_BRACKET_CHR           '('
#define CYBERIADA_ACTION_STRINGS_CHR           '\n'

#define CYBERIADA_ACTION_REGEXP_MATCHES        10
#define CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER  1
#define CYBERIADA_ACTION_REGEXP_MATCH_GUARD    6
#define CYBERIADA_ACTION_REGEXP_MATCH_PROP     7
#define CYBERIADA_ACTION_REGEXP_MATCH_ACTION   9
/*  #define CYBERIADA_ACTION_NL_REGEXP_MATCHES     4*/
#define CYBERIADA_ACTION_LEGACY_MATCHES        7
#define CYBERIADA_ACTION_LEGACY_MATCH_TRIGGER  1
#define CYBERIADA_ACTION_LEGACY_EDGE_MATCHES   9
#define CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_TRIGGER 1
#define CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_GUARD 6
#define CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_ACTION 8

typedef struct _CyberiadaRegexpsMisc {
	/* basic regexps */
	regex_t edge_action_regexp;
	regex_t node_action_regexp;
	regex_t node_legacy_action_regexp;
	regex_t edge_legacy_action_regexp;
	/*regex_t newline_regexp;*/
	regex_t spaces_regexp;
} CyberiadaRegexpsMics;

static int cyberiaga_matchres_action_regexps(const char* text,
											 const regmatch_t* pmatch, size_t pmatch_size,
											 char** trigger, char** guard, char** behavior,
											 size_t match_trigger, size_t match_guard, size_t match_action)
{
	size_t i;
	char* part;
	int start, end;
		
	if (pmatch_size > CYBERIADA_ACTION_REGEXP_MATCHES) {
		ERROR("bad action regexp match array size\n");
		return CYBERIADA_ASSERT;
	}

	for (i = 0; i < pmatch_size; i++) {
		if (i != match_trigger &&
			i != match_guard &&
			i != match_action) {
			continue;
		}
		start = pmatch[i].rm_so;
		end = pmatch[i].rm_eo;
		if (end > start && text[start] != 0) {
			part = (char*)malloc((size_t)(end - start + 1));
			strncpy(part, &text[start], (size_t)(end - start));
			part[(size_t)(end - start)] = 0;
		} else {
			part = "";
		}
		if (i == match_trigger) {
			*trigger = part;
		} else if (i == match_guard) {
			*guard = part;
		} else {
			/* i == ACTION_REGEXP_MATCH_ACTION */
			*behavior= part;
		}
	}

	return CYBERIADA_NO_ERROR;
}

/*static int cyberiaga_matchres_newline(const regmatch_t* pmatch, size_t pmatch_size,
									  size_t* next_block)
{
	if (pmatch_size != ACTION_NL_REGEXP_MATCHES) {
		ERROR("bad new line regexp match array size\n");
		return CYBERIADA_ASSERT;
	}
	if (next_block) {
		*next_block = (size_t)pmatch[pmatch_size - 1].rm_so;
	}

	return CYBERIADA_NO_ERROR;
	}*/

static int decode_utf8_strings(char** trigger, char** guard, char** behavior)
{
	char* oldptr;
	size_t len;
	if (*trigger && **trigger) {
		oldptr = *trigger;
		*trigger = utf8_decode(*trigger, strlen(*trigger), &len);
		free(oldptr);
	}	
	if (*guard && **guard) {
		oldptr = *guard;
		*guard = utf8_decode(*guard, strlen(*guard), &len);
		free(oldptr);
	}	
	if (*behavior && **behavior) {
		oldptr = *behavior;
		*behavior = utf8_decode(*behavior, strlen(*behavior), &len);
		free(oldptr);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_edge_action(const char* text, CyberiadaAction** action, CyberiadaRegexps* regexps)
{
	int res;
	size_t buffer_len;
	char *trigger = "", *guard = "", *behavior = "";
	char *buffer;
	regmatch_t pmatch[CYBERIADA_ACTION_REGEXP_MATCHES];

	buffer = utf8_encode(text, strlen(text), &buffer_len);

	if (!buffer) {
		*action = NULL;
		return CYBERIADA_NO_ERROR;
	}

	if (regexps->berloga_legacy) {
		if ((res = regexec(&(regexps->r->edge_legacy_action_regexp), buffer,
						   CYBERIADA_ACTION_LEGACY_EDGE_MATCHES, pmatch, 0)) != 0) {
			if (res == REG_NOMATCH) {
				ERROR("legacy edge action text didn't match the regexp\n");
				return CYBERIADA_ACTION_FORMAT_ERROR;
			} else {
				ERROR("legacy edge action regexp error %d\n", res);
				return CYBERIADA_ASSERT;
			}
		}
		if (cyberiaga_matchres_action_regexps(buffer,
											  pmatch, CYBERIADA_ACTION_LEGACY_EDGE_MATCHES,
											  &trigger, &guard, &behavior,
											  CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_TRIGGER,
											  CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_GUARD,
											  CYBERIADA_ACTION_REGEXP_MATCH_LEGACY_ACTION) != CYBERIADA_NO_ERROR) {
			return CYBERIADA_ASSERT;
		}		
	} else {
		if ((res = regexec(&(regexps->r->edge_action_regexp), buffer,
						   CYBERIADA_ACTION_REGEXP_MATCHES, pmatch, 0)) != 0) {
			if (res == REG_NOMATCH) {
				ERROR("edge action text didn't match the regexp\n");
				return CYBERIADA_ACTION_FORMAT_ERROR;
			} else {
				ERROR("edge action regexp error %d\n", res);
				return CYBERIADA_ASSERT;
			}
		}
		if (cyberiaga_matchres_action_regexps(buffer,
											  pmatch, CYBERIADA_ACTION_REGEXP_MATCHES,
											  &trigger, &guard, &behavior,
											  CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER,
											  CYBERIADA_ACTION_REGEXP_MATCH_GUARD,
											  CYBERIADA_ACTION_REGEXP_MATCH_ACTION) != CYBERIADA_NO_ERROR) {
			return CYBERIADA_ASSERT;
		}
	}

	decode_utf8_strings(&trigger, &guard, &behavior);
	
/*	DEBUG("\n");
	DEBUG("edge action:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("action: %s\n", action);*/

	if (*trigger || *guard || *behavior) {
		cyberiada_string_trim(trigger);
		cyberiada_string_trim(guard);
		cyberiada_string_trim(behavior);
		*action = cyberiada_new_action(cybActionTransition, trigger, guard, behavior);
	} else {
		*action = NULL;
	}
	
	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*behavior) free(behavior);

	free(buffer);
	return CYBERIADA_NO_ERROR;
}

int cyberiada_add_action(const char* trigger, const char* guard, const char* behavior,
						 CyberiadaAction** action)
{
	CyberiadaAction* new_action;
	CyberiadaActionType type;

	if (trigger) {
		if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_ENTRY) == 0) {
			type = cybActionEntry;
		} else if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_EXIT) == 0) {
			type = cybActionExit;
		} else if (strcmp(trigger, CYBERIADA_ACTION_TRIGGER_DO) == 0) {
			type = cybActionDo;
		} else {
			type = cybActionTransition;
		}
	}
	
	/*DEBUG("\n");
	DEBUG("node action:\n");
	DEBUG("trigger: %s\n", trigger);
	DEBUG("guard: %s\n", guard);
	DEBUG("behavior: %s\n", behavior);
	DEBUG("type: %d\n", type);*/

	new_action = cyberiada_new_action(type, trigger, guard, behavior);
	if (*action) {
		CyberiadaAction* b = *action;
		while (b->next) b = b->next;
		b->next = new_action;
	} else {
		*action = new_action;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_state_block_action(const char* text, CyberiadaAction** action, CyberiadaRegexps* regexps)
{
	int res;
	char *trigger = "", *guard = "", *behavior = "";
	regmatch_t pmatch[CYBERIADA_ACTION_REGEXP_MATCHES];
	if ((res = regexec(&(regexps->r->node_action_regexp), text,
					   CYBERIADA_ACTION_REGEXP_MATCHES, pmatch, 0)) != 0) {
		if (res == REG_NOMATCH) {
			ERROR("node block action text didn't match the regexp\n");
			return CYBERIADA_ACTION_FORMAT_ERROR;
		} else {
			ERROR("node block action regexp error %d\n", res);
			return CYBERIADA_ASSERT;
		}
	}
	if (cyberiaga_matchres_action_regexps(text,
										  pmatch, CYBERIADA_ACTION_REGEXP_MATCHES,
										  &trigger, &guard, &behavior,
										  CYBERIADA_ACTION_REGEXP_MATCH_TRIGGER,
										  CYBERIADA_ACTION_REGEXP_MATCH_GUARD,
										  CYBERIADA_ACTION_REGEXP_MATCH_ACTION) != CYBERIADA_NO_ERROR) {
		return CYBERIADA_ASSERT;
	}

	decode_utf8_strings(&trigger, &guard, &behavior);
	cyberiada_string_trim(trigger);
	cyberiada_string_trim(guard);
	cyberiada_string_trim(behavior);
	cyberiada_add_action(trigger, guard, behavior, action);

	if (*trigger) free(trigger);
	if (*guard) free(guard);
	if (*behavior) free(behavior);
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_state_actions(const char* text, CyberiadaAction** actions, CyberiadaRegexps* regexps)
{
	int res;
	size_t buffer_len;
	char *buffer, *start, *block, *block2, *next;
/*	regmatch_t pmatch[ACTION_NL_REGEXP_MATCHES];*/
	
	buffer = utf8_encode(text, strlen(text), &buffer_len);
	next = buffer;

	*actions = NULL;

	if (!buffer) {
		return CYBERIADA_NO_ERROR;
	}
	
	while (*next) {
		start = next;
		block = strstr(start, CYBERIADA_NEWLINE);
		block2 = strstr(start, CYBERIADA_NEWLINE_RN);
		if (block2 && ((block && (block > block2)) || !block)) {
			block = block2;
			*block2 = 0;
			next = block2 + 4;
		} else if (block) {
			*block = 0;
			next = block + 2;
		} else {
			block = start;
			next = start + strlen(block);
		}
/*		if ((res = regexec(&cyberiada_newline_regexp, start,
						   ACTION_NL_REGEXP_MATCHES, pmatch, 0)) != 0) {
			if (res != REG_NOMATCH) {
				ERROR("newline regexp error %d\n", res);
				return CYBERIADA_ASSERT;
			}
		}
		if (res == REG_NOMATCH) {
			block = start;
			start = start + strlen(block);
		} else {
			if (cyberiaga_matchres_newline(pmatch, ACTION_NL_REGEXP_MATCHES,
										   &next_block) != CYBERIADA_NO_ERROR) {
				return CYBERIADA_ASSERT;
			}
			block = start;
			start[next_block - 1] = 0;
			start = start + next_block;
			DEBUG("first part: '%s'\n", block);
			DEBUG("second part: '%s'\n", start);
			}*/
		
		if (regexec(&(regexps->r->spaces_regexp), start, 0, NULL, 0) == 0) {
			continue ;
		}

		if ((res = cyberiada_decode_state_block_action(start, actions, regexps)) != CYBERIADA_NO_ERROR) {
			ERROR("error while decoding state block %s: %d\n", start, res);
			return res;
		}
	}
	
	free(buffer);
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_state_actions_yed(const char* text, CyberiadaAction** actions, CyberiadaRegexps* regexps)
{
	int res;
	char *buffer, *next, *start, *block, *buffer2 = NULL;
	size_t buffer_len;
	CyberiadaList *sections_list = NULL, *list;
	regmatch_t pmatch[CYBERIADA_ACTION_LEGACY_MATCHES];
		
	buffer = utf8_encode(text, strlen(text), &buffer_len);
	next = buffer;
	*actions = NULL;

	if (!buffer) {
		return CYBERIADA_NO_ERROR;
	}
	if (regexps->flattened_regexps) {
		buffer2 = (char*)malloc(buffer_len * 2);
		memset(buffer2, 0, buffer_len * 2);
		char* start2 = buffer2;
		block = buffer2;
		
		start = next;
		while (*next) {
			size_t size = (size_t)(next - start + 1);
			char chr = *next;
			if (chr == CYBERIADA_ACTION_SEPARATOR_CHR || chr == CYBERIADA_ACTION_ENDING_CHR) {
				if (chr == CYBERIADA_ACTION_SEPARATOR_CHR && block != buffer2) {
					*block = 0;
					block++;
					cyberiada_list_add(&sections_list, start2, NULL);
					start2 = block;
				}
				memcpy(block, start, size);
				block += size;
				*block = CYBERIADA_ACTION_STRINGS_CHR;
				block++;
				start = next + 1;
			}
			next++;
		}

		if (block != buffer2) {
			*block = 0;
			block++;
			cyberiada_list_add(&sections_list, start2, NULL);
		}
		
	} else {
		while (*next) {
			start = next;
			while (*start && isspace(*start)) start++;
			if (!*start) {
				res = CYBERIADA_NO_ERROR;
				break;
			}
			if (regexps->berloga_legacy > 1) {
				res = regexec(&(regexps->r->node_legacy_action_regexp), start,
							  CYBERIADA_ACTION_LEGACY_MATCHES, pmatch, 0);
				if (res != 0 && res != REG_NOMATCH) {
					ERROR("newline regexp error %d\n", res);
					res = CYBERIADA_ACTION_FORMAT_ERROR;
					break;
				}
				if (res == 0) {
					list = sections_list;
					cyberiada_list_add(&sections_list, start, NULL);
					if (list) {
						block = (char*)list->key;
						while (*block) {
							if (block == start) {
								*(block - 1) = 0;
								break;
							}
							block++;
						}
					}
				}
				block = strstr(start, CYBERIADA_SINGLE_NEWLINE);
				if (block) {
					next = block + 1;
				} else {
					next = start + strlen(start);
				}
			} else {
				res = regexec(&(regexps->r->node_legacy_action_regexp), start,
							  CYBERIADA_ACTION_LEGACY_MATCHES, pmatch, 0);
				if (res == REG_NOMATCH) {
					ERROR("action regexp error: \"%s\"\n", start);
					res = CYBERIADA_ACTION_FORMAT_ERROR;
					break;
				}
				cyberiada_list_add(&sections_list, start, NULL);
				
				block = strstr(start, CYBERIADA_NEWLINE);
				if (block) {
					*block = 0;
					next = block + 2;
				} else {
					next = start + strlen(start);
				}
			}
			res = CYBERIADA_NO_ERROR;
		}
	}
	for (list = sections_list; list; list = list->next) {
		start = (char*)list->key;
		if ((res = cyberiada_decode_state_block_action(start, actions, regexps)) != CYBERIADA_NO_ERROR) {
			ERROR("error while decoding state block %s: %d\n", start, res);
			break;
		}
		res = CYBERIADA_NO_ERROR;
	}

	free(buffer);
	if (buffer2) free(buffer2);
	cyberiada_list_free(&sections_list);
	
	return res;
}

int cyberiada_print_action(CyberiadaAction* action, int level)
{
	char levelspace[16];
	int i;

	memset(levelspace, 0, sizeof(levelspace));
	for(i = 0; i < level; i++) {
		if (i == 14) break;
		levelspace[i] = ' ';
	}
	
	printf("%sActions:\n", levelspace);
	while (action) {
		printf("%s Action (type %d):\n", levelspace, action->type);
		if(action->trigger) {
			printf("%s  Trigger: \"%s\"\n", levelspace, action->trigger);
		}		
		if(action->guard) {
			printf("%s  Guard: \"%s\"\n", levelspace, action->guard);
		}
		if(action->behavior) {
			printf("%s  Behavior: \"%s\"\n", levelspace, action->behavior);
		}
		action = action->next;
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_check_action_doubles(CyberiadaAction* a)
{
	CyberiadaAction *entry_action = NULL, *exit_action = NULL;
	while (a) {
		if (a->type == cybActionEntry) {
			if (entry_action) {
				ERROR("Multiple entry actions\n");
				return CYBERIADA_FORMAT_ERROR;
			} else {
				entry_action = a;
			}
		} else if (a->type == cybActionExit) {
			if (exit_action) {
				ERROR("Multiple exit actions\n");
				return CYBERIADA_FORMAT_ERROR;
			} else {
				exit_action = a;
			}			
		}
		a = a->next;
	}
	return CYBERIADA_NO_ERROR;
}

extern int cyberiada_destroy_action(CyberiadaAction* action);

int cyberiada_join_action_doubles(CyberiadaAction** action)
{
	CyberiadaAction *entry_action = NULL, *exit_action = NULL, *save_action;
	CyberiadaAction *a, *to_destroy, *prev = NULL;
		
	if (!action) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!*action) {
		/* skip trivial action */
		return CYBERIADA_NO_ERROR;
	}

	a = *action;
	
	while(a) {
		if (a->type == cybActionEntry || a->type == cybActionExit) {
			save_action = NULL;
			if (a->type == cybActionEntry) {
				if (!entry_action) {
					entry_action = a;
				} else {
					save_action = entry_action;
				}
			} else {
				if (!exit_action) {
					exit_action = a;
				} else {
					save_action = exit_action;
				}
			}
			if (save_action && a->behavior) {
				cyberiada_append_string(&(save_action->behavior),
										&(save_action->behavior_len),
										a->behavior,
										"\n");
				to_destroy = a;
				a = a->next;
				prev->next = a;
				to_destroy->next = NULL;
				cyberiada_destroy_action(to_destroy);
				continue;
			}
		}
		prev = a;
		a = a->next;
	}
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_remove_empty_actions(CyberiadaAction** action)
{
	CyberiadaAction *a, *prev = NULL;
		
	if (!action) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (!*action) {
		/* skip trivial action */
		return CYBERIADA_NO_ERROR;
	}

	a = *action;
	
	while(a) {
		if (!*a->behavior && !*a->guard) {
			CyberiadaAction* to_destroy = a;
			a = a->next;
			if (prev) {
				prev->next = a;
			} else {
				*action = a;
			}
			to_destroy->next = NULL;
			cyberiada_destroy_action(to_destroy);
		} else {
			prev = a;
			a = a->next;
		}
	}
	
	return CYBERIADA_NO_ERROR;
}

static int cyberiada_command_arguments_difference(const char* command1, const char* command2)
{
	int bracket = 0;
	/* DEBUG("Comparing commands with arguments %s and %s\n", command1, command2); */
	for (; *command1 && *command2; command1++, command2++) {
		if (*command1 == *command2) {
			if (!bracket && *command1 == CYBERIADA_ACTION_BRACKET_CHR) { 
				bracket = 1;
			}
		} else if (!bracket) {
			break;
		} else {
			return 1;
		}
	}
	return 0;
}

static int cyberiada_compare_action_behaviors(const char* behavior1, const char* behavior2, int* compare_flags)
{
	char *buffer1 = NULL, *buffer2 = NULL;
	char **commands1 = NULL, **commands2 = NULL;
	size_t i, j, commands1_size = 0, commands2_size = 0;
	char* c;
	
	cyberiada_copy_string(&buffer1, NULL, behavior1);
	commands1_size = 1;
	c = buffer1;
	while (*c) {
		if (*c == CYBERIADA_ACTION_STRINGS_CHR) commands1_size++; 
		c++;
	}
	cyberiada_copy_string(&buffer2, NULL, behavior2);
	commands2_size = 1;
	c = buffer2;
	while (*c) {
		if (*c == CYBERIADA_ACTION_STRINGS_CHR) commands2_size++; 
		c++;
	}

	if (commands1_size != commands2_size && compare_flags) {
		*compare_flags |= CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION;
	}
	
	commands1 = (char**)malloc(sizeof(char*) * commands1_size);
	commands1[0] = buffer1;
	for (i = 1, c = buffer1; *c && i < commands1_size; c++) { 
		if (*c == CYBERIADA_ACTION_STRINGS_CHR) {
			*c = 0;
			commands1[i++] = c + 1;
		}
	}

	commands2 = (char**)malloc(sizeof(char*) * commands2_size);
	commands2[0] = buffer2;
	for (i = 1, c = buffer2; *c && i < commands2_size; c++) { 
		if (*c == CYBERIADA_ACTION_STRINGS_CHR) {
			*c = 0;
			commands2[i++] = c + 1;
		}
	}

	for (i = 0; i < commands1_size; i++) {
		for (j = 0; j < commands2_size; j++) {
			if (!*(commands2[j])) continue;
			if (strcmp(commands1[i], commands2[j]) == 0) {
				if (i != j && compare_flags) {
					*compare_flags |= CYBERIADA_ACTION_DIFF_BEHAVIOR_ORDER;				
				}
				*(commands2[j]) = 0;
				break;
			} else {
				if (cyberiada_command_arguments_difference(commands1[i], commands2[j])) {
					if (compare_flags) {
						*compare_flags |= CYBERIADA_ACTION_DIFF_BEHAVIOR_ARG;
					}
					*(commands2[j]) = 0;
				}
			}
		}
	}

	for (j = 0; j < commands2_size; j++) {
		if (*commands2[j]) {
			*compare_flags |= CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION;
			break;
		}
	}
	
	if (buffer1) free(buffer1);
	if (buffer2) free(buffer2);
	if (commands1) free(commands1);
	if (commands2) free(commands2);
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_compare_node_actions(CyberiadaAction* n1action, CyberiadaAction* n2action, int* compare_flags)
{
	CyberiadaAction *a1, *a2;
	size_t n_a1 = 0, n_a2 = 0, n1_types = 0, n2_types = 0;

	if (!n1action && !n2action) {
		if (compare_flags) {
			*compare_flags = 0;
		}
		return CYBERIADA_NO_ERROR;
	}
	if ((n1action && !n2action) || (!n1action && n2action)) {
		if (compare_flags) {
			*compare_flags = CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION | CYBERIADA_ACTION_DIFF_TYPES | CYBERIADA_ACTION_DIFF_NUMBER;
		}
		return CYBERIADA_NO_ERROR;
	}

	for (a1 = n1action; a1; a1 = a1->next) {
		n_a1++;
		n1_types |= a1->type;
	}
	for (a2 = n2action; a2; a2 = a2->next) {
		n_a2++;
		n2_types |= a2->type;
	}
	
	if (n_a1 != n_a2 && compare_flags) {
		*compare_flags |= CYBERIADA_ACTION_DIFF_NUMBER;
	}

	if (n1_types != n2_types && compare_flags) {
		*compare_flags |= CYBERIADA_ACTION_DIFF_TYPES;
	}
	
	for (a1 = n1action; a1; a1 = a1->next) {
		int found = 0;
		for (a2 = n2action; a2; a2 = a2->next) {
			if (a1->type == a2->type &&
				(a2->type != cybActionTransition || strcmp(a1->trigger, a2->trigger) == 0)) {
				if (strcmp(a1->guard, a2->guard) == 0) {
					found = 1;
					if (strcmp(a1->behavior, a2->behavior) != 0) {
						/*DEBUG("Compare action behaviors %s and %s\n", a1->behavior, a2->behavior);*/
						cyberiada_compare_action_behaviors(a1->behavior, a2->behavior, compare_flags);
					}
					break;
				} else if(strcmp(a1->behavior, a2->behavior) == 0 && compare_flags) {
					*compare_flags |= CYBERIADA_ACTION_DIFF_GUARDS;
				}
			}
		}
		if (!found) {
			if (compare_flags) *compare_flags |= CYBERIADA_ACTION_DIFF_BEHAVIOR_ACTION;
			return CYBERIADA_NO_ERROR;
		}
	}

	return CYBERIADA_NO_ERROR;
}
