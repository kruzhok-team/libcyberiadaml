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

#define CYBERIADA_STANDARD_VERSION_CYBERIADAML   "1.0"

#define CYBERIADA_META_SEPARATOR_CHR             '/'
#define CYBERIADA_META_NEW_LINE_CHR              '\n'
#define CYBERIADA_META_NEW_LINE_STR              "\n"
#define CYBERIADA_META_STANDARD_VERSION          "standardVersion"
#define CYBERIADA_META_PLATFORM_NAME             "platform"
#define CYBERIADA_META_PLATFORM_VERSION          "platformVersion"
#define CYBERIADA_META_PLATFORM_LANGUAGE         "platformLanguage"
#define CYBERIADA_META_TARGET_SYSTEM             "target"
#define CYBERIADA_META_NAME                      "name"
#define CYBERIADA_META_AUTHOR                    "author"
#define CYBERIADA_META_CONTACT                   "contact"
#define CYBERIADA_META_DESCRIPTION               "description"
#define CYBERIADA_META_VERSION                   "version"
#define CYBERIADA_META_DATE                      "date"
#define CYBERIADA_META_TRANSITION_ORDER          "transitionOrder"
#define CYBERIADA_META_AO_TRANSITION             "transitionFirst"
#define CYBERIADA_META_AO_EXIT                   "exitFirst"
#define CYBERIADA_META_EVENT_PROPAGATION         "eventPropagation"
#define CYBERIADA_META_EP_PROPAGATE              "propagate"
#define CYBERIADA_META_EP_BLOCK                  "block"
#define CYBERIADA_META_MARKUP_LANGUAGE           "markupLanguage"

typedef struct {
	const char* name;
	size_t value_offset;
	size_t len_offset;
	const char* title;
} MetainfoDeclaration;

static MetainfoDeclaration cyberiada_metadata[] = {
	{
		CYBERIADA_META_STANDARD_VERSION,
		offsetof(CyberiadaMetainformation, standard_version),
		offsetof(CyberiadaMetainformation, standard_version_len),
		"standard version"
	}, {
		CYBERIADA_META_PLATFORM_NAME,
		offsetof(CyberiadaMetainformation, platform_name),
		offsetof(CyberiadaMetainformation, platform_name_len),
		"platform name"
	}, {
		CYBERIADA_META_PLATFORM_VERSION,
		offsetof(CyberiadaMetainformation, platform_version),
		offsetof(CyberiadaMetainformation, platform_version_len),
		"platform version"
	}, {
		CYBERIADA_META_PLATFORM_LANGUAGE,
		offsetof(CyberiadaMetainformation, platform_language),
		offsetof(CyberiadaMetainformation, platform_language_len),
		"platform language"
	}, {
		CYBERIADA_META_TARGET_SYSTEM,
		offsetof(CyberiadaMetainformation, target_system),
		offsetof(CyberiadaMetainformation, target_system_len),
		"target system"
	}, {
		CYBERIADA_META_NAME,
		offsetof(CyberiadaMetainformation, name),
		offsetof(CyberiadaMetainformation, name_len),
		"document name"
	}, {
		CYBERIADA_META_AUTHOR,
		offsetof(CyberiadaMetainformation, author),
		offsetof(CyberiadaMetainformation, author_len),
		"document author"
	}, {
		CYBERIADA_META_CONTACT,
		offsetof(CyberiadaMetainformation, contact),
		offsetof(CyberiadaMetainformation, contact_len),
		"document author's contact"
	}, {
		CYBERIADA_META_DESCRIPTION,
		offsetof(CyberiadaMetainformation, description),
		offsetof(CyberiadaMetainformation, description_len),
		"document description"
	}, {
		CYBERIADA_META_VERSION,
		offsetof(CyberiadaMetainformation, version),
		offsetof(CyberiadaMetainformation, version_len),
		"document version"
	}, {
		CYBERIADA_META_DATE,
		offsetof(CyberiadaMetainformation, date),
		offsetof(CyberiadaMetainformation, date_len),
		"document date"
	}, {
		CYBERIADA_META_MARKUP_LANGUAGE,
		offsetof(CyberiadaMetainformation, markup_language),
		offsetof(CyberiadaMetainformation, markup_language_len),
		"markup language"
	}
};

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
	size_t i;
	char *value;
	CyberiadaMetainformation* dst;
	if (!src) {
		return NULL;
	}
	dst = cyberiada_new_meta();
	free(dst->standard_version);
	for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
		value = *((char**)((char*)src + cyberiada_metadata[i].value_offset));
		if (value) {
			cyberiada_copy_string((char**)((char*)dst + cyberiada_metadata[i].value_offset),
								  (size_t*)((char*)dst + cyberiada_metadata[i].len_offset), value);
		}
	}
	dst->transition_order_flag = src->transition_order_flag;
	dst->event_propagation_flag = src->event_propagation_flag;	
	return dst;
}

int cyberiada_destroy_meta(CyberiadaMetainformation* meta)
{
	if (meta) {
		if (meta->standard_version) free(meta->standard_version);
		if (meta->platform_name) free(meta->platform_name);
		if (meta->platform_version) free(meta->platform_version);
		if (meta->platform_language) free(meta->platform_language);
		if (meta->target_system) free(meta->target_system);
		if (meta->name) free(meta->name);
		if (meta->author) free(meta->author);
		if (meta->contact) free(meta->contact);
		if (meta->description) free(meta->description);
		if (meta->version) free(meta->version);
		if (meta->date) free(meta->date);
		if (meta->markup_language) free(meta->markup_language);
		free(meta);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_add_default_meta(CyberiadaDocument* doc, const char* sm_name)
{
	CyberiadaMetainformation* meta;
	
	if (doc->meta_info) {
		return CYBERIADA_BAD_PARAMETER;
	}
	
	meta = cyberiada_new_meta();

	if (*sm_name) {
		cyberiada_copy_string(&(meta->name), &(meta->name_len), sm_name);
	}
		
	doc->meta_info = meta;
	return CYBERIADA_NO_ERROR;	
}

int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body, size_t* meta_body_len)
{
	size_t i, buffer_len;
	int written;
	char *buffer, *value;

	buffer_len = 1;
	for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
		value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
		if (value) {
			buffer_len += (strlen(cyberiada_metadata[i].name) +
						   *(size_t*)((char*)meta + cyberiada_metadata[i].len_offset) +
						   4);
		}
	}
	buffer_len += (strlen(CYBERIADA_META_TRANSITION_ORDER) +
				   (meta->transition_order_flag == 1 ? strlen(CYBERIADA_META_AO_TRANSITION) : strlen(CYBERIADA_META_AO_EXIT)) +
				   strlen(CYBERIADA_META_EVENT_PROPAGATION) +
				   (meta->event_propagation_flag == 1 ? strlen(CYBERIADA_META_EP_BLOCK) : strlen(CYBERIADA_META_EP_PROPAGATE)) + 
				   8);		
	buffer = (char*)malloc(buffer_len);
	*meta_body = buffer;
	for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
		value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
		if (value) {
			written = snprintf(buffer, buffer_len, "%s/ %s\n\n",
							   cyberiada_metadata[i].name,
							   value);
			buffer_len -= (size_t)written;
			buffer += written;
		}
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
	if (meta_body_len) {
		*meta_body_len = strlen(*meta_body);
	}
	return CYBERIADA_NO_ERROR;
}

int cyberiada_decode_meta(CyberiadaDocument* doc, char* metadata, CyberiadaRegexps* regexps)
{
	CyberiadaMetainformation* meta;
	char  *start, *block, *block2, *next, *parts;
	size_t i;
	char found;
	
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
		
		found = 0;
		for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
			if (strcmp(start, cyberiada_metadata[i].name) == 0) {
				if (*(char**)((char*)meta + cyberiada_metadata[i].value_offset) != NULL) {
					ERROR("Error decoding SM metainformation: double parameter %s\n",
						  cyberiada_metadata[i].title);
					cyberiada_destroy_meta(meta);
					return CYBERIADA_METADATA_FORMAT_ERROR;					
				}
				cyberiada_copy_string((char**)((char*)meta + cyberiada_metadata[i].value_offset),
									  (size_t*)((char*)meta + cyberiada_metadata[i].len_offset), parts);
				found = 1;
				break;
			}
		}
		if (!found) {
			if (strcmp(start, CYBERIADA_META_TRANSITION_ORDER) == 0) {
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
			} else {
				ERROR("Error decoding SM metainformation: bad key %s\n", start);
				cyberiada_destroy_meta(meta);
				return CYBERIADA_METADATA_FORMAT_ERROR;
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
	size_t i;
	char* value;
	printf("Meta information:\n");

	if (meta) {
		for (i = 0; i < sizeof(cyberiada_metadata) / sizeof(MetainfoDeclaration); i++) {
			value = *((char**)((char*)meta + cyberiada_metadata[i].value_offset));
			if (value) {
				printf(" %s: %s\n", cyberiada_metadata[i].title, value);
			}
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
