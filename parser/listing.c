/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The listing of a document: an element per line, the textconv driver of git
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cyberiadaml.h"
#include "cyberiadaml_diff.h"
#include "listing.h"

/* The listing is the text of the revision diff of an empty document and the document,
   every element added, without the marks: the text of hsm-git textconv, so a repository
   set up for it keeps its diffs. */

#define LISTING_PROGRAM          "cybparser listing"
#define LISTING_INDENT           "  "
#define LISTING_UNKNOWN          "?"
#define LISTING_ELEMENT          "element"
#define LISTING_TRANSITION       "transition"
#define LISTING_STATE_MACHINE    "state machine"
#define LISTING_META             "meta"
#define LISTING_EDITOR_VIEW      "cyberiadaEditorView"
#define LISTING_COPY_BUFFER      4096

typedef struct {
	CyberiadaNodeType type;
	const char*       word;
} ListingWord;

/* the words of the editor */
static const ListingWord listing_words[] = {
	{cybNodeSM,              LISTING_STATE_MACHINE},
	{cybNodeSimpleState,     "state"},
	{cybNodeCompositeState,  "state"},
	{cybNodeSubmachineState, "submachine"},
	{cybNodeComment,         "comment"},
	{cybNodeFormalComment,   "formal comment"},
	{cybNodeInitial,         "initial"},
	{cybNodeFinal,           "final"},
	{cybNodeChoice,          "choice"},
	{cybNodeTerminate,       "terminate"},
	{cybNodeShallowHistory,  "shallow history"},
	{cybNodeDeepHistory,     "deep history"},
	{cybNodeEntryPoint,      "entry point"},
	{cybNodeExitPoint,       "exit point"}
};

/* the editor's own metainformation, written at every save: shown with the geometry only */
static const char* listing_geometry_meta[] = {
	LISTING_EDITOR_VIEW,
	CYBERIADA_META_REGION_GEOMETRY
};

/* the names of the event propagation values, as the binding of the library tells them */
static const char* listing_propagation[] = {
	"eventPropagationNone",
	"eventPropagationBlock",
	"eventPropagationPropagate",
	"eventPropagationDefer"
};

typedef struct {
	const char* word;
	const char* name;
	size_t      count;
} ListingKey;

typedef struct {
	ListingKey*           keys;
	size_t                keys_size;
	const CyberiadaNode** shown;          /* the nodes already written */
	size_t                shown_size;
	size_t                capacity;
} Listing;

typedef struct {
	const char* word;
	const char* name;
	const char* id;
} ListingName;

static int listing_is_state(const CyberiadaNode* node)
{
	return node->type == cybNodeSimpleState || node->type == cybNodeCompositeState ||
		node->type == cybNodeSubmachineState;
}

/* the binding keeps the actions of a simple and a composite state only: a submachine
   state has none (PNST 1044 8.1) */
static int listing_has_actions(const CyberiadaNode* node)
{
	return node->type == cybNodeSimpleState || node->type == cybNodeCompositeState;
}

static int listing_is_comment(const CyberiadaNode* node)
{
	return node->type == cybNodeComment || node->type == cybNodeFormalComment;
}

/* the elements with a color: the states, the comments and the choices */
static int listing_has_color(const CyberiadaNode* node)
{
	return listing_is_state(node) || listing_is_comment(node) || node->type == cybNodeChoice;
}

static int listing_is_set(const char* s)
{
	return s && *s;
}

static const char* listing_string(const char* s)
{
	return s ? s : "";
}

static void listing_node_name(const CyberiadaNode* node, ListingName* name)
{
	size_t i;
	name->word = LISTING_ELEMENT;
	for (i = 0; i < sizeof(listing_words) / sizeof(*listing_words); i++) {
		if (listing_words[i].type == node->type) {
			name->word = listing_words[i].word;
			break;
		}
	}
	name->name = listing_string(node->title);
	name->id = node->id;
}

static void listing_edge_name(const CyberiadaEdge* edge, ListingName* name)
{
	name->word = LISTING_TRANSITION;
	name->name = "";
	name->id = edge->id;
}

static ListingKey* listing_find_key(Listing* listing, const ListingName* name)
{
	size_t i;
	for (i = 0; i < listing->keys_size; i++) {
		ListingKey* key = listing->keys + i;
		if (strcmp(key->word, name->word) == 0 && strcmp(key->name, name->name) == 0) {
			return key;
		}
	}
	return NULL;
}

static void listing_count(Listing* listing, const ListingName* name)
{
	ListingKey* key = listing_find_key(listing, name);
	if (!key) {
		key = listing->keys + listing->keys_size++;
		key->word = name->word;
		key->name = name->name;
		key->count = 0;
	}
	key->count++;
}

/* another element of the document has the same word and name */
static int listing_repeated(Listing* listing, const ListingName* name)
{
	ListingKey* key = listing_find_key(listing, name);
	return key && key->count > 1;
}

static void listing_print_indent(int level)
{
	int i;
	for (i = 0; i < level; i++) {
		fputs(LISTING_INDENT, stdout);
	}
}

static void listing_print_id(Listing* listing, const ListingName* name)
{
	if (listing_repeated(listing, name)) {
		printf(" (%s)", name->id);
	}
}

/* the word and the name of an element */
static void listing_print_label(Listing* listing, const ListingName* name)
{
	fputs(name->word, stdout);
	if (*(name->name)) {
		printf(" %s", name->name);
	}
	listing_print_id(listing, name);
}

/* the end of a transition or a link: the name, or the word of an unnamed one */
static void listing_print_short(Listing* listing, const ListingName* name)
{
	if (!name) {
		fputs(LISTING_UNKNOWN, stdout);
		return;
	}
	fputs(*(name->name) ? name->name : name->word, stdout);
	listing_print_id(listing, name);
}

/* the length of the line break at s, 0 when there is none: the breaks str.splitlines()
   of Python knows */
static size_t listing_line_break(const char* s)
{
	const unsigned char* u = (const unsigned char*)s;
	if (u[0] == '\r') return u[1] == '\n' ? 2 : 1;
	if (u[0] == '\n' || u[0] == '\v' || u[0] == '\f' || (u[0] >= 0x1c && u[0] <= 0x1e)) return 1;
	if (u[0] == 0xc2 && u[1] == 0x85) return 2;
	if (u[0] == 0xe2 && u[1] == 0x80 && (u[2] == 0xa8 || u[2] == 0xa9)) return 3;
	return 0;
}

/* the characters of a UTF-8 string of the given bytes */
static size_t listing_characters(const char* s, size_t bytes)
{
	size_t i, n = 0;
	for (i = 0; i < bytes; i++) {
		if (((unsigned char)s[i] & 0xc0) != 0x80) n++;
	}
	return n;
}

static void listing_print_line(int level, size_t indent, const char* line, size_t bytes)
{
	size_t i;
	listing_print_indent(level);
	for (i = 0; i < indent; i++) {
		fputc(' ', stdout);
	}
	fwrite(line, 1, bytes, stdout);
	fputc('\n', stdout);
}

/* a text of several lines continued under its first line, the indent the length of the
   first line of the prefix */
static int listing_print_multiline(int level, const char* prefix, const char* value)
{
	size_t prefix_len = strlen(prefix), value_len = strlen(listing_string(value));
	size_t indent = 0, start = 0, i = 0, lines = 0;
	char* text = (char*)malloc(prefix_len + value_len + 1);

	if (!text) {
		return CYBERIADA_MEMORY_ERROR;
	}
	memcpy(text, prefix, prefix_len);
	memcpy(text + prefix_len, listing_string(value), value_len + 1);

	if (strchr(prefix, '\n')) {
		while (i < prefix_len && !listing_line_break(prefix + i)) i++;
		indent = listing_characters(prefix, i);
	} else {
		indent = listing_characters(prefix, prefix_len);
	}

	i = 0;
	while (text[i]) {
		size_t brk = listing_line_break(text + i);
		if (brk) {
			listing_print_line(level, lines ? indent : 0, text + start, i - start);
			lines++;
			i += brk;
			start = i;
		} else {
			i++;
		}
	}
	if (start < i || lines == 0) {
		listing_print_line(level, lines ? indent : 0, text + start, i - start);
	}
	free(text);
	return CYBERIADA_NO_ERROR;
}

static int listing_print_text(int level, const char* prefix, const char* value)
{
	listing_print_indent(level);
	printf("%s%s\n", prefix, listing_string(value));
	return CYBERIADA_NO_ERROR;
}

/* the parent the binding sees: a region is no element, its nodes belong to its state */
static const CyberiadaNode* listing_parent(const CyberiadaNode* node)
{
	const CyberiadaNode* parent = node->parent;
	while (parent && parent->type == cybNodeRegion) {
		parent = parent->parent;
	}
	return parent;
}

static int listing_top_level(const CyberiadaNode* node)
{
	const CyberiadaNode* parent = listing_parent(node);
	return !parent || parent->type == cybNodeSM;
}

/* the states holding the node */
static int listing_depth(const CyberiadaNode* node)
{
	int depth = 0;
	while (!listing_top_level(node)) {
		node = listing_parent(node);
		depth++;
	}
	return depth;
}

static int listing_shown(Listing* listing, const CyberiadaNode* node)
{
	size_t i;
	for (i = 0; i < listing->shown_size; i++) {
		if (listing->shown[i] == node) return 1;
	}
	return 0;
}

static void listing_show(Listing* listing, const CyberiadaNode* node)
{
	if (!listing_shown(listing, node) && listing->shown_size < listing->capacity) {
		listing->shown[listing->shown_size++] = node;
	}
}

/* the line of a node: its label and the submachine it refers to */
static void listing_print_head(Listing* listing, const CyberiadaNode* node, int level)
{
	ListingName name;
	listing_node_name(node, &name);
	listing_print_indent(level);
	listing_print_label(listing, &name);
	if (node->type == cybNodeSubmachineState) {
		printf(" -> %s", node->link ? listing_string(node->link->ref) : "");
	}
	fputc('\n', stdout);
}

/* the states holding a node written before it, from the outermost one */
static void listing_print_context(Listing* listing, const CyberiadaNode* node, int level)
{
	const CyberiadaNode* outer;
	if (listing_top_level(node)) {
		return;
	}
	outer = listing_parent(node);
	listing_print_context(listing, outer, level);
	if (!listing_shown(listing, outer)) {
		listing_show(listing, outer);
		listing_print_head(listing, outer, level + listing_depth(outer));
	}
}

static int listing_print_node(Listing* listing, const CyberiadaNode* node, int level)
{
	const CyberiadaAction* a;
	int res = CYBERIADA_NO_ERROR;
	int depth;

	listing_print_context(listing, node, level);
	listing_show(listing, node);
	depth = level + listing_depth(node);
	listing_print_head(listing, node, depth);
	depth++;

	if (listing_has_actions(node)) {
		for (a = node->actions; a && res == CYBERIADA_NO_ERROR; a = a->next) {
			if (a->type == cybActionEntry) {
				res = listing_print_multiline(depth, "entry: ", a->behavior);
			} else if (a->type == cybActionExit) {
				res = listing_print_multiline(depth, "exit: ", a->behavior);
			} else {
				const char* trigger = listing_string(a->trigger);
				const char* guard = listing_string(a->guard);
				char* prefix = (char*)malloc(strlen(trigger) + strlen(guard) + sizeof(" []: "));
				if (!prefix) {
					return CYBERIADA_MEMORY_ERROR;
				}
				if (*guard) {
					sprintf(prefix, "%s [%s]: ", trigger, guard);
				} else {
					sprintf(prefix, "%s: ", trigger);
				}
				res = listing_print_multiline(depth, prefix, a->behavior);
				free(prefix);
			}
		}
	}
	if (res == CYBERIADA_NO_ERROR && listing_is_comment(node) && node->comment_data &&
		listing_is_set(node->comment_data->body)) {
		res = listing_print_multiline(depth, "text: ", node->comment_data->body);
	}
	if (res == CYBERIADA_NO_ERROR && listing_has_color(node) && listing_is_set(node->color)) {
		res = listing_print_text(depth, "color: ", node->color);
	}
	return res;
}

static void listing_print_end(Listing* listing, const CyberiadaNode* node)
{
	ListingName name;
	if (!node || node->type == cybNodeRegion) {
		listing_print_short(listing, NULL);
		return;
	}
	listing_node_name(node, &name);
	listing_print_short(listing, &name);
}

static int listing_print_transition(Listing* listing, const CyberiadaEdge* edge, int level)
{
	const CyberiadaAction* action = edge->action;
	int res = CYBERIADA_NO_ERROR;

	listing_print_indent(level);
	fputs(LISTING_TRANSITION " ", stdout);
	listing_print_end(listing, edge->source);
	fputs(" -> ", stdout);
	listing_print_end(listing, edge->target);
	if (action && listing_is_set(action->trigger)) {
		printf(" [%s]", action->trigger);
	}
	fputc('\n', stdout);

	level++;
	if (action) {
		if (listing_is_set(action->guard)) {
			res = listing_print_multiline(level, "guard: ", action->guard);
		}
		if (res == CYBERIADA_NO_ERROR && listing_is_set(action->behavior)) {
			res = listing_print_multiline(level, "do: ", action->behavior);
		}
		if (res == CYBERIADA_NO_ERROR && action->propagation != cybEventPropagationNone &&
			(size_t)action->propagation < sizeof(listing_propagation) / sizeof(*listing_propagation)) {
			res = listing_print_text(level, "propagation: ", listing_propagation[action->propagation]);
		}
	}
	if (res == CYBERIADA_NO_ERROR && listing_is_set(edge->color)) {
		res = listing_print_text(level, "color: ", edge->color);
	}
	return res;
}

/* the line of a comment link: the comment, its subject and the fragment */
static void listing_print_link(Listing* listing, const CyberiadaEdge* edge, int level)
{
	ListingName name;
	const CyberiadaNode* comment = edge->source;

	if (!comment || !listing_is_comment(comment)) {
		return;
	}
	listing_print_indent(level);
	fputs("link ", stdout);
	listing_node_name(comment, &name);
	listing_print_label(listing, &name);
	fputs(" -> ", stdout);
	if (edge->target_edge) {
		listing_edge_name(edge->target_edge, &name);
		listing_print_short(listing, &name);
	} else {
		listing_print_end(listing, edge->target);
	}
	if (edge->comment_subject && edge->comment_subject->type != cybCommentSubjectNode) {
		printf(": %s", listing_string(edge->comment_subject->fragment));
	}
	fputc('\n', stdout);
}

/* the differences the text shows: no geometry */
static unsigned int listing_visible(unsigned int flags)
{
	return flags & ~(CYBERIADA_DIFF_FLAG_GEOMETRY | CYBERIADA_DIFF_FLAG_COLLAPSED | CYBERIADA_DIFF_FLAG_ELEMENTS);
}

/* the links of the comments in the order of the comments in the tree, each comment's in
   the order of the document, as the binding writes them */
static void listing_print_links(Listing* listing, const CyberiadaSMDiff* sm, const CyberiadaNode* nodes, int level)
{
	const CyberiadaNode* node;
	size_t i;
	for (node = nodes; node; node = node->next) {
		if (listing_is_comment(node)) {
			for (i = 0; i < sm->edges_size; i++) {
				const CyberiadaEdge* edge = sm->edges[i].e2;
				if (listing_visible(sm->edges[i].flags) && edge && edge->type == cybEdgeComment &&
					edge->source == node) {
					listing_print_link(listing, edge, level);
				}
			}
		}
		listing_print_links(listing, sm, node->children, level);
	}
}

static int listing_print_sm(Listing* listing, const CyberiadaSMDiff* sm)
{
	size_t i;
	int res = CYBERIADA_NO_ERROR, any = 0;
	const int level = 1;

	for (i = 0; i < sm->nodes_size && !any; i++) {
		any = listing_visible(sm->nodes[i].flags) != 0;
	}
	for (i = 0; i < sm->edges_size && !any; i++) {
		any = listing_visible(sm->edges[i].flags) != 0;
	}
	if (!sm->sm2 || (!listing_visible(sm->flags) && !any)) {
		return CYBERIADA_NO_ERROR;
	}

	printf(LISTING_STATE_MACHINE " %s\n", sm->sm2->nodes ? listing_string(sm->sm2->nodes->title) : "");

	for (i = 0; i < sm->nodes_size && res == CYBERIADA_NO_ERROR; i++) {
		const CyberiadaNode* node = sm->nodes[i].n2;
		if (listing_visible(sm->nodes[i].flags) && node && node->type != cybNodeRegion) {
			res = listing_print_node(listing, node, level);
		}
	}
	for (i = 0; i < sm->edges_size && res == CYBERIADA_NO_ERROR; i++) {
		const CyberiadaEdge* edge = sm->edges[i].e2;
		if (listing_visible(sm->edges[i].flags) && edge && edge->type != cybEdgeComment) {
			res = listing_print_transition(listing, edge, level);
		}
	}
	if (res == CYBERIADA_NO_ERROR && sm->sm2->nodes) {
		listing_print_links(listing, sm, sm->sm2->nodes->children, level);
	}
	return res;
}

static int listing_geometry_meta_name(const char* name)
{
	size_t i;
	for (i = 0; i < sizeof(listing_geometry_meta) / sizeof(*listing_geometry_meta); i++) {
		if (strcmp(name, listing_geometry_meta[i]) == 0) return 1;
	}
	return 0;
}

static void listing_print_meta(const CyberiadaDocumentDiff* diff)
{
	size_t i;
	int header = 0;
	for (i = 0; i < diff->meta_size; i++) {
		const CyberiadaMetaDiff* m = diff->meta + i;
		if (!m->name || listing_geometry_meta_name(m->name)) continue;
		if (!header) {
			puts(LISTING_META);
			header = 1;
		}
		if (m->value1) {
			listing_print_indent(1);
			printf("%s: %s\n", m->name, m->value1);
		}
		if (m->value2) {
			listing_print_indent(1);
			printf("%s: %s\n", m->name, m->value2);
		}
	}
}

static size_t listing_entries(const CyberiadaDocumentDiff* diff)
{
	size_t i, n = 0;
	for (i = 0; i < diff->sms_size; i++) {
		n += diff->sms[i].nodes_size + diff->sms[i].edges_size;
	}
	return n;
}

/* the words and the names of every element of the document: a repeated one gets its id */
static void listing_count_elements(Listing* listing, const CyberiadaDocumentDiff* diff)
{
	size_t i, j;
	ListingName name;
	for (i = 0; i < diff->sms_size; i++) {
		const CyberiadaSMDiff* sm = diff->sms + i;
		for (j = 0; j < sm->nodes_size; j++) {
			const CyberiadaNode* node = sm->nodes[j].n2;
			if (node && node->type != cybNodeRegion) {
				listing_node_name(node, &name);
				listing_count(listing, &name);
			}
		}
		for (j = 0; j < sm->edges_size; j++) {
			const CyberiadaEdge* edge = sm->edges[j].e2;
			if (edge && edge->type != cybEdgeComment) {
				listing_edge_name(edge, &name);
				listing_count(listing, &name);
			}
		}
	}
}

static int listing_print_document(CyberiadaDocument* doc)
{
	CyberiadaDocument empty;
	CyberiadaDocumentDiff diff;
	Listing listing;
	size_t i;
	int res;

	cyberiada_init_sm_document(&empty);
	res = cyberiada_diff_documents(&empty, doc, NULL, &diff);
	cyberiada_cleanup_sm_document(&empty);
	if (res != CYBERIADA_NO_ERROR) {
		return res;
	}

	memset(&listing, 0, sizeof(listing));
	listing.capacity = listing_entries(&diff);
	if (listing.capacity) {
		listing.keys = (ListingKey*)malloc(sizeof(ListingKey) * listing.capacity);
		listing.shown = (const CyberiadaNode**)malloc(sizeof(CyberiadaNode*) * listing.capacity);
		if (!listing.keys || !listing.shown) {
			free(listing.keys);
			free(listing.shown);
			cyberiada_cleanup_document_diff(&diff);
			return CYBERIADA_MEMORY_ERROR;
		}
	}
	listing_count_elements(&listing, &diff);

	for (i = 0; i < diff.sms_size && res == CYBERIADA_NO_ERROR; i++) {
		res = listing_print_sm(&listing, diff.sms + i);
	}
	if (res == CYBERIADA_NO_ERROR) {
		listing_print_meta(&diff);
	}

	free(listing.keys);
	free(listing.shown);
	cyberiada_cleanup_document_diff(&diff);
	return res;
}

static const char* listing_base_name(const char* path)
{
	const char* slash = strrchr(path, '/');
	const char* backslash = strrchr(path, '\\');
	if (backslash && (!slash || backslash > slash)) slash = backslash;
	return slash ? slash + 1 : path;
}

/* the file as it is, so that git still compares it */
static void listing_copy_file(FILE* file)
{
	char buffer[LISTING_COPY_BUFFER];
	size_t n;
	rewind(file);
	while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		fwrite(buffer, 1, n, stdout);
	}
}

int print_listing(const char* path, CyberiadaXMLFormat format)
{
	CyberiadaDocument doc;
	FILE* file;
	long size;
	int res;

	if (!path) {
		fprintf(stderr, "%s: no file given\n", LISTING_PROGRAM);
		return 1;
	}
	file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "%s: %s: cannot open the file\n", LISTING_PROGRAM, listing_base_name(path));
		return 2;
	}
	if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0) {
		fprintf(stderr, "%s: %s: cannot read the file\n", LISTING_PROGRAM, listing_base_name(path));
		fclose(file);
		return 2;
	}
	if (size == 0) {
		fclose(file);
		return 0;
	}

	cyberiada_init_sm_document(&doc);
	res = cyberiada_read_sm_document(&doc, path, format, CYBERIADA_FLAG_NO);
	if (res == CYBERIADA_NO_ERROR) {
		res = listing_print_document(&doc);
	}
	if (res != CYBERIADA_NO_ERROR) {
		fflush(stdout);
		fprintf(stderr, "%s: %s: the diagram is not read: %s\n", LISTING_PROGRAM, listing_base_name(path),
				cyberiada_error_str(res));
		listing_copy_file(file);
	}
	fclose(file);
	cyberiada_cleanup_sm_document(&doc);
	return 0;
}
