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

 +---------------------+  independent of cyberiadaml.c, work on CyberiadaSM:
 |     isomorph.c      |  the isomorphism check: the structural similarity
 +---------------------+  of two diagrams
 |       diff.c        |  the revision diff: two revisions of one document,
 +---------------------+  paired by the ids
```

The call direction is strictly downward: `cyberiadaml.c` is the only module
that touches XML and the only caller of `cyb_meta` (decode path),
`cyb_graph_recon` and `geometry.c`. Nothing calls back up. `isomorph.c`,
`diff.c` and `geometry.c` do not depend on the parser and can be used on
documents constructed programmatically.

## Modules

| Module | Role |
|---|---|
| `cyberiadaml.h` | The public header: the core types, flags, error codes and prototypes. Installed to `include/cyberiada/`. |
| `cyberiadaml_iso.h` | The public isomorphism check header (result structure, flags, prototypes); installed next to `cyberiadaml.h` and included by it. |
| `cyberiadaml_diff.h` | The public revision diff header (options, result structures, flags, prototypes); installed next to `cyberiadaml.h` and included by it. |
| `cyberiadaml.c` | GraphML reader and writer: key tables, dialect detection, the DOM-walk processor, both format writers, the `read`/`write`/`decode`/`encode` entry points. |
| `cyb_types.c/.h` | Lifecycle of the core structures: `new_*`, deep `copy_*`, `destroy_*`, `print_*` for document / SM / node / edge / action / link / comment. |
| `cyb_graph.c/.h` | Graph primitives: find node by id/type, find edge, add node/edge with type-aware placement. |
| `cyb_graph_recon.c/.h` | Node and edge identifier reconstruction and simplification (`CYBERIADA_FLAG_SIMPLIFY_IDS`), backed by a rename list. |
| `cyb_meta.c/.h` | Metainformation lifecycle, encode/decode of the metadata comment body, `standardVersion` validation. |
| `cyb_actions.c` | Regex-driven parsing of state/transition action text into `CyberiadaAction` lists (`trigger [guard] / behavior`); legacy yEd block splitting; action comparison for the diff. |
| `cyb_regexps.c` | Compilation and release of the action regexps; carries per-parse dialect flags (`berloga_legacy`, `arena_legacy`, `flattened_regexps`). |
| `cyb_regex.h` | Platform wrapper selecting the regex backend: POSIX `<regex.h>` on Linux, `pcre2posix` elsewhere (`CYBERIADA_PCRE2_REGEXPS`). |
| `cyb_string.c/.h` | String copy/append/trim helpers; `MAX_STR_LEN` bounds only the attribute buffers. |
| `cyb_structs.c/.h` | One generic `{key, data, next}` node reused as stack, list and queue. |
| `cyb_node_stack.c/.h` | Typed stack tracking (current XML element, current node) during the DOM walk. |
| `geometry.c/.h` | Bridge to libhtgeom: Cyberiada graph -> `HTDocument` -> convert / reconstruct -> back. Clean, round and validation helpers. |
| `isomorph.c` | Graph isomorphism check: the structural similarity of two state machines (prototypes in `cyberiadaml_iso.h`). |
| `diff.c` | Revision diff: the pairs of the elements of two revisions by their verified ids, with their differences (prototypes in `cyberiadaml_diff.h`). |
| `utf8enc.c/.h` | Escape/unescape of non-ASCII bytes around action text. |
| `cyb_error.h` | `DEBUG()` / `ERROR()` reporting macros. |
| `parser/parser.c` | `cybparser`, the reference command line tool: `print`, `convert`, `isomorph`, `diff`. |

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
                                           └── non-strict repairs? ──────► Lapki dialect
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
     +──► graph checks (pseudostates, geometry, ids; the strict rows with the flag)
     +──► EXT-FA checks (cyb_fa.c) when the metadata says type/ fa, in any mode
     |
     v
 CyberiadaDocument
```

The parser is a table-driven pushdown automaton: the `GraphProcessorState`
enum enumerates the processing states, and two transition tables (one for the
Cyberiada format, one for the yEd family) map (state, XML element) to a
handler function. The automaton is dispatched over a recursive DOM traversal.

All mutable parsing state travels in a per-document parser context (the
action regexps with their dialect flags and the key map binding non-default
GraphML key ids to the standard attributes); the key catalog itself is
constant, so concurrent document decoding is safe.

### Input dialects

The public format enum has two values plus auto-detection; internally six
dialects are distinguished and reported via `CyberiadaDocument.format`:

| `doc->format` string | Family | Detection |
|---|---|---|
| `Cyberiada-GraphML-1.0` | Cyberiada | default (no yWorks namespace) |
| `Cyberiada-GraphML-Arena` | Cyberiada | non-standard `referenceGraphID` key; relaxed edge-id parsing; non-strict: node points (centre, y up) read as rects, label points dropped |
| `Cyberiada-GraphML-Lapki` | Cyberiada | non-strict only: `lapkiVisual` meta or a repair applied — region graph id = node id renamed `<id>::region`, point on a rect node read as a loose rect, `-1 x -1` rect read as a point, repeated `CGML_META` dropped (its parameters too), the first of repeated meta parameters kept and `undefined` ones dropped, `gFormat` without the version, `dLapki*` data skipped, `CGML_COMPONENT` + `id/` renamed `CGML_COMPONENT <id>`, meta edges dropped, repeated edge ids `<id>#N`, a nameless machine named by its graph id, the initial transition text without `/` read as its behavior |
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
  documents;
* `cybxmlLapki` — the Cyberiada writer over a copy turned into the Lapki IDE
  dialect read by cyberiadaml-js/py 1.x (lapki-client, lapki-compiler):

```
 copy ─► layout of the absent geometry (states, choices, pseudostates, comments)
      ─► CGML_META in every machine (the platform is per machine there)
      ─► CGML_COMPONENT + id/ parameter, label and comment points
      ─► the initial transition behavior without a slash
      ─► refused: no platform, other formal comments, completion transitions
```

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
be reconstructed with the `CYBERIADA_FLAG_RECONSTRUCT_*` flags (the
preserving fill-in of the missing geometry) or rebuilt from the structure
by the standalone `cyberiada_reconstruct_document_geometry()`, or completed
around the existing geometry by `cyberiada_update_document_geometry()`, or laid
out anew as the existing geometry is arranged - its bands, order, directions and
minimum sizes - by `cyberiada_arrange_document_geometry()`; the `_with` calls
take the layout options (the orthogonal routes); the bridge
passes the node roles (`HTNodeRole`) and the text-size estimates of the
states, the comments and the transition labels to the libhtgeom layered
layout (10 px per character, 20 px per line, 8 px margins: a state at least
300x200; a composite's title, entry and internal actions as its header and
its exit block as the footer band at the bottom, as the editor draws it, the
region between them; a comment's body wrapped at 40 characters plus its name
line, at least 200x100 - the editor's default box). The meta node and the component
comments (`CGML_COMPONENT`, hidden in the editors) without geometry take no
part in the layout.

## Isomorphism check and revision diff

Two algorithms answer two questions about a pair of state machines:

```
 isomorph.c                                  diff.c
+------------------------------------+      +------------------------------------+
| the isomorphism check              |      | the revision diff                  |
| two diagrams, maybe two authors:   |      | two revisions of one document:     |
| ids mean nothing, the structure    |      | ids mean identity, checked by the  |
| and the texts pair the nodes       |      | contents; unpaired: added, removed |
+------------------------------------+      +------------------------------------+
  verdict + the differing pairs               every pair + its flags, the meta,
  cyberiada_check_sm_isomorphism()            the state machines
                                              cyberiada_diff_documents(), _sms()
  cybparser isomorph                          cybparser diff, libcyberiadamlpp
```

They share only `cyberiada_compare_node_actions()` (cyb_actions.c).

`cyberiada_check_sm_isomorphism()` compares two `CyberiadaSM` graphs:

1. enumerate vertexes with their in/out degrees;
2. build a candidate-match matrix (every pair of compatible types) and the
   proximity matrices over nodes and edges;
3. search permutation matrices in descending proximity order.

The verdict is a flag word — `identical` / `equal` / `isomorphic` /
different states / different initial / different edges — plus arrays of
changed node and edge pairs (with per-pair difference flags), added elements
and missing elements — gathered in the `CyberiadaIsomorphismResult` structure
and released with `cyberiada_cleanup_isomorphism_result()`. Action-level
comparison distinguishes different behavior, behavior order and behavior
arguments.

`cyberiada_diff_documents()` pairs the state machines (id, title, order) and
the metainformation (by name), then `cyberiada_diff_sms()` pairs the elements of
each pair of state machines:

```
 sm1, sm2
    |
    v
 the nodes, parents before children        an id index of sm2 (sorted)
    same id + compatible type  ----------> the evidence:
    + the evidence holds                     2  equal title, actions, comment text
    repeated while new pairs appear          1  paired parents, the same place,
    |                                           the only one of its type,
    |                                           a paired neighbour by its trigger
    v                                        a pair at 2
 the edges: the id with paired ends; the paired ends + the trigger; the paired ends
    |
    v
 every pair compared -> CYBERIADA_DIFF_FLAG_*; unpaired -> ADDED / REMOVED
```

No pass pairs the leftovers by their structure: an element whose id changed is
removed and added, and the isomorphism check is the tool for the diagrams
renumbered or drawn anew. The geometry is compared as stored, so both documents
come in the same coordinate format, the left-top one being the meaningful
basis.

## Memory ownership

* Every structure has `new` / `copy` / `destroy` functions in `cyb_types.c`;
  the document owns its state machines, which own their nodes and edges.
* Every string field is paired with an explicit `_len` and must be set via
  `cyberiada_copy_string()`; the content strings have no length limit.
* `cyberiada_cleanup_sm_document()` frees the content but not the structure
  (for stack allocation); `cyberiada_destroy_sm_document()` frees both.
* The isomorphism result arrays are allocated by the check and released with
  `cyberiada_cleanup_isomorphism_result()`; the nodes/edges they point to
  belong to the compared documents.
* The diff results are allocated by the diff and released with
  `cyberiada_cleanup_document_diff()` / `cyberiada_cleanup_sm_diff()`; their
  element pointers belong to the compared documents, their meta strings to the
  result.

---

Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>.
This document is distributed under the GNU Free Documentation License
(version 1.3).
