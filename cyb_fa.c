/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The EXT-FA rules: deterministic finite automaton documents (type/ fa)
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include "cyb_fa.h"
#include "cyb_meta.h"
#include "cyb_regex.h"
#include "cyb_error.h"

/* the alphabet grammar (FA 4): the escapable and the forbidden characters */
#define FA_ESCAPABLE           ".[]-{},?()|\\"
#define FA_FORBIDDEN           "*+^$"
/* the ERE characters escaped outside a bracket expression */
#define FA_ERE_SPECIAL         ".[()*+?|^$\\"
/* '.' - one printable non-space character: an ASCII byte or a UTF-8 sequence */
#define FA_ANY_CHAR            "([^[:space:]\x80-\xff]|[\xc2-\xf4][\x80-\xbf]{1,3})"
#define FA_PATTERN_MAX         4096
#define FA_ANY_NODE            0

/* the words a symbol cannot be: the reserved words of 6.8.1 and the reserved
   event names of the standard and of the extension (FA 3.4) */
static const char* fa_reserved_symbols[] = {
	"entry", "exit", "do", "propagate", "block", "defer", "else",
	"ANY", "UNKNOWN", CYBERIADA_FA_EOF
};

static const char* fa_input_titles[] = {
	CYBERIADA_FA_INPUT_TITLE, CYBERIADA_FA_INPUT_RE_TITLE
};
static const char* fa_output_titles[] = {
	CYBERIADA_FA_OUTPUT_TITLE, CYBERIADA_FA_OUTPUT_RE_TITLE
};

#define FA_STATE_TYPES (cybNodeSimpleState | cybNodeCompositeState)
#define FA_NODE_TYPES  (FA_STATE_TYPES | cybNodeRegion | cybNodeComment | \
						cybNodeFormalComment | cybNodeInitial | cybNodeFinal)

/* -----------------------------------------------------------------------------
 * Strings and symbols
 * ----------------------------------------------------------------------------- */

static int fa_title_in(const char* title, const char** titles, size_t count)
{
	size_t i;
	if (!title) {
		return 0;
	}
	for (i = 0; i < count; i++) {
		if (strcmp(title, titles[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

static int fa_symbol_reserved(const char* symbol)
{
	return fa_title_in(symbol, fa_reserved_symbols,
					   sizeof(fa_reserved_symbols) / sizeof(const char*));
}

static int fa_word_char(unsigned char c)
{
	return c == '_' || isalnum(c) || c >= 0x80;
}

/* the shape of a symbol: the event name pattern of the library, \w((\w| |\.)*\w)? */
static int fa_symbol_ok(const char* symbol)
{
	size_t i, n = strlen(symbol);
	if (n == 0 || !fa_word_char((unsigned char)symbol[0]) ||
		!fa_word_char((unsigned char)symbol[n - 1])) {
		return 0;
	}
	for (i = 1; i + 1 < n; i++) {
		unsigned char c = (unsigned char)symbol[i];
		if (!fa_word_char(c) && c != ' ' && c != '.') {
			return 0;
		}
	}
	return 1;
}

/* trim both ends in place, return the start */
static char* fa_trim(char* s)
{
	size_t n;
	while (*s && isspace((unsigned char)*s)) {
		s++;
	}
	n = strlen(s);
	while (n > 0 && isspace((unsigned char)s[n - 1])) {
		s[--n] = 0;
	}
	return s;
}

static char* fa_trimmed_copy(const char* s)
{
	char* copy;
	char* start;
	if (!s) {
		s = "";
	}
	copy = (char*)malloc(strlen(s) + 1);
	if (!copy) {
		return NULL;
	}
	strcpy(copy, s);
	start = fa_trim(copy);
	if (start != copy) {
		memmove(copy, start, strlen(start) + 1);
	}
	return copy;
}

/* -----------------------------------------------------------------------------
 * The restricted regular expressions -> POSIX ERE (FA 4)
 * ----------------------------------------------------------------------------- */

typedef struct {
	const char*  expr;
	size_t       pos;
	char*        out;
	size_t       len;
	const char*  error;      /* the first error, NULL while translating */
	size_t       error_pos;
} FaTranslator;

static void fa_fail(FaTranslator* t, const char* message, size_t pos)
{
	if (!t->error) {
		t->error = message;
		t->error_pos = pos;
	}
}

static void fa_emit(FaTranslator* t, const char* s)
{
	size_t n = strlen(s);
	if (t->len + n + 1 > FA_PATTERN_MAX) {
		fa_fail(t, "pattern too long", t->pos);
		return;
	}
	memcpy(t->out + t->len, s, n + 1);
	t->len += n;
}

static void fa_emit_char(FaTranslator* t, char c)
{
	char buf[2] = { c, 0 };
	fa_emit(t, buf);
}

/* a literal character outside a bracket expression */
static void fa_emit_literal(FaTranslator* t, char c)
{
	if (c == '{' || c == '}' || c == ']') {
		fa_emit_char(t, '[');
		fa_emit_char(t, c);
		fa_emit_char(t, ']');
	} else {
		if (strchr(FA_ERE_SPECIAL, c)) {
			fa_emit_char(t, '\\');
		}
		fa_emit_char(t, c);
	}
}

static int fa_escaped(FaTranslator* t, char* c)
{
	char next = t->expr[t->pos + 1];
	if (!next || !strchr(FA_ESCAPABLE, next)) {
		fa_fail(t, "unknown escape", t->pos);
		return 0;
	}
	*c = next;
	return 1;
}

static void fa_sequence(FaTranslator* t, int top);

static void fa_group(FaTranslator* t)
{
	size_t start = t->pos - 1;
	size_t before;
	fa_emit_char(t, '(');
	for (;;) {
		before = t->len;
		fa_sequence(t, 0);
		if (t->error) {
			return;
		}
		if (t->len == before) {
			fa_fail(t, "empty alternative", t->pos);
			return;
		}
		if (!t->expr[t->pos]) {
			fa_fail(t, "unbalanced '('", start);
			return;
		}
		if (t->expr[t->pos++] == ')') {
			fa_emit_char(t, ')');
			return;
		}
		fa_emit_char(t, '|');
	}
}

/* one character of a bracket expression */
static int fa_class_item(FaTranslator* t, char* c)
{
	char ch = t->expr[t->pos];
	if (ch == '\\') {
		if (!fa_escaped(t, c)) {
			return 0;
		}
		t->pos += 2;
		return 1;
	}
	if (ch == '[' || ch == '^' || ch == '-') {
		fa_fail(t, "the character must be escaped", t->pos);
		return 0;
	}
	*c = ch;
	t->pos += 1;
	return 1;
}

/* a bracket expression: ']' first, '-' last, '^' never first, '\' doubled */
static void fa_class(FaTranslator* t)
{
	size_t start = t->pos - 1;
	char items[FA_PATTERN_MAX];
	size_t n = 0;
	int bracket = 0, dash = 0;
	char low, high;
	while (t->expr[t->pos] && t->expr[t->pos] != ']') {
		if (!fa_class_item(t, &low)) {
			return;
		}
		if (t->expr[t->pos] == '-' && t->expr[t->pos + 1] && t->expr[t->pos + 1] != ']') {
			t->pos++;
			if (!fa_class_item(t, &high)) {
				return;
			}
			if ((unsigned char)high < (unsigned char)low) {
				fa_fail(t, "range out of order", t->pos - 1);
				return;
			}
			if (n + 3 >= sizeof(items)) {
				fa_fail(t, "pattern too long", t->pos);
				return;
			}
			items[n++] = low;
			items[n++] = '-';
			items[n++] = high;
		} else if (low == ']') {
			bracket = 1;
		} else if (low == '-') {
			dash = 1;
		} else {
			if (n + 2 >= sizeof(items)) {
				fa_fail(t, "pattern too long", t->pos);
				return;
			}
			if (low == '\\') {
				items[n++] = '\\';
			}
			items[n++] = low;
		}
	}
	if (!t->expr[t->pos]) {
		fa_fail(t, "unbalanced '['", start);
		return;
	}
	t->pos++;
	if (n == 0 && !bracket && !dash) {
		fa_fail(t, "empty character class", start);
		return;
	}
	if (n == 1 && items[0] == '^' && !bracket && !dash) {
		fa_emit(t, "\\^");
		return;
	}
	items[n] = 0;
	fa_emit_char(t, '[');
	if (bracket) {
		fa_emit_char(t, ']');
	}
	if (n > 0 && items[0] == '^' && !bracket) {
		/* a leading '^' negates: put the last item first */
		fa_emit_char(t, items[n - 1]);
		items[n - 1] = 0;
	}
	fa_emit(t, items);
	if (dash) {
		fa_emit_char(t, '-');
	}
	fa_emit_char(t, ']');
}

static void fa_sequence(FaTranslator* t, int top)
{
	int quantifiable = 0;
	char c;
	while (t->expr[t->pos] && !t->error) {
		c = t->expr[t->pos];
		if (c == ')' || c == '|') {
			if (top) {
				fa_fail(t, "alternative or ')' outside a group", t->pos);
			}
			return;
		}
		if (strchr(FA_FORBIDDEN, c)) {
			fa_fail(t, "the construct is not admissible", t->pos);
			return;
		}
		if (c == '?') {
			if (!quantifiable) {
				fa_fail(t, "'?' follows nothing", t->pos);
				return;
			}
			fa_emit_char(t, '?');
			quantifiable = 0;
			t->pos++;
		} else if (c == '{') {
			size_t start = t->pos, low = 0, high = 0, digits = 0;
			if (!quantifiable) {
				fa_fail(t, "count follows nothing", t->pos);
				return;
			}
			t->pos++;
			while (isdigit((unsigned char)t->expr[t->pos])) {
				low = low * 10 + (size_t)(t->expr[t->pos++] - '0');
				digits++;
			}
			if (!digits) {
				fa_fail(t, "count {n} or {n,m} expected", start);
				return;
			}
			if (t->expr[t->pos] == ',') {
				t->pos++;
				digits = 0;
				while (isdigit((unsigned char)t->expr[t->pos])) {
					high = high * 10 + (size_t)(t->expr[t->pos++] - '0');
					digits++;
				}
				if (!digits) {
					fa_fail(t, "count {n} or {n,m} expected", start);
					return;
				}
				if (high < low) {
					fa_fail(t, "count {n,m} with m < n", start);
					return;
				}
			}
			if (t->expr[t->pos] != '}') {
				fa_fail(t, "count {n} or {n,m} expected", start);
				return;
			}
			t->pos++;
			{
				char buf[64];
				size_t n = t->pos - start;
				if (n >= sizeof(buf)) {
					fa_fail(t, "count too long", start);
					return;
				}
				memcpy(buf, t->expr + start, n);
				buf[n] = 0;
				fa_emit(t, buf);
			}
			quantifiable = 0;
		} else if (c == ']' || c == '}' || c == '-' || c == ',') {
			fa_fail(t, "the character must be escaped", t->pos);
			return;
		} else if (c == '\\') {
			if (!fa_escaped(t, &c)) {
				return;
			}
			fa_emit_literal(t, c);
			quantifiable = 1;
			t->pos += 2;
		} else if (c == '.') {
			fa_emit(t, FA_ANY_CHAR);
			quantifiable = 1;
			t->pos++;
		} else if (c == '[') {
			t->pos++;
			fa_class(t);
			quantifiable = 1;
		} else if (c == '(') {
			t->pos++;
			fa_group(t);
			quantifiable = 1;
		} else {
			fa_emit_literal(t, c);
			quantifiable = 1;
			t->pos++;
		}
	}
}

/* translate a restricted expression into an anchored ERE, 0 on an error */
static int fa_translate(const char* expr, char* out, const char** error, size_t* error_pos)
{
	FaTranslator t;
	t.expr = expr;
	t.pos = 0;
	t.out = out;
	t.len = 0;
	t.error = NULL;
	t.error_pos = 0;
	out[0] = 0;
	if (!*expr) {
		fa_fail(&t, "empty expression", 0);
	} else {
		fa_emit(&t, "^(");
		fa_sequence(&t, 1);
		fa_emit(&t, ")$");
	}
	*error = t.error;
	*error_pos = t.error_pos;
	return t.error == NULL;
}

/* -----------------------------------------------------------------------------
 * The alphabets (FA 4)
 * ----------------------------------------------------------------------------- */

typedef struct {
	const CyberiadaNode* node;
	int                  regex;
	size_t               count;
	char**               symbols;    /* the plain lines */
	regex_t*             patterns;   /* the compiled lines of a _RE comment */
} FaAlphabet;

static void fa_alphabet_free(FaAlphabet* a)
{
	size_t i;
	if (a->symbols) {
		for (i = 0; i < a->count; i++) {
			free(a->symbols[i]);
		}
		free(a->symbols);
	}
	if (a->patterns) {
		for (i = 0; i < a->count; i++) {
			cyb_regfree(a->patterns + i);
		}
		free(a->patterns);
	}
	memset(a, 0, sizeof(FaAlphabet));
}

/* parse the body: one symbol or pattern per line, no blank line inside */
static int fa_alphabet_parse(FaAlphabet* a, const CyberiadaNode* node)
{
	const char* title = node->title;
	const char* body = node->comment_data ? node->comment_data->body : NULL;
	char* copy;
	char* line;
	char* next;
	char** lines;
	size_t n = 0, i, first = 0, last = 0, capacity = 1;
	char pattern[FA_PATTERN_MAX];
	const char* error;
	size_t error_pos;

	memset(a, 0, sizeof(FaAlphabet));
	a->node = node;
	a->regex = strlen(title) > 3 && strcmp(title + strlen(title) - 3, "_RE") == 0;
	copy = fa_trimmed_copy(body);
	if (!copy) {
		return CYBERIADA_MEMORY_ERROR;
	}
	for (line = copy; *line; line++) {
		if (*line == '\n') {
			capacity++;
		}
	}
	lines = (char**)malloc(capacity * sizeof(char*));
	if (!lines) {
		free(copy);
		return CYBERIADA_MEMORY_ERROR;
	}
	for (line = copy; line; line = next) {
		next = strchr(line, '\n');
		if (next) {
			*next++ = 0;
		}
		lines[n++] = fa_trim(line);
	}
	/* the body was trimmed, so the first and the last lines are filled */
	if (n == 0 || !*lines[0]) {
		ERROR("FA: the alphabet %s is empty\n", title);
		free(lines);
		free(copy);
		return CYBERIADA_FA_ERROR;
	}
	last = n - 1;
	a->count = last - first + 1;
	a->symbols = (char**)calloc(a->count, sizeof(char*));
	if (a->regex) {
		a->patterns = (regex_t*)calloc(a->count, sizeof(regex_t));
	}
	if (!a->symbols || (a->regex && !a->patterns)) {
		free(lines);
		free(copy);
		return CYBERIADA_MEMORY_ERROR;
	}
	for (i = first; i <= last; i++) {
		if (!*lines[i]) {
			ERROR("FA: empty line %lu in the alphabet %s\n", (unsigned long)(i + 1), title);
			free(lines);
			free(copy);
			return CYBERIADA_FA_ERROR;
		}
		a->symbols[i - first] = (char*)malloc(strlen(lines[i]) + 1);
		if (!a->symbols[i - first]) {
			free(lines);
			free(copy);
			return CYBERIADA_MEMORY_ERROR;
		}
		strcpy(a->symbols[i - first], lines[i]);
		if (a->regex) {
			if (!fa_translate(lines[i], pattern, &error, &error_pos)) {
				ERROR("FA: alphabet %s line %lu: %s (column %lu)\n", title,
					  (unsigned long)(i + 1), error, (unsigned long)(error_pos + 1));
				free(lines);
				free(copy);
				return CYBERIADA_FA_ERROR;
			}
			if (cyb_regcomp(a->patterns + (i - first), pattern)) {
				ERROR("FA: alphabet %s line %lu: the pattern cannot be compiled\n", title,
					  (unsigned long)(i + 1));
				/* the failed slot must not be freed */
				a->count = i - first;
				free(lines);
				free(copy);
				return CYBERIADA_FA_ERROR;
			}
		} else if (fa_symbol_reserved(lines[i])) {
			ERROR("FA: the reserved name %s is a symbol of the alphabet %s\n", lines[i], title);
			free(lines);
			free(copy);
			return CYBERIADA_FA_ERROR;
		}
	}
	free(lines);
	free(copy);
	return CYBERIADA_NO_ERROR;
}

static int fa_alphabet_contains(const FaAlphabet* a, const char* symbol)
{
	size_t i;
	if (fa_symbol_reserved(symbol)) {
		return 0;
	}
	for (i = 0; i < a->count; i++) {
		if (a->regex) {
			if (cyb_regexec(a->patterns + i, symbol, 0, NULL, 0) == 0) {
				return 1;
			}
		} else if (strcmp(a->symbols[i], symbol) == 0) {
			return 1;
		}
	}
	return 0;
}

/* a symbol belongs to the alphabet when one is declared */
static int fa_check_member(const FaAlphabet* a, const char* symbol, const char* where,
						   const char* id)
{
	if (a->node && !fa_alphabet_contains(a, symbol)) {
		ERROR("FA: the %s %s of %s is not in the alphabet %s\n", where, symbol, id,
			  a->node->title);
		return CYBERIADA_FA_ERROR;
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The nodes (FA 3, 3.1-3.3, 4.1)
 * ----------------------------------------------------------------------------- */

static int fa_region_count(const CyberiadaNode* state)
{
	const CyberiadaNode* n;
	int count = 0;
	for (n = state->children; n; n = n->next) {
		if (n->type == cybNodeRegion) {
			count++;
		}
	}
	return count;
}

static int fa_composite_has_initial(const CyberiadaNode* state)
{
	const CyberiadaNode *region, *n;
	for (region = state->children; region; region = region->next) {
		if (region->type != cybNodeRegion) {
			continue;
		}
		for (n = region->children; n; n = n->next) {
			if (n->type == cybNodeInitial) {
				return 1;
			}
		}
	}
	return 0;
}

/* the output symbol of a state: at most one one-line entry action (FA 3.1) */
static int fa_check_state_actions(const CyberiadaNode* state, const FaAlphabet* outputs)
{
	const CyberiadaAction* a;
	char* symbol;
	int res;
	for (a = state->actions; a; a = a->next) {
		if (a->type != cybActionEntry) {
			ERROR("FA: state %s has a behaviour block other than entry/\n", state->id);
			return CYBERIADA_FA_ERROR;
		}
		if (state->type == cybNodeCompositeState) {
			ERROR("FA: composite state %s has an entry/ block\n", state->id);
			return CYBERIADA_FA_ERROR;
		}
		symbol = fa_trimmed_copy(a->behavior);
		if (!symbol) {
			return CYBERIADA_MEMORY_ERROR;
		}
		if (!fa_symbol_ok(symbol)) {
			ERROR("FA: the entry/ block of state %s is not one output symbol\n", state->id);
			free(symbol);
			return CYBERIADA_FA_ERROR;
		}
		if (fa_symbol_reserved(symbol)) {
			ERROR("FA: the reserved name %s is the output symbol of state %s\n", symbol, state->id);
			free(symbol);
			return CYBERIADA_FA_ERROR;
		}
		res = fa_check_member(outputs, symbol, "output symbol", state->id);
		free(symbol);
		if (res != CYBERIADA_NO_ERROR) {
			return res;
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* walk the sibling list; parent is the enclosing state, NULL at the top level */
static int fa_check_nodes(const CyberiadaNode* nodes, const CyberiadaNode* parent,
						  const FaAlphabet* outputs)
{
	const CyberiadaNode* n;
	int res;
	for (n = nodes; n; n = n->next) {
		if (!(n->type & FA_NODE_TYPES)) {
			ERROR("FA: node %s of type %d is not admissible in an FA diagram\n", n->id, (int)n->type);
			return CYBERIADA_FA_ERROR;
		}
		if (n->type == cybNodeRegion) {
			if ((res = fa_check_nodes(n->children, parent, outputs)) != CYBERIADA_NO_ERROR) {
				return res;
			}
			continue;
		}
		if (n->type == cybNodeFinal && parent) {
			ERROR("FA: final state %s inside composite state %s\n", n->id, parent->id);
			return CYBERIADA_FA_ERROR;
		}
		if (n->type == cybNodeFormalComment && parent &&
			(fa_title_in(n->title, fa_input_titles, 2) || fa_title_in(n->title, fa_output_titles, 2))) {
			ERROR("FA: alphabet comment %s inside state %s, the top level expected\n", n->id, parent->id);
			return CYBERIADA_FA_ERROR;
		}
		if (n->type & FA_STATE_TYPES) {
			if (n->collapsed_flag) {
				ERROR("FA: state %s has a collapsed decomposition block\n", n->id);
				return CYBERIADA_FA_ERROR;
			}
			if (n->type == cybNodeCompositeState && fa_region_count(n) != 1) {
				ERROR("FA: composite state %s has %d regions\n", n->id, fa_region_count(n));
				return CYBERIADA_FA_ERROR;
			}
			if ((res = fa_check_state_actions(n, outputs)) != CYBERIADA_NO_ERROR) {
				return res;
			}
			if ((res = fa_check_nodes(n->children, n, outputs)) != CYBERIADA_NO_ERROR) {
				return res;
			}
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* one initial pseudostate at the top level (FA 3.2) */
static int fa_check_top_initial(const CyberiadaSM* sm)
{
	const CyberiadaNode* n;
	int count = 0;
	for (n = sm->nodes->children; n; n = n->next) {
		if (n->type == cybNodeInitial) {
			count++;
		}
	}
	if (count != 1) {
		ERROR("FA: state machine %s has %d top-level initial pseudostates\n", sm->nodes->id, count);
		return CYBERIADA_FA_ERROR;
	}
	return CYBERIADA_NO_ERROR;
}

/* the alphabet comments: at most one of each kind at the top level (FA 4) */
static int fa_find_alphabets(const CyberiadaSM* sm, FaAlphabet* inputs, FaAlphabet* outputs)
{
	const CyberiadaNode* n;
	FaAlphabet* target;
	int res;
	for (n = sm->nodes->children; n; n = n->next) {
		if (n->type != cybNodeFormalComment) {
			continue;
		}
		if (fa_title_in(n->title, fa_input_titles, 2)) {
			target = inputs;
		} else if (fa_title_in(n->title, fa_output_titles, 2)) {
			target = outputs;
		} else {
			continue;
		}
		if (target->node) {
			ERROR("FA: second alphabet comment %s (%s) after %s (%s)\n", n->id, n->title,
				  target->node->id, target->node->title);
			return CYBERIADA_FA_ERROR;
		}
		if ((res = fa_alphabet_parse(target, n)) != CYBERIADA_NO_ERROR) {
			return res;
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The transitions (FA 3.2-3.4)
 * ----------------------------------------------------------------------------- */

static int fa_check_edges(const CyberiadaSM* sm, const FaAlphabet* inputs, const FaAlphabet* outputs)
{
	const CyberiadaEdge *e, *e2;
	const CyberiadaNode *src, *dst;
	char* trigger;
	char* symbol;
	int is_eof, to_final, res;

	for (e = sm->edges; e; e = e->next) {
		if (e->type == cybEdgeComment) {
			continue;
		}
		src = e->source;
		dst = e->target;
		if (!src || !dst) {
			continue;   /* an unresolved end is reported by the reconstruction */
		}
		to_final = dst->type == cybNodeFinal;
		if (src->type == cybNodeInitial) {
			if (e->action) {
				ERROR("FA: transition %s from the initial pseudostate carries a label\n", e->id);
				return CYBERIADA_FA_ERROR;
			}
			if (to_final) {
				ERROR("FA: transition %s into the final state %s carries no EOF\n", e->id, dst->id);
				return CYBERIADA_FA_ERROR;
			}
			continue;
		}
		if (!(src->type & FA_STATE_TYPES)) {
			continue;   /* the node kinds are checked by fa_check_nodes */
		}
		if (!e->action) {
			ERROR("FA: transition %s carries no symbol\n", e->id);
			return CYBERIADA_FA_ERROR;
		}
		trigger = fa_trimmed_copy(e->action->trigger);
		if (!trigger) {
			return CYBERIADA_MEMORY_ERROR;
		}
		res = CYBERIADA_NO_ERROR;
		is_eof = strcmp(trigger, CYBERIADA_FA_EOF) == 0;
		if (!*trigger) {
			ERROR("FA: transition %s carries no symbol\n", e->id);
			res = CYBERIADA_FA_ERROR;
		} else if (e->action->guard && *e->action->guard) {
			ERROR("FA: transition %s carries a guard\n", e->id);
			res = CYBERIADA_FA_ERROR;
		} else if (e->action->propagation != cybEventPropagationNone) {
			ERROR("FA: transition %s carries an event parameter\n", e->id);
			res = CYBERIADA_FA_ERROR;
		} else if (!is_eof && !fa_symbol_ok(trigger)) {
			ERROR("FA: the event %s of transition %s is not a symbol\n", trigger, e->id);
			res = CYBERIADA_FA_ERROR;
		} else if (!is_eof && fa_symbol_reserved(trigger)) {
			ERROR("FA: the reserved name %s is the symbol of transition %s\n", trigger, e->id);
			res = CYBERIADA_FA_ERROR;
		} else if (is_eof != to_final) {
			if (is_eof) {
				ERROR("FA: EOF transition %s targets %s, not a final state\n", e->id, dst->id);
			} else {
				ERROR("FA: transition %s into the final state %s carries no EOF\n", e->id, dst->id);
			}
			res = CYBERIADA_FA_ERROR;
		} else if (!is_eof) {
			res = fa_check_member(inputs, trigger, "symbol", e->id);
		}
		if (res == CYBERIADA_NO_ERROR && e->action->behavior && *e->action->behavior) {
			symbol = fa_trimmed_copy(e->action->behavior);
			if (!symbol) {
				free(trigger);
				return CYBERIADA_MEMORY_ERROR;
			}
			if (!fa_symbol_ok(symbol)) {
				ERROR("FA: the action of transition %s is not one output symbol\n", e->id);
				res = CYBERIADA_FA_ERROR;
			} else if (fa_symbol_reserved(symbol)) {
				ERROR("FA: the reserved name %s is the output symbol of transition %s\n", symbol, e->id);
				res = CYBERIADA_FA_ERROR;
			} else {
				res = fa_check_member(outputs, symbol, "output symbol", e->id);
			}
			free(symbol);
		}
		if (res == CYBERIADA_NO_ERROR && dst->type == cybNodeCompositeState &&
			!fa_composite_has_initial(dst)) {
			ERROR("FA: composite state %s is entered by transition %s but contains no initial pseudostate\n",
				  dst->id, e->id);
			res = CYBERIADA_FA_ERROR;
		}
		/* determinism: one own transition per symbol (FA 3.1.1) */
		for (e2 = e->next; res == CYBERIADA_NO_ERROR && e2; e2 = e2->next) {
			if (e2->type == cybEdgeComment || e2->source != src || !e2->action) {
				continue;
			}
			symbol = fa_trimmed_copy(e2->action->trigger);
			if (!symbol) {
				free(trigger);
				return CYBERIADA_MEMORY_ERROR;
			}
			if (strcmp(symbol, trigger) == 0) {
				ERROR("FA: transitions %s and %s leave state %s by the same symbol %s\n",
					  e->id, e2->id, src->id, trigger);
				res = CYBERIADA_FA_ERROR;
			}
			free(symbol);
		}
		free(trigger);
		if (res != CYBERIADA_NO_ERROR) {
			return res;
		}
	}
	return CYBERIADA_NO_ERROR;
}

/* -----------------------------------------------------------------------------
 * The document (FA 5)
 * ----------------------------------------------------------------------------- */

int cyberiada_document_is_fa(const CyberiadaDocument* doc)
{
	const char* type;
	char* value;
	int result;
	if (!doc || !doc->meta_info) {
		return 0;
	}
	type = cyberiada_find_meta_string((CyberiadaMetainformation*)doc->meta_info, CYBERIADA_META_TYPE);
	if (!type) {
		return 0;
	}
	value = fa_trimmed_copy(type);
	if (!value) {
		return 0;
	}
	result = strcmp(value, CYBERIADA_META_TYPE_FA) == 0;
	free(value);
	return result;
}

int cyberiada_check_fa(const CyberiadaDocument* doc)
{
	const CyberiadaSM* sm;
	FaAlphabet inputs, outputs;
	int res = CYBERIADA_NO_ERROR;

	if (!doc || !doc->state_machines) {
		return CYBERIADA_BAD_PARAMETER;
	}
	if (doc->state_machines->next) {
		ERROR("FA: the document contains more than one state machine\n");
		return CYBERIADA_FA_ERROR;
	}
	if (doc->meta_info->event_propagation_flag == cybEventPropagationPropagate) {
		ERROR("FA: eventPropagation is propagate, block expected\n");
		return CYBERIADA_FA_ERROR;
	}
	sm = doc->state_machines;
	if (!sm->nodes) {
		return CYBERIADA_NO_ERROR;
	}
	memset(&inputs, 0, sizeof(FaAlphabet));
	memset(&outputs, 0, sizeof(FaAlphabet));
	if ((res = fa_find_alphabets(sm, &inputs, &outputs)) == CYBERIADA_NO_ERROR &&
		(res = fa_check_nodes(sm->nodes->children, NULL, &outputs)) == CYBERIADA_NO_ERROR &&
		(res = fa_check_top_initial(sm)) == CYBERIADA_NO_ERROR) {
		res = fa_check_edges(sm, &inputs, &outputs);
	}
	fa_alphabet_free(&inputs);
	fa_alphabet_free(&outputs);
	return res;
}
