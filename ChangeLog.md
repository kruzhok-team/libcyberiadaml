# libcyberiadaml project changelog

## Version 1.2.0 (unreleased)

Added:
- the revision diff of two revisions of a document (`cyberiadaml_diff.h`):
  `cyberiada_diff_documents`, `cyberiada_diff_sms` and their cleanups, the
  options (`cyberiada_default_diff_options`: the geometry, the comments
  left out) and the `CYBERIADA_DIFF_FLAG_*` flags; the elements paired by their
  ids, checked by their contents, the state machines by the id, the title and
  the order, the metainformation by its names.

Changed:
- the isomorphism check takes every pair of nodes of compatible types as a
  candidate, the degrees acting through the proximity only: check(A, B) and
  check(B, A) agree.

Fixed:
- the isomorphism check: a submachine link compared with the other link, not
  with the other node's title; a link or a title on one side only differs; a
  simple and a composite state differ by their children, not by their type;
  with the comments compared, a comment link to a transition is paired; the
  proximity kept whole (no cap at 127, which tied the close candidates) and the
  array of its levels sized for every level; the verdict flags accumulate
  instead of overwriting each other;
- the action comparison compares an absent string as an empty one (it
  crashed) and takes a NULL flags pointer everywhere.

## Version 1.1.0

Added:
- the geometry arrange `cyberiada_arrange_document_geometry`;
- the layout options `CyberiadaLayoutOptions` (`cyberiada_default_layout_options`)
  and the calls `cyberiada_{reconstruct,update,arrange}_document_geometry_with`;
- the meta parameter `regionGeometry/ stored` (`CYBERIADA_META_REGION_GEOMETRY`);
- the bands of a composite as the editor draws it;
- the EXT-FA rules on a deterministic finite automaton;
- the geometry reconstruction and update give the comment links their ends
  (libhtgeom 1.3).

Changed:
- an FA output symbol is one line of any characters, as the escapes of FA 4
  allow (`.-`, `/`).

Fixed:
- the target point of a comment link to a transition is relative to the
  transition's source node, not to its target node.
- the strict mode refuses a comment link targeting a state machine or a region
  graph and a `dName` link to a transition.

## Version 1.0.8

Added:
- the geometry update `cyberiada_update_document_geometry` (libhtgeom 1.2): the
  existing geometry is kept, the geometry-less elements are laid out around it;
- the strict mode checks of the node identifiers unique in the whole document and
  of the component identifiers unique in their state machine;
- the non-strict mode reads the Arena node points as rects (centre, y up);
- the non-strict Lapki dialect (Lapki IDE, cyberiadaml-js/py 1.x), the `cybxmlLapki`
  format: writes the Lapki dialect read by lapki-client and lapki-compiler (`cybparser -t lapki`).
- the non-strict mode reads a rect on a pseudostate as its centre point.

Fixed:
- the geometry update gave a drawn transition without a label rect the
  estimated label size at the origin, written as its label rect.

## Version 1.0.7

Added:
- the full geometry reconstruction uses the libhtgeom 1.1 layered layout
  (the node roles, the sizes estimated from the state and transition texts,
  the adaptive direction: left-to-right at the machine level);
- the strict mode checks of the final CyberiadaML-GraphML 1.0 text

Fixed:
- minor bugs with the standard compatibility

## Version 1.0.6

The CyberiadaML-GraphML 1.0 (PNST_1044-2025) compatible version of the library.

Added:
- the strict mode (`CYBERIADA_FLAG_STRICT`) checking the requirements of the
  CyberiadaML-GraphML 1.0 standard;
- the yEd Ostranna / Berloga 1.6 export support;
- geometry reconstruction of the documents without geometry;
- new isomorphism API (`cyberiadaml_iso.h`);
- `cyberiada_error_str` and `cyberiada_cleanup_library` in the public interface;
- the ctest-based test suite, the architecture and the API documentation.

Fixed:
- geometry reconstruction update;
- submachine state subgraphs support;
- full CyberiadaML-GraphML 1.0 standard compatibility;
- the CMake package configuration is installed into its own directory.

## Version 1.0

Stable version of the library implementing the 1.0 version of
CyberiadaML-GrapML standard as well as the legacy yED HSM formats.

Known issues:
- document's geometry format was partially supported;
- geometry reconstruction was partially implemented;
- no transitions keywords support.
