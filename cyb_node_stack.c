/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The node stack structure
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

#include "cyb_node_stack.h"

int node_stack_push(NodeStack** stack)
{
	cyberiada_stack_push(stack);
	return CYBERIADA_NO_ERROR;
}

int node_stack_set_top_node(NodeStack** stack, CyberiadaNode* node)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_update_top_data((CyberiadaStack**)stack, (void*)node);
	return CYBERIADA_NO_ERROR;
}

int node_stack_set_top_element(NodeStack** stack, const char* element)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_update_top_key(stack, element);
	return CYBERIADA_NO_ERROR;	
}

CyberiadaNode* node_stack_current_node(NodeStack** stack)
{
	return cyberiada_stack_get_top_data(stack);
}

int node_stack_pop(NodeStack** stack)
{
	if (!stack || !*stack) {
		return CYBERIADA_BAD_PARAMETER;
	}
	cyberiada_stack_pop(stack);
	return CYBERIADA_NO_ERROR;
}

int node_stack_empty(NodeStack** stack)
{
	return cyberiada_stack_is_empty(stack);
}

int node_stack_free(NodeStack** stack)
{
	return cyberiada_stack_free(stack);
}
