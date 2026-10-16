/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The Lapki dialect writer test
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include <math.h>
#include <string.h>
#include <cyberiadaml.h>
#include "testutils.h"

#define OUT "31-out.graphml"

static CyberiadaDocument* load(const char* filename)
{
	CyberiadaDocument* doc = cyberiada_new_sm_document();
	TEST_ASSERT(doc);
	TEST_ASSERT(cyberiada_read_sm_document(doc, filename, cybxmlUnknown, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	return doc;
}

static char* text(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	long size;
	char* buffer;
	TEST_ASSERT(f);
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = (char*)malloc(size + 1);
	TEST_ASSERT(buffer && fread(buffer, 1, size, f) == (size_t)size);
	buffer[size] = 0;
	fclose(f);
	return buffer;
}

static size_t count(const char* haystack, const char* needle)
{
	size_t n = 0;
	for (; (haystack = strstr(haystack, needle)); haystack++) n++;
	return n;
}

/* the geometry element written for the element with the id */
static int written_kind(const char* buffer, const char* element, const char* id, const char* kind)
{
	char start[256];
	const char *s, *end;
	snprintf(start, sizeof(start), "<%s id=\"%s\"", element, id);
	s = strstr(buffer, start);
	TEST_ASSERT(s);
	end = strstr(s, strcmp(element, "node") == 0 ? "</node>" : "</edge>");
	TEST_ASSERT(end);
	s = strstr(s, kind);
	return s && s < end;
}

static CyberiadaNode* node(CyberiadaDocument* doc, const char* id)
{
	CyberiadaNode* n = cyberiada_graph_find_node_by_id(doc->state_machines->nodes, id);
	TEST_ASSERT(n);
	return n;
}

static CyberiadaEdge* edge(CyberiadaDocument* doc, const char* id)
{
	CyberiadaEdge* e;
	for (e = doc->state_machines->edges; e; e = e->next) {
		if (strcmp(e->id, id) == 0) return e;
	}
	TEST_ASSERT(0);
	return NULL;
}

int main(void)
{
	CyberiadaDocument* doc;
	CyberiadaMetaStringList* s;
	CyberiadaRect* rect;
	char* buffer;

	/* the old-style components, the initial behavior alone, the standard format label */
	doc = load("diagrams/lapki-old.graphml");
	rect = htree_new_rect();
	rect->x = rect->y = 5.0;
	rect->width = 40.0;
	rect->height = 20.0;
	edge(doc, "on-off")->geometry_label_rect = rect;
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	buffer = text(OUT);
	TEST_ASSERT(strstr(buffer, "<data key=\"gFormat\">Cyberiada-GraphML-1.0</data>"));
	TEST_ASSERT(strstr(buffer, "<data key=\"dName\">CGML_COMPONENT</data>"));
	TEST_ASSERT(strstr(buffer, "id/ LED1\n\ntype/ LED"));
	TEST_ASSERT(!strstr(buffer, "CGML_COMPONENT LED1"));
	TEST_ASSERT(strstr(buffer, "<data key=\"dData\">LED1.off()\n</data>"));
	TEST_ASSERT(written_kind(buffer, "edge", "on-off", "<point"));
	free(buffer);

	/* the written document reads back to the same model; the Lapki format reads as 1.0 */
	doc = cyberiada_new_sm_document();
	TEST_ASSERT(cyberiada_read_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(strcmp(doc->format, "Cyberiada-GraphML-Lapki") == 0);
	TEST_ASSERT(strcmp(node(doc, "cLED1")->title, "CGML_COMPONENT LED1") == 0);
	TEST_ASSERT(strcmp(edge(doc, "init-on")->action->behavior, "LED1.off()") == 0);
	TEST_ASSERT(strcmp(edge(doc, "on-off")->action->trigger, "timer.timeout") == 0);

	/* what the Lapki tools cannot hold is refused: a completion transition */
	edge(doc, "on-off")->action->trigger[0] = 0;
	free(edge(doc, "on-off")->action->behavior);
	edge(doc, "on-off")->action->behavior = (char*)malloc(sizeof("finish()"));
	strcpy(edge(doc, "on-off")->action->behavior, "finish()");
	edge(doc, "on-off")->action->behavior_len = strlen("finish()");
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlCyberiada10, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	/* a formal comment other than the meta and the components */
	doc = load("diagrams/lapki-old.graphml");
	node(doc, "cLED1")->title[0] = 'X';
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);

	/* the platform is required; every machine gets the meta node, comments a point */
	doc = load("diagrams/lapki-js.graphml");
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_FORMAT_ERROR);
	for (s = doc->meta_info->strings; s && s->next; s = s->next);
	if (s) {
		s->next = cyberiada_new_meta_string("platform", "ArduinoUno");
	} else {
		doc->meta_info->strings = cyberiada_new_meta_string("platform", "ArduinoUno");
	}
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	buffer = text(OUT);
	TEST_ASSERT(count(buffer, "<data key=\"dName\">CGML_META</data>") == 2);
	TEST_ASSERT(count(buffer, "platform/ ArduinoUno") == 2);
	TEST_ASSERT(written_kind(buffer, "node", "note", "<point"));
	free(buffer);

	/* the absent geometry is laid out */
	doc = load("diagrams/lapki-old.graphml");
	htree_destroy_rect(node(doc, "off")->geometry_rect);
	node(doc, "off")->geometry_rect = NULL;
	TEST_ASSERT(cyberiada_write_sm_document(doc, OUT, cybxmlLapki, CYBERIADA_FLAG_NO) ==
				CYBERIADA_NO_ERROR);
	TEST_ASSERT(cyberiada_destroy_sm_document(doc) == CYBERIADA_NO_ERROR);
	buffer = text(OUT);
	TEST_ASSERT(written_kind(buffer, "node", "off", "<rect"));
	free(buffer);

	return 0;
}
