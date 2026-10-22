# libcyberiadaml project changelog

## Version 1.0.9 (unreleased)

Fixed:
- the target point of a comment link to a transition is relative to the
  transition's source node (PNST 1044 9.1.1), not to its target node.

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
