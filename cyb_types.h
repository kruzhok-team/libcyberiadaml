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

#ifndef __CYBERIADA_TYPES_H
#define __CYBERIADA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML basic data structures: stack and list
 * ----------------------------------------------------------------------------- */

	struct _CyberiadaStruct {
		void*                    key;
		void*                    data;
		struct _CyberiadaStruct* next;
	};

	typedef struct _CyberiadaStruct CyberiadaStack; 
	
	int   cyberiada_stack_push(CyberiadaStack** stack);	
	int   cyberiada_stack_update_top_key(CyberiadaStack** stack, const char* new_key);
	int   cyberiada_stack_update_top_data(CyberiadaStack** stack, void* new_data);
	void* cyberiada_stack_get_top_data(CyberiadaStack** stack);
	int   cyberiada_stack_pop(CyberiadaStack** stack);
	int   cyberiada_stack_is_empty(CyberiadaStack** stack);
	int   cyberiada_stack_free(CyberiadaStack** stack);

	typedef struct _CyberiadaStruct CyberiadaList; 
	
	int   cyberiada_list_add(CyberiadaList** list, const char* key, void* data);	
	void* cyberiada_list_find(CyberiadaList** list, const char* key);
	int   cyberiada_list_free(CyberiadaList** list);

	typedef struct _CyberiadaStruct CyberiadaQueue; 
	
	int   cyberiada_queue_add(CyberiadaQueue** queue, void* key, void* data);	
	int   cyberiada_queue_get(CyberiadaQueue** queue, void** key, void**data);
	int   cyberiada_queue_free(CyberiadaQueue** queue);
	
#ifdef __cplusplus
}
#endif
    
#endif
