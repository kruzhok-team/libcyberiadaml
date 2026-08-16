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
with `cyberiada_copy_string()`.

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
An action is `trigger [guard] / behavior`; a state owns a list of
entry/exit/do actions, a transition owns a list of transition actions.

### Metainformation (`CyberiadaMetainformation`)

`standard_version` (must be `"1.0"`, `CYBERIADA_STANDARD_VERSION_CYBERIADAML`),
`transition_order_flag` (0 unset / 1 transition first / 2 exit first),
`event_propagation_flag` (0 unset / 1 block / 2 propagate) and a generic
`strings` name/value list for everything else. The names of the standard
parameters are provided as `CYBERIADA_META_*` constants.

### File formats (`CyberiadaXMLFormat`)

* `cybxmlCyberiada10` — Cyberiada-GraphML 1.0;
* `cybxmlYED` — the legacy yEd-based Berloga/Ostranna format;
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

### Isomorphism and diff

Declared in `cyberiadaml_iso.h` (auto-included).

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
`require_initial` is set. The arrays are allocated by the check and released
with `cyberiada_cleanup_isomorphism_result()`; the nodes and edges they point
to belong to the compared graphs. Action comparison reports
`CYBERIADA_ACTION_DIFF_*` flags.

The previous pointer-based form is kept for compatibility and is deprecated:

```c
int cyberiada_check_isomorphism(CyberiadaSM* sm1, CyberiadaSM* sm2,
                                int ignore_comments, int require_initial,
                                int* result_flags, CyberiadaNode** new_initial,
                                /* ... sixteen optional output pointers ... */);
```

It wraps the structure form; pass `NULL` for outputs you do not need, free the
requested arrays yourself.

### Geometry

```c
int cyberiada_document_has_geometry(CyberiadaDocument* doc);
int cyberiada_clean_document_geometry(CyberiadaDocument* doc);
int cyberiada_reconstruct_document_geometry(CyberiadaDocument* doc, int reconstruct_sm);
int cyberiada_convert_document_geometry(CyberiadaDocument* doc,
                                        CyberiadaGeometryCoordFormat new_node_coord_format,
                                        CyberiadaGeometryCoordFormat new_edge_coord_format,
                                        CyberiadaGeometryCoordFormat new_edge_pl_coord_format,
                                        CyberiadaGeometryEdgeFormat new_edge_format);
```

`has_geometry` returns 1 if any geometry object is present. `clean` removes
all geometry; `reconstruct` builds it from scratch (optionally including the
SM rectangle); `convert` changes the document coordinate/edge formats and
transforms all geometry data accordingly.

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

`cyberiada_copy_string` initializes a string field and its length; it is
the required way to set strings in the Cyberiada structures. It does not
free the previous value - release it first when replacing a string. `encode_meta`
serializes metainformation into the comment-body format used in GraphML.
`cyberiada_error_str` returns a static human-readable description of a
library error code.

---

Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>.
This document is distributed under the GNU Free Documentation License
(version 1.3).
