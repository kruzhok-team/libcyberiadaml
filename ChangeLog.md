# libcyberiadaml project changelog

The CyberiadaML-GraphML 1.0 (PNST_1044-2025) compatible version of the library.

## Version 1.0.6

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
- full CyberiadaML-GraphML 1.0 standard compatibility.

## Version 1.0

Stable version of the library implementing the 1.0 version of
CyberiadaML-GrapML standard as well as the legacy yED HSM formats.

Known issues:
- document's geometry format was partially supported;
- geometry reconstruction was partially implemented;
- no transitions keywords support.
