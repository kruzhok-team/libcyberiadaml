# The Cyberiada GraphML Library API Reference

The public API is the installed header:

```c
#include <cyberiada/cyberiadaml.h>
```

The isomorphism check and diff API lives in the companion installed header
`cyberiadaml_iso.h`, included automatically by `cyberiadaml.h`.

It depends on `<cyberiada/htgeom.h>` from libhtreegeom for the geometry types.
All functions return `int` error codes (see [Error codes](#error-codes)) unless
stated otherwise; constructors return a pointer or `NULL` on failure.

## Data model

```
 CyberiadaDocument
   ├── format                  (dialect string, e.g. "Cyberiada-GraphML-1.0")
   ├── meta_info               -> CyberiadaMetainformation
   ├── geometry_format         (none / short / full)
   ├── node/edge/edge_pl coord formats + edge geometry format
   ├── bounding_rect
   └── state_machines          -> CyberiadaSM (list)
         ├── nodes             -> CyberiadaNode (tree: parent/children/next)
         │     ├── type, id, title, formal_title
         │     ├── actions     -> CyberiadaAction (list)   [states]
         │     ├── comment_data-> CyberiadaCommentData     [comments]
         │     ├── link        -> CyberiadaLink            [submachine states]
         │     └── geometry_point | geometry_rect, color, collapsed_flag
         └── edges             -> CyberiadaEdge (list)
               ├── type, id, source_id/target_id (+ resolved source/target)
               ├── action      -> CyberiadaAction          [transitions]
               ├── comment_subject -> CyberiadaCommentSubject [comment edges]
               └── label / polyline / source & target points, color
```

Every string field `s` is paired with a length field `s_len` and must be set
with `cyberiada_copy_string()`. The document content strings (titles,
actions, comments) have no length limit.

### Node types (`CyberiadaNodeType`)

Bit-flag values, so the same constants form a `CyberiadaNodeTypeMask`:
`cybNodeSM` (0), `cybNodeSimpleState`, `cybNodeCompositeState`,
`cybNodeRegion`, `cybNodeSubmachineState`, `cybNodeComment`,
`cybNodeFormalComment`, `cybNodeInitial`, `cybNodeFinal`, `cybNodeChoice`,
`cybNodeTerminate`, `cybNodeEntryPoint`, `cybNodeExitPoint`,
`cybNodeShallowHistory`, `cybNodeDeepHistory`, `cybNodeFork`, `cybNodeJoin`.

### Edge types (`CyberiadaEdgeType`)

`cybEdgeLocalTransition`, `cybEdgeExternalTransition`, `cybEdgeComment`.

### Action types (`CyberiadaActionType`)

`cybActionTransition`, `cybActionEntry`, `cybActionExit`, `cybActionDo`.
An action is `trigger [guard] keyword / behavior`; a state owns a list of
entry/exit/do actions, a transition owns a list of transition actions. The
optional event handling keyword before the slash — `propagate`, `block` or
`defer` (PNST 1044-2025, 6.8) — is stored in the `propagation` field as a
`CyberiadaEventPropagation` value and written back on export.

### Metainformation (`CyberiadaMetainformation`)

`standard_version` (must be `"1.0"`, `CYBERIADA_STANDARD_VERSION_CYBERIADAML`),
`transition_order_flag` (0 unset / 1 transition first / 2 exit first),
`event_propagation_flag` (0 unset / 1 block / 2 propagate) and a generic
`strings` name/value list for everything else. The names of the standard
parameters are provided as `CYBERIADA_META_*` constants.

### Finite automaton documents (EXT-FA)

A document whose metainformation carries `type/ fa` (`CYBERIADA_META_TYPE`,
`CYBERIADA_META_TYPE_FA`) is a deterministic finite automaton diagram of the
ПРИМС:КА extension (compat-tests testing spec 2.9). `cyberiada_document_is_fa`
tells. With `CYBERIADA_FLAG_STRICT` such a document is checked against the
EXT-FA rules on every read and write; without it an improper automaton (an
editor's draft, FA-extension 0.4 §5) is read and written as it is, and
`cyberiada_check_fa_document` lists its violations. Executors and code
generators read with `CYBERIADA_FLAG_STRICT`. The rules: one state machine,
`eventPropagation` absent or `block`, states / one-region composites / initial
/ final / transitions / comments only, one `entry/` output symbol per simple
state, one symbol per transition without guard or event parameter and at most
one output symbol as the action, `EOF` (`CYBERIADA_FA_EOF`) transitions into
the final states only, one own transition per symbol, the alphabets
`FA_INPUT`/`FA_INPUT_RE` and `FA_OUTPUT`/`FA_OUTPUT_RE`
(`CYBERIADA_FA_*_TITLE`) as top-level formal comments, one symbol or restricted
regular expression per line. A violation of a strict read or write returns
`CYBERIADA_FA_ERROR`. An input symbol has the shape of an event name of the
library, `\w((\w| |\.)*\w)?`; an output symbol is one line of any characters
(`.-`, `/`), not an event name with a slash (a second event block); the `_RE`
lines use the constructs `.`, `[a-z]`, `{n}`, `{n,m}`, `?`, `(a|b)` and the `\`
escapes, translated to POSIX ERE (`.` matches one UTF-8 character).

### File formats (`CyberiadaXMLFormat`)

* `cybxmlCyberiada10` — Cyberiada-GraphML 1.0;
* `cybxmlYED` — the legacy yEd-based Berloga/Ostranna format;
* `cybxmlLapki` — the Lapki IDE dialect of Cyberiada-GraphML 1.0 for lapki-client
  and lapki-compiler (write; read as `cybxmlCyberiada10`); needs the `platform`
  meta parameter;
* `cybxmlUnknown` — auto-detect on read (not valid for write).

### Geometry types

`CyberiadaPoint`, `CyberiadaRect`, `CyberiadaPolyline`,
`CyberiadaGeometryCoordFormat` and `CyberiadaGeometryEdgeFormat` are typedefs
of the libhtreegeom types (`HTreePoint`, `HTreeRect`, `HTreePolyline`,
`HTCoordFormat`, `HTEdgeFormat`). `CyberiadaGeometryFormat` describes the
document geometry mode: `cybgeomNone`, `cybgeomShort`, `cybgeomFull`.

## Import/export flags

The `flags` argument of the read/write/decode/encode functions is a bitfield.

Geometry conversion (import only; at most one flag per group):

| Group | Flags |
|---|---|
| node coordinates | `CYBERIADA_FLAG_NODES_ABSOLUTE_GEOMETRY`, `..._NODES_LEFTTOP_LOCAL_GEOMETRY`, `..._NODES_CENTER_LOCAL_GEOMETRY` |
| edge source/target points | `CYBERIADA_FLAG_EDGES_ABSOLUTE_GEOMETRY`, `..._EDGES_LEFTTOP_LOCAL_GEOMETRY`, `..._EDGES_CENTER_LOCAL_GEOMETRY` |
| edge polylines | `CYBERIADA_FLAG_EDGES_PL_ABSOLUTE_GEOMETRY`, `..._EDGES_PL_LEFTTOP_LOCAL_GEOMETRY`, `..._EDGES_PL_CENTER_LOCAL_GEOMETRY` |
| edge point placement | `CYBERIADA_FLAG_CENTER_EDGE_GEOMETRY`, `CYBERIADA_FLAG_BORDER_EDGE_GEOMETRY` |

Unset groups default to center-local coordinates and border edge placement.
On export the conversion flags are rejected (`CYBERIADA_BAD_PARAMETER`) — the
target format dictates the output geometry.

Geometry processing:

* `CYBERIADA_FLAG_RECONSTRUCT_GEOMETRY` — reconstruct absent node/edge
  geometry on import;
* `CYBERIADA_FLAG_RECONSTRUCT_SM_GEOMETRY` — also reconstruct the SM geometry;
* `CYBERIADA_FLAG_RECONSTRUCT_FULL_GEOMETRY` — reconstruct the full geometry;
* `CYBERIADA_FLAG_SKIP_GEOMETRY` — drop geometry on import/export;
* `CYBERIADA_FLAG_SHRINK_GEOMETRY` — shrink geometry on import/export;
* `CYBERIADA_FLAG_ROUND_GEOMETRY` — round exported coordinates to 0.001.

Non-geometry:

* `CYBERIADA_FLAG_FLATTENED` — treat the document as flattened (single-line);
* `CYBERIADA_FLAG_CHECK_INITIAL` — require an initial pseudostate on the top
  level;
* `CYBERIADA_FLAG_STRICT_ACTION_ENTRIES` — require unique entry/exit action
  entries;
* `CYBERIADA_FLAG_SKIP_EMPTY_BEHAVIOR` — skip actions with empty behavior;
* `CYBERIADA_FLAG_SIMPLIFY_IDS` — replace node/edge ids with simplified ones;
* `CYBERIADA_FLAG_SKIP_META` — skip the metainformation node.
* `CYBERIADA_FLAG_STRICT` — check the optional requirements of the standard
  and the EXT-FA rules of a `type/ fa` document; without it the Arena and Lapki
  dialect deviations are repaired on import (see `ARCHITECTURE.md`) and an
  improper automaton is accepted.

## Error codes

| Code | Value | Meaning |
|---|---|---|
| `CYBERIADA_NO_ERROR` | 0 | success |
| `CYBERIADA_XML_ERROR` | -1 | XML reading/writing error |
| `CYBERIADA_FORMAT_ERROR` | -2 | GraphML format violation |
| `CYBERIADA_ACTION_FORMAT_ERROR` | -3 | bad action text syntax |
| `CYBERIADA_METADATA_FORMAT_ERROR` | -4 | bad metainformation |
| `CYBERIADA_NOT_FOUND` | -5 | file or element not found |
| `CYBERIADA_BAD_PARAMETER` | -6 | invalid argument |
| `CYBERIADA_ASSERT` | -7 | internal assertion |
| `CYBERIADA_NOT_IMPLEMENTED` | -8 | unsupported feature |
| `CYBERIADA_MEMORY_ERROR` | -9 | allocation failure or string too long |
| `CYBERIADA_FA_ERROR` | -10 | a finite automaton document (`type/ fa`) breaks the EXT-FA rules |

## Functions

### Document lifecycle

```c
CyberiadaDocument* cyberiada_new_sm_document(void);
int  cyberiada_init_sm_document(CyberiadaDocument* doc);
CyberiadaDocument* cyberiada_copy_sm_document(CyberiadaDocument* source_doc);
int  cyberiada_cleanup_sm_document(CyberiadaDocument* doc);
int  cyberiada_destroy_sm_document(CyberiadaDocument* doc);
```

`new` allocates and initializes on the heap; `init` initializes a caller-owned
(e.g. stack) structure, which must then be released with `cleanup` (frees the
content only). `destroy` frees both the content and the structure; `copy` makes
a deep copy.

### Reading and writing

```c
int cyberiada_read_sm_document(CyberiadaDocument* doc, const char* filename,
                               CyberiadaXMLFormat format, int flags);
int cyberiada_write_sm_document(CyberiadaDocument* doc, const char* filename,
                                CyberiadaXMLFormat format, int flags);
int cyberiada_decode_sm_document(CyberiadaDocument* doc, const char* buffer,
                                 size_t buffer_size, CyberiadaXMLFormat format,
                                 int flags);
int cyberiada_encode_sm_document(CyberiadaDocument* doc, char** buffer,
                                 size_t* buffer_size, CyberiadaXMLFormat format,
                                 int flags);
```

The document must be allocated/initialized before reading. `format` may be
`cybxmlUnknown` on read for auto-detection; on write it must name a concrete
format (the yEd writer supports single-SM documents only). `encode` allocates
the output buffer; the caller frees it.

### Printing (debug)

```c
int cyberiada_print_sm_document(CyberiadaDocument* doc);
int cyberiada_print_node(CyberiadaNode* node, int level);
int cyberiada_print_edge(CyberiadaEdge* edge);
```

Human-readable dump to stdout; `level` is the indent in spaces.

### Construction

```c
CyberiadaSM*             cyberiada_new_sm(void);
CyberiadaNode*           cyberiada_new_node(const char* id);
CyberiadaEdge*           cyberiada_new_edge(const char* id, const char* source,
                                            const char* target, int external);
CyberiadaAction*         cyberiada_new_action(CyberiadaActionType type,
                                              const char* trigger,
                                              const char* guard,
                                              const char* behavior);
CyberiadaCommentData*    cyberiada_new_comment_data(void);
CyberiadaLink*           cyberiada_new_link(const char* ref);
CyberiadaCommentSubject* cyberiada_new_comment_subject(CyberiadaCommentSubjectType type);
CyberiadaMetainformation* cyberiada_new_meta(void);
CyberiadaMetaStringList* cyberiada_new_meta_string(const char* name, const char* value);
```

### Queries

```c
int cyberiada_sm_size(CyberiadaSM* sm, size_t* v, size_t* e,
                      int ignore_comments, int ignore_regions);
CyberiadaNode* cyberiada_graph_find_node_by_id(CyberiadaNode* root, const char* id);
CyberiadaNode* cyberiada_graph_find_node_by_type(CyberiadaNode* root,
                                                 CyberiadaNodeTypeMask mask);
```

The find functions search the subtree under `root` and return `NULL` when
nothing matches; the type search accepts an OR-ed mask of node types.

### Finite automaton check

```c
typedef struct _CyberiadaFAFinding {
    char*                       requirement; /* the testing spec row, "CGML-FA-3.4-1" */
    char*                       element_id;  /* the node/edge, NULL for the document */
    char*                       message;     /* "transition t3 carries no symbol" */
    int                         foreign;     /* 1: not an automaton construct */
    struct _CyberiadaFAFinding* next;
} CyberiadaFAFinding;

int cyberiada_check_fa_document(const CyberiadaDocument* doc,
                                CyberiadaFAFinding** findings);
int cyberiada_destroy_fa_findings(CyberiadaFAFinding* findings);
```

The check collects every EXT-FA violation of a `type/ fa` document, whatever
the flags it was read with, and prints nothing. It returns `CYBERIADA_NO_ERROR`
with `*findings == NULL` for a compliant automaton or a document without
`type/ fa`, `CYBERIADA_FA_ERROR` with the list otherwise; the caller frees the
list. The strict read/write runs the same rules and stops at the first one.

`foreign` marks a construct outside the FA element set, which no draft of an
automaton has: a node kind other than the states, initial, final and comments,
a collapsed state, a behaviour other than `entry/`, a composite with several
regions or an `entry/`, a nested final state, a guard or an event keyword on a
transition, several state machines, `eventPropagation/ propagate`. The other
violations are the faults of a draft (a missing or repeated symbol, the
initial, `EOF`, the alphabets). An editor refuses to turn a document with a
foreign construct into an automaton.

### Isomorphism check

Declared in `cyberiadaml_iso.h` (auto-included). The check looks for the
structural similarity of two state machines that may come from different
authors: the ids mean nothing there, the nodes are paired by their types,
titles, actions and neighbourhood. The changes between two revisions of one
document are the revision diff below.

```c
int cyberiada_check_sm_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2,
                                   int ignore_comments, int require_initial,
                                   CyberiadaIsomorphismResult* result);
int cyberiada_cleanup_isomorphism_result(CyberiadaIsomorphismResult* result);
int cyberiada_compare_node_actions(CyberiadaAction* n1action, CyberiadaAction* n2action,
                                   int* compare_flags);
```

The check fills a caller-owned `CyberiadaIsomorphismResult`: `flags` receives
the `CYBERIADA_ISOMORPH_FLAG_*` verdict (identical / equal / isomorphic /
diff-states / diff-initial / diff-edges); the `diff_nodes`/`diff_edges` pair
arrays carry per-pair `CYBERIADA_NODE_DIFF_*` / `CYBERIADA_EDGE_DIFF_*` flag
words; `new_*`/`missing_*` list the elements present in only one graph;
`new_initial` points to the second graph's differing initial pseudostate when
`require_initial` is set. Empty graphs are comparable: two empty state
machines are identical, and an empty against a non-empty one yields the
diff-states verdict with the non-empty side listed in the `new_*`/`missing_*`
arrays (`require_initial` still rejects empty graphs — they have no initial
pseudostate). The arrays are allocated by the check and released
with `cyberiada_cleanup_isomorphism_result()`; the nodes and edges they point
to belong to the compared graphs. Action comparison reports
`CYBERIADA_ACTION_DIFF_*` flags.

* Every pair of nodes of compatible types (a simple and a composite state are
  compatible) is a candidate; the ids, the titles, the actions and the degrees
  only raise the proximity of a candidate.
* The diff verdicts accumulate: different states and different edges are
  both reported. A node type differs except between a simple and a composite
  state (a different number of children reports that); a title or a
  submachine link present on one side only differs; an absent string compares
  as an empty one.
* With `ignore_comments` off, a comment link to a transition is paired through
  the pairs of the transitions.

The previous pointer-based form is kept for compatibility and is deprecated:

```c
int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2,
                                int ignore_comments, int require_initial,
                                int* result_flags, CyberiadaNode** new_initial,
                                /* ... sixteen optional output pointers ... */);
```

It wraps the structure form; pass `NULL` for outputs you do not need, free the
requested arrays yourself.

### Revision diff

Declared in `cyberiadaml_diff.h` (auto-included). The diff compares two
revisions of one document: the ids of its elements are their identity, checked
by their contents because a new element may get the id of a deleted one. It
lists every pair of elements with what differs in it, and the elements found
on one side only.

```c
void cyberiada_default_diff_options(CyberiadaDiffOptions* options);
int  cyberiada_diff_documents(CyberiadaDocument* doc1, CyberiadaDocument* doc2,
                              const CyberiadaDiffOptions* options,
                              CyberiadaDocumentDiff* result);
int  cyberiada_cleanup_document_diff(CyberiadaDocumentDiff* result);
int  cyberiada_diff_sms(CyberiadaSM* sm1, CyberiadaSM* sm2,
                        const CyberiadaDiffOptions* options, CyberiadaSMDiff* result);
int  cyberiada_cleanup_sm_diff(CyberiadaSMDiff* result);
```

`options` may be `NULL` (the defaults): `flags` of
`CYBERIADA_DIFF_OPTION_GEOMETRY` (compare the geometry and the collapsed flag)
and `CYBERIADA_DIFF_OPTION_IGNORE_COMMENTS` (leave the comments and their links
out), and `geometry_tolerance`, the largest coordinate difference counted as
none (`CYBERIADA_DIFF_GEOMETRY_TOLERANCE`).

| Structure | Holds |
|---|---|
| `CyberiadaDocumentDiff` | `flags` (the OR of the SM flags and `META`; 0: no differences), `meta` (`CyberiadaMetaDiff`: `name`, `value1`, `value2`, a NULL value for a missing one; the differing entries only), `sms` |
| `CyberiadaSMDiff` | `sm1`, `sm2` (NULL for an added or removed SM), `flags` (of the SM node, and `ELEMENTS` when an element differs), `nodes`, `edges` |
| `CyberiadaNodeDiff` | `n1`, `n2` (NULL for an added or removed node), `flags` (0 for an unchanged pair), `action_flags` (`CYBERIADA_ACTION_DIFF_*`) |
| `CyberiadaEdgeDiff` | `e1`, `e2`, `flags` |

| `CYBERIADA_DIFF_FLAG_*` | The element |
|---|---|
| `ADDED`, `REMOVED` | is in the second document only, in the first only |
| `TYPE` | changed its type (not between a simple and a composite state) |
| `TITLE` | changed its title or formal title; a SM: its name |
| `ACTIONS` | changed its actions (their order counts), detailed in `action_flags` |
| `PARENT` | moved to another parent |
| `LINK` | changed its submachine reference |
| `COMMENT` | changed its comment text or markup |
| `COLOR` | changed its colour |
| `GEOMETRY`, `COLLAPSED` | moved or resized, collapsed or expanded (with the geometry option) |
| `SOURCE`, `TARGET` | a transition or a comment link changed its end |
| `TRIGGER`, `GUARD`, `BEHAVIOR`, `PROPAGATION` | a transition changed that part of its action |
| `SUBJECT` | a comment link changed its subject (the type or the fragment) |
| `META` | the document metainformation differs |
| `ELEMENTS` | a SM: some of its nodes or edges differ |

The pairing:

1. the state machines by the id of their node, then by the title, then in the
   document order; the metainformation by its names (the standard version,
   `transitionOrder`, `eventPropagation`, the strings);
2. the nodes, parents before children: a node of the same id and a compatible
   type is the same node when the evidence holds - an equal non-empty title,
   action list or comment text, or two of the weaker signs: the parents paired
   with each other, the same place, the only node of its type under the paired
   parents, a transition to an already paired neighbour by the same trigger;
   the other nodes are added or removed, no node is paired by its structure
   alone;
3. the transitions by the id with their ends paired (one end is enough with the
   same trigger and guard: the other end changed), then by their paired ends and
   trigger, then by their paired ends; the comment links after them.

The nodes come in the order of the second document followed by the removed ones
in the order of the first, the edges and the state machines likewise. The
pointers belong to the compared documents, which must outlive the result; the
meta strings belong to the result. The cleanup frees the arrays and zeroes the
structure; a failing call cleans up itself. The meta node is no node of the
diff.

The geometry is compared as stored, within the tolerance: the nodes' rectangles
and points, the transitions' end points, labels and polylines. The meaningful
basis is the left-top one of CyberiadaML-GraphML 1.0 (a node relative to its
parent's corner), where moving a composite state moves no element inside it;
with the geometry option, documents of different coordinate formats are a
`CYBERIADA_BAD_PARAMETER` - convert one of them first.

### Geometry

```c
int cyberiada_document_has_geometry(CyberiadaDocument* doc);
int cyberiada_clean_document_geometry(CyberiadaDocument* doc);
int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
int cyberiada_update_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
int cyberiada_arrange_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
void cyberiada_default_layout_options(CyberiadaLayoutOptions* options);
int cyberiada_reconstruct_document_geometry_with(CyberiadaDocument* doc, int reconstruct_sm,
                                                 const CyberiadaLayoutOptions* options);
int cyberiada_update_document_geometry_with(CyberiadaDocument* doc, int reconstruct_sm,
                                            const CyberiadaLayoutOptions* options);
int cyberiada_arrange_document_geometry_with(CyberiadaDocument* doc, int reconstruct_sm,
                                             const CyberiadaLayoutOptions* options);
int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
                                        CyberiadaGeometryCoordFormat new_node_coord_format,
                                        CyberiadaGeometryCoordFormat new_edge_coord_format,
                                        CyberiadaGeometryCoordFormat new_edge_pl_coord_format,
                                        CyberiadaGeometryEdgeFormat new_edge_format);
```

`has_geometry` returns 1 if any geometry object is present. `clean` removes
all geometry; `reconstruct` builds it from scratch with the layered layout
of libhtgeom 1.1 (optionally including the SM rectangle) - the pseudostate
roles pin the flow, the state blocks and the transition labels are sized
by estimates from their texts; `update` keeps the existing geometry and
lays out only the geometry-less elements around it (the drawn ones keep
their places, the containers only grow); `arrange` lays the document out
anew, seeded by its geometry; `convert` changes the document
coordinate/edge formats and transforms all geometry data accordingly.

The meta parameter `regionGeometry/ stored` (`CYBERIADA_META_REGION_GEOMETRY`,
`CYBERIADA_META_REGION_GEOMETRY_STORED`) says that the region rects of the
document are the frames of their nested elements as drawn (PNST 1044 7.2.1).
A producer whose regions are the ones it draws sets it: an editor that stores
its regions, a tool after the layout. The library neither sets nor needs it:
it always reads the nested elements from the region rects.

The `_with` calls take the layout options (`NULL` the defaults, as the
calls without options): `CyberiadaLayoutOptions::routing`, a
`CyberiadaLayoutRouting` (the libhtgeom `HTRouting`) - `htRoutingPolyline`
(the default) or `htRoutingOrthogonal`, the routes in runs along and across
the flow (libhtgeom 1.3.0). The orthogonal arrange keeps the drawn ends of
the transitions: an end drawn on a side along the flow stays on that side.

### Utilities

```c
int cyberiada_copy_string(char** target, size_t* size, const char* source);
int cyberiada_encode_meta(CyberiadaMetainformation* meta, char** meta_body,
                          size_t* meta_body_len);
int cyberiada_destroy_meta(CyberiadaMetainformation* meta);
int cyberiada_destroy_action(CyberiadaAction* action);
int cyberiada_destroy_sm(CyberiadaSM* sm);
const char* cyberiada_error_str(int error_code);
```

`cyberiada_copy_string` initializes a string field and its length (of any
size); it is the required way to set strings in the Cyberiada structures.
It does not free the previous value - release it first when replacing a
string. `encode_meta`
serializes metainformation into the comment-body format used in GraphML.
`cyberiada_error_str` returns a static human-readable description of a
library error code.
`cyberiada_cleanup_library` releases the global state of the underlying XML
library; it is optional and, when used, must be called once at process exit
after all threads have finished using the library — never between documents.

---

Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>.
This document is distributed under the GNU Free Documentation License
(version 1.3).
