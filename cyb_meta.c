/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The graph metainformation
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
#include <stdio.h>

#include "cyb_meta.h"
#include "cyb_string.h"
#include "cyb_error.h"

#define CYBERIADA_META_SEPARATOR_CHR             '/'
#define CYBERIADA_META_NEW_LINE_CHR              '\n'
#define CYBERIADA_META_NEW_LINE_STR              "\n"

CyberiadaMetaStringList* cyberiada_new_meta_string(const char* _name, const char* _value)
{
	CyberiadaMetaStringList* metastr = (CyberiadaMetaStringList*)malloc(sizeof(CyberiadaMetaStringList));
	memset(metastr, 0, sizeof(CyberiadaMetaStringList));
	cyberiada_copy_string(&(metastr->name), &(metastr->name_len), _name);
	cyberiada_copy_string(&(metastr->value), &(metastr->value_len), _value);
	return metastr;
}

static int cyberiada_destroy_meta_string(CyberiadaMetaStringList* sl)
{
	if (!sl) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (sl->name) free(sl->name);
	if (sl->value) free(sl->value);
	free(sl);
	return CYBERIADA_NO_ERROR;
}

CyberiadaMetainformation* cyberiada_new_meta(void)
{
	CyberiadaMetainformation* meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	cyberiada_copy_string(&(meta->standard_version),
						  &(meta->standard_version_len),
						  CYBERIADA_STANDARD_VERSION_CYBERIADAML);
	meta->transition_order_flag = 1;
	meta->event_propagation_flag = 1;
	return meta;
}

CyberiadaMetainformation* cyberiada_copy_meta(CyberiadaMetainformation* src)
{
	CyberiadaMetainformation* dst;
	CyberiadaMetaStringList* sl;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_meta();
	free(dst->standard_version);
	cyberiada_copy_string(&(dst->standard_version), &(dst->standard_version_len), src->standard_version);
	dst->transition_order_flag = src->transition_order_flag;
	dst->event_propagation_flag = src->event_propagation_flag;
	sl = src->strings;
	while(sl) {
		CyberiadaMetaStringList* dst_sl = NULL;
		if (dst->strings) {
			dst_sl = dst->strings;
			while (dst_sl->next) dst_sl = dst_sl->next;
			dst_sl->next = cyberiada_new_meta_string(sl->name, sl->value);
		} else {
			dst->strings = cyberiada_new_meta_string(sl->name, sl->value);
		}
		sl = sl->next;
	}
	return dst;
}

int cyberiada_destroy_meta(CyberiadaMetainformation* meta)
{
	CyberiadaMetaStringList *sl, *next;
	if (meta) {
		if (meta->standard_version) free(meta->standard_version);
		sl = meta->strings;
		while (sl) {
			next = sl->next;
			cyberiada_destroy_meta_string(sl);
			sl = next;
		}
		free(meta);
	}
	return CYBERIADA_NO_ERROR;
}

const char* cyberiada_find_meta_string(CyberiadaMetainformation* meta, const char* name)
{
	CyberiadaMetaStringList *sl;
	if (!meta || !meta->strings) {
		return NULL;
	}
	while (sl) {
		if (sl->name && strcmp(sl->name, name) == 0) {
			return sl->value;
		}
		sl = sl->next;
	}
	return NULL;
}

int cyberiada_add_default_meta(CyberiadaDocument* doc, const char* sm_name)
{
	CyberiadaMetainformation* meta;
	
	if (doc->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = cyberiada_new_meta();

	if (*sm_name) {
		meta->strings = cyberiada_new_meta_string(CYBERIADA_META_NAME, sm_name);
	}
		
	doc->meta_info = meta;
	return CYBERIADA_NO_ERROR;	
}

int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body, size_t* meta_body_len)
{
	size_t buffer_len, buffer_len_result;
	int written;
	char *buffer;
	CyberiadaMetaStringList* sl;

	if (!meta) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	/* calculate buffer length */
	buffer_len = 1 + strlen(CYBERIADA_META_STANDARD_VERSION) + strlen(meta->standard_version) + 4;
	buffer_len += (strlen(CYBERIADA_META_TRANSITION_ORDER) +
				   (meta->transition_order_flag == 1 ? strlen(CYBERIADA_META_AO_TRANSITION) : strlen(CYBERIADA_META_AO_EXIT)) +
				   strlen(CYBERIADA_META_EVENT_PROPAGATION) +
				   (meta->event_propagation_flag == 1 ? strlen(CYBERIADA_META_EP_BLOCK) : strlen(CYBERIADA_META_EP_PROPAGATE)) + 
				   8);
	sl = meta->strings;
	while (sl) {
		if (sl->name && sl->value) {
			buffer_len += strlen(sl->name) + strlen(sl->value) + 4;
		}
		sl = sl->next;
	}
	buffer_len_result = buffer_len;

	if (meta_body) {
		/* write data to buffer */
		buffer = (char*)malloc(buffer_len);
		*meta_body = buffer;
		written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
						   CYBERIADA_META_STANDARD_VERSION,
						   meta->standard_version);
		buffer_len -= (size_t)written;
		buffer += written;
		sl = meta->strings;
		while (sl) {
			if (sl->name && sl->value) {
				written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
								   sl->name,
								   sl->value);
				buffer_len -= (size_t)written;
				buffer += written;
			}
			sl = sl->next;
		}
		written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
						   CYBERIADA_META_TRANSITION_ORDER,
						   meta->transition_order_flag == 1 ? CYBERIADA_META_AO_TRANSITION : CYBERIADA_META_AO_EXIT);
		buffer_len -= (size_t)written;
		buffer += written;
		written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
						   CYBERIADA_META_EVENT_PROPAGATION,
						   meta->event_propagation_flag == 1 ? CYBERIADA_META_EP_BLOCK : CYBERIADA_META_EP_PROPAGATE);
		buffer_len -= (size_t)written;
		buffer += written;
		*buffer = 0;
	}
	if (meta_body_len) {
		if (meta_body) {
			*meta_body_len = strlen(*meta_body);
		} else {
			*meta_body_len = buffer_len_result;
		}
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_meta(CyberiadaDocument* doc, char* metadata, CyberiadaRegexps* regexps)
{
	CyberiadaMetainformation* meta;
	char  *start, *block, *block2, *next, *parts;
	
	if (doc->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = (CyberiadaMetainformation*)malloc(sizeof(CyberiadaMetainformation));
	memset(meta, 0, sizeof(CyberiadaMetainformation));

	next = metadata;	
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
		if (regexec(&(regexps->spaces_regexp), start, 0, NULL, 0) == 0) {
			continue;
		}

		parts = strchr(start, CYBERIADA_META_SEPARATOR_CHR);
		if (parts == NULL) {
			ERROR("Error decoding SM metainformation: cannot find separator\n");
			cyberiada_destroy_meta(meta);
			return CYBERIADA_METADATA_FORMAT_ERROR;
		}
		*parts = 0;
		do {
			parts++;
		} while (isspace(*parts));
		cyberiada_string_trim(parts);
		
		if (strcmp(start, CYBERIADA_META_STANDARD_VERSION) == 0) {
			cyberiada_copy_string(&(meta->standard_version), &(meta->standard_version_len), parts);
		} else if (strcmp(start, CYBERIADA_META_TRANSITION_ORDER) == 0) {
			if (strcmp(parts, CYBERIADA_META_AO_TRANSITION) == 0) {
				meta->transition_order_flag = 1;
			} else if (strcmp(parts, CYBERIADA_META_AO_EXIT) == 0) {
				meta->transition_order_flag = 2;
			} else {
				ERROR("Error decoding SM metainformation: bad value of actions order flag parameter\n");
				cyberiada_destroy_meta(meta);
				return CYBERIADA_METADATA_FORMAT_ERROR;					
			}
		} else if (strcmp(start, CYBERIADA_META_EVENT_PROPAGATION) == 0) {
			if (strcmp(parts, CYBERIADA_META_EP_BLOCK) == 0) {
				meta->event_propagation_flag = 1;
			} else if (strcmp(parts, CYBERIADA_META_EP_PROPAGATE) == 0) {
				meta->event_propagation_flag = 2;
			} else {
				ERROR("Error decoding SM metainformation: bad value of event propagation flag parameter\n");
				cyberiada_destroy_meta(meta);
				return CYBERIADA_METADATA_FORMAT_ERROR;					
			}
		} else if (strlen(start) == 0) {
			ERROR("Error decoding SM metainformation: empty key\n");
			cyberiada_destroy_meta(meta);
			return CYBERIADA_METADATA_FORMAT_ERROR;
		} else {
			CyberiadaMetaStringList *sl;
			if (meta->strings) {
				sl = meta->strings;
				while (sl->next) sl = sl->next;
				sl->next = cyberiada_new_meta_string(start, parts);
			} else {
				meta->strings = cyberiada_new_meta_string(start, parts);
			}
		}
	}
	
	if (!meta->standard_version) {
		ERROR("Error decoding SM metainformation: standard version is not set\n");
		cyberiada_destroy_meta(meta);
		return CYBERIADA_METADATA_FORMAT_ERROR;
	}

	if (strcmp(meta->standard_version, CYBERIADA_STANDARD_VERSION_CYBERIADAML) != 0) {
		ERROR("Error decoding SM metainformation: unsupported version of Cyberiada standard - %s\n",
			  meta->standard_version);
		cyberiada_destroy_meta(meta);
		return CYBERIADA_METADATA_FORMAT_ERROR;
	}
	
	// set default values
	if (!meta->transition_order_flag) {
		meta->transition_order_flag = 1;
	}
	if (!meta->event_propagation_flag) {
		meta->event_propagation_flag = 1;
	}
	
	doc->meta_info = meta;
	
	return CYBERIADA_NO_ERROR;
}

int cyberiada_print_meta(CyberiadaMetainformation* meta)
{
	CyberiadaMetaStringList *sl;
	
	printf("Meta information:\n");

	if (meta) {		
		if (meta->standard_version) {
			printf(" %s: %s\n", CYBERIADA_META_STANDARD_VERSION, meta->standard_version);
		}

		sl = meta->strings;
		while (sl) {
			if (sl->name && sl->value) {
				printf(" %s: %s\n", sl->name, sl->value);
			}
			sl = sl->next;
		}
	
		if (meta->transition_order_flag) {
			printf(" trantision order flag: %d\n", meta->transition_order_flag);
		}
		if (meta->event_propagation_flag) {
			printf(" event propagation flag: %d\n", meta->event_propagation_flag);
		}
	}
	return CYBERIADA_NO_ERROR;
}
