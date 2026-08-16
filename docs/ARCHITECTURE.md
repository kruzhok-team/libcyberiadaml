# The Cyberiada GraphML Library Architecture

This document describes the internal structure of `libcyberiadaml`, the base C
library for reading and writing CyberiadaML GraphML files with Hierarchical
State Machine (HSM) diagrams.

## Components

```
                        +--------------------------------------+
                        |            client program            |
                        |    (cybparser, libcyberiadamlpp)     |
                        +------------------+-------------------+
                                           | public API calls
                                           v
 +------------------+     +--------------------------------------+
 |     libxml2      |<----|            cyberiadaml.c             |
 | (DOM, xmlWriter) | XML |  read / write / decode / encode      |
 +------------------+     |  dialect detection, GraphML keys,    |
                          |  processor-state tables              |
                          +--+------+------+------+------+------+
                             |      |      |      |      |
              action text    | meta |  XML walk   |      |  rename ids
                    v        |  v   |   state     |      |      v
          +--------------+   | +----------+       |      | +----------------+
          | cyb_actions  |   | | cyb_meta |       |      | | cyb_graph_recon|
          | (regexps per |   | +----------+       |      | +----------------+
          |  dialect)    |   |      +-------------+---+  |
          +------+-------+   |      | cyb_node_stack   |  |  geometry
                 |           |      +------------------+  |  import/export
                 v           |  lifecycle, print          |      v
          +--------------+   |      v                     | +-------------+     +-----------+
          | cyb_regexps  |   | +-----------+              | | geometry.c  |---->| libhtgeom |
          +--------------+   | | cyb_types |              | | convert,    |     | (coords   |
                             | +-----------+              | | clean,      |     |  math)    |
   leaf utilities:           |      +----------------+    | | reconstruct |     +-----------+
   cyb_string, cyb_structs,  |      | cyb_graph      |    | +-------------+
   utf8enc, cyb_error.h      |      | (find/add)     |    |
                             |      +----------------+    |

 +---------------------+  independent of cyberiadaml.c, works on CyberiadaSM:
 |     isomorph.c      |  isomorphism check & structural diff
 +---------------------+
```

The call direction is strictly downward: `cyberiadaml.c` is the only module
that touches XML and the only caller of `cyb_meta` (decode path),
`cyb_graph_recon` and `geometry.c`. Nothing calls back up. `isomorph.c` and
`geometry.c` do not depend on the parser and can be used on documents
constructed programmatically.

## Modules

| Module | Role |
|---|---|
| `cyberiadaml.h` | The public header: all types, flags, error codes, prototypes. The only installed header (`include/cyberiada/cyberiadaml.h`). |
| `cyberiadaml.c` | GraphML reader and writer: key tables, dialect detection, the DOM-walk processor, both format writers, the `read`/`write`/`decode`/`encode` entry points. |
| `cyb_types.c/.h` | Lifecycle of the core structures: `new_*`, deep `copy_*`, `destroy_*`, `print_*` for document / SM / node / edge / action / link / comment. |
| `cyb_graph.c/.h` | Graph primitives: find node by id/type, find edge, add node/edge with type-aware placement. |
| `cyb_graph_recon.c/.h` | Node and edge identifier reconstruction and simplification (`CYBERIADA_FLAG_SIMPLIFY_IDS`), backed by a rename list. |
| `cyb_meta.c/.h` | Metainformation lifecycle, encode/decode of the metadata comment body, `standardVersion` validation. |
| `cyb_actions.c` / `cyb_actions_pcre2.c` | Regex-driven parsing of state/transition action text into `CyberiadaAction` lists (`trigger [guard] / behavior`); legacy yEd block splitting; action comparison for the diff. POSIX regex on Linux, PCRE2 elsewhere. |
| `cyb_regexps.c` / `cyb_regexps_pcre2.c` | Compilation and release of the action regexps; carries per-parse dialect flags (`berloga_legacy`, `arena_legacy`, `flattened_regexps`). |
| `cyb_string.c/.h` | String copy/append/trim helpers; `MAX_STR_LEN` (4096) limit. |
| `cyb_structs.c/.h` | One generic `{key, data, next}` node reused as stack, list and queue. |
| `cyb_node_stack.c/.h` | Typed stack tracking (current XML element, current node) during the DOM walk. |
| `geometry.c/.h` | Bridge to libhtgeom: Cyberiada graph -> `HTDocument` -> convert / reconstruct -> back. Clean, round and validation helpers. |
| `isomorph.c` | Graph isomorphism check and structural diff (prototypes in `cyberiadaml.h`). |
| `utf8enc.c/.h` | Escape/unescape of non-ASCII bytes around action text. |
| `cyb_error.h` | `DEBUG()` / `ERROR()` reporting macros. |
| `parser/parser.c` | `cybparser`, the reference command line tool: `print`, `convert`, `diff`. |

## The reading pipeline

```
 file/buffer
     |
     v
 flattened-file probe ──────────────► flattened_regexps flag
     |
     v
 libxml2 DOM parse
     |
     v
 namespace check ── yWorks ns? ──► yEd family ── SchemeName? ──► Berloga (<1.6 / 1.6)
     |                                    └────────────────────► Ostranna / Orbita
     └─ otherwise ──► Cyberiada-GraphML-1.0 ── referenceGraphID key? ──► Arena dialect
     |
     v
 recursive DOM walk + pushdown automaton
   (GraphProcessorState, one transition table per family,
    cyb_node_stack holds the current XML element/node)
     |
     +──► action text ──► cyb_actions (dialect regexps) ──► CyberiadaAction list
     +──► meta comment ──► cyb_meta decode ──► CyberiadaMetainformation
     |
     v
 post-processing: edge source/target resolution, id simplification (flag),
 geometry import (geometry.c): fix source coord format by dialect,
 convert to the caller-requested formats, optional reconstruction
     |
     v
 CyberiadaDocument
```

The parser is a table-driven pushdown automaton: the `GraphProcessorState`
enum enumerates the processing states, and two transition tables (one for the
Cyberiada format, one for the yEd family) map (state, XML element) to a
handler function. The automaton is dispatched over a recursive DOM traversal.

### Input dialects

The public format enum has two values plus auto-detection; internally five
dialects are distinguished and reported via `CyberiadaDocument.format`:

| `doc->format` string | Family | Detection |
|---|---|---|
| `Cyberiada-GraphML-1.0` | Cyberiada | default (no yWorks namespace) |
| `Cyberiada-GraphML-Arena` | Cyberiada | non-standard `referenceGraphID` key; relaxed edge-id parsing |
| `yEd Berloga` | yEd | `SchemeName` attribute, Berloga AD < 1.6 action syntax |
| `yEd Berloga-1.6` | yEd | `SchemeName` attribute, Berloga AD 1.6 action syntax |
| `yEd Ostranna` | yEd | yWorks namespace default (also the Orbita Simulator) |

A separate "flattened" heuristic (no two consecutive whitespace characters in
the whole document) selects an alternative action-regexp set before parsing.

## The writing pipeline

Two writers built on libxml2 `xmlTextWriter`:

* `cybxmlCyberiada10` — emits `Cyberiada-GraphML-1.0` documents (the standard
  key set, metainformation comment first, edges in one block);
* `cybxmlYED` — emits the legacy yEd format; restricted to single-SM
  documents.

Geometry conversion flags are rejected on export: the target format dictates
the output geometry. Export-only flags (skip / shrink / round) are applied by
`geometry.c` before serialization.

## Geometry

There are three node coordinate systems (see libhtgeom for the math):

1. Cyberiada-GraphML 1.0 — hierarchical, relative to the parent's top-left
   corner;
2. legacy yEd — absolute;
3. Qt QGraphicsView — hierarchical, relative to the parent node center
   (the in-memory default).

All conversion, cleaning, rounding and reconstruction is delegated to
libhtgeom through `geometry.c`: the document is translated to an `HTDocument`
tree, transformed, and written back. On import the *source* format is fixed by
the detected dialect; the target formats come from the import flags (default:
center-local nodes/edges/polylines, border edge points). Missing geometry can
be reconstructed with the `CYBERIADA_FLAG_RECONSTRUCT_*` flags or the
standalone `cyberiada_reconstruct_document_geometry()`.

## Isomorphism and diff

`cyberiada_check_isomorphism()` compares two `CyberiadaSM` graphs:

1. enumerate vertexes with their in/out degrees;
2. build a candidate-match matrix and proximity matrices over nodes and edges;
3. search permutation matrices in descending proximity order.

The verdict is a flag word — `identical` / `equal` / `isomorphic` /
different states / different initial / different edges — plus arrays of
changed node and edge pairs (with per-pair difference flags), added elements
and missing elements. Action-level comparison distinguishes different
behavior, behavior order and behavior arguments.

## Memory ownership

* Every structure has `new` / `copy` / `destroy` functions in `cyb_types.c`;
  the document owns its state machines, which own their nodes and edges.
* Every string field is paired with an explicit `_len` and must be set via
  `cyberiada_copy_string()`; strings are limited by `MAX_STR_LEN` (4096).
* `cyberiada_cleanup_sm_document()` frees the content but not the structure
  (for stack allocation); `cyberiada_destroy_sm_document()` frees both.
* The arrays returned by `cyberiada_check_isomorphism()` are owned by the
  caller; the nodes/edges they point to belong to the compared documents.

---

Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>.
This document is distributed under the GNU Free Documentation License
(version 1.3).
