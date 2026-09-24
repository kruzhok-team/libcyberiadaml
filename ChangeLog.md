# libcyberiadaml project changelog

## Version 1.0.7 (unreleased)

Added:
- the strict mode checks of the final CyberiadaML-GraphML 1.0 text: one entry/exit/do
  block per state, reserved event names, `defer` only as the whole behavior of an
  internal transition, `propagate`/`block` only with an event name, the commented
  fragment present in the subject, named entry/exit points, no behavior and no
  self-reference in submachine states, submachine points named after the points of the
  referenced state machine.

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
