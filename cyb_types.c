/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The basic data structures
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

#include <string.h>
#include <stdlib.h>
#include "cyb_types.h"
	
int   cyberiada_stack_push(CyberiadaStack** stack)
{
	CyberiadaStack* new_item = (CyberiadaStack*)malloc(sizeof(CyberiadaStack));
	memset(new_item, 0, sizeof(CyberiadaStack));
	new_item->next = (*stack);
	*stack = new_item;
	return 0;
}

int   cyberiada_stack_update_top_key(CyberiadaStack** stack, const char* new_key)
{
	if (!stack || !*stack) {
		return -1;
	}
	(*stack)->key = new_key;	
	return 0;
}

int   cyberiada_stack_update_top_data(CyberiadaStack** stack, void* new_data)
{
	if (!stack || !*stack) {
		return -1;
	}
	(*stack)->data = new_data;
	return 0;
}

void* cyberiada_stack_get_top_data(CyberiadaStack** stack)
{
	CyberiadaStack* s;
	if (!stack || !*stack) {
		return NULL;
	}
	s = *stack;
	while (s) {
		if (s->data)
			return s->data;
		s = s->next;
	}
	return NULL;
}

int   cyberiada_stack_pop(CyberiadaStack** stack)
{
	CyberiadaStack* to_pop;
	if (!stack || !*stack) {
		return -1;
	}
	to_pop = *stack;
	*stack = to_pop->next;
	free(to_pop);
	return 0;
}

int   cyberiada_stack_is_empty(CyberiadaStack** stack)
{
	return !stack || !*stack;
}

int   cyberiada_stack_free(CyberiadaStack** stack)
{
	while (!cyberiada_stack_is_empty(stack)) {
		cyberiada_stack_pop(stack);
	}
	return 0;
}

int   cyberiada_list_add(CyberiadaList** list, const char* key, void* data)
{
	CyberiadaList* new_item;
	if (!list) {
		return -1;
	}
	new_item = (CyberiadaList*)malloc(sizeof(CyberiadaList));
	new_item->key = key;
	new_item->data = data;
	new_item->next = *list;
	*list = new_item;
	return 0;
}

void* cyberiada_list_find(CyberiadaList** list, const char* key)
{
	CyberiadaList* item;
	if (!list || !*list) {
		return NULL;
	}
	item = *list;
	while (item) {
		if (item->key && strcmp(item->key, key) == 0) {
			return item->data;
		}
		item = item->next;
	}
	return NULL;
}

int   cyberiada_list_free(CyberiadaList** list)
{
	CyberiadaList* item;
	if (!list) {
		return 0;
	}
	while(*list) {
		item = *list;
		*list = item->next;
		free(item);
	}
	return 0;
}
