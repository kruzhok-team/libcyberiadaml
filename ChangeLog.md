# libcyberiadaml project changelog

## Version 1.0.6

Added:
- the strict mode (`CYBERIADA_FLAG_STRICT`) checking the requirements of the
  PNST 1044-2025 standard;
- the yEd Ostranna and Berloga 1.6 export dialects;
- the event handling keywords in the transition actions;
- the geometry reconstruction of the documents without geometry;
- the isomorphism result API (`cyberiadaml_iso.h`);
- `cyberiada_error_str` and `cyberiada_cleanup_library` in the public interface;
- the ctest-based test suite, the architecture and the API documentation.

Fixed:
- the metainformation node is not placed by the geometry reconstruction;
- the document geometry format handling in the reconstruction and the export;
- the base format rect sizes are derived from the authored coordinates;
- the GraphML decoder is reentrant;
- the memory leaks on the decoding error paths;
- the string length limit of the document content is removed;
- the buffer overflows and the null dereferences found by the security audit;
- the metadata format error code is propagated from the parser.

## Version 1.0

Stable version of the library implementing the 1.0 version of
CyberiadaML-GrapML standard as well as the legacy yED HSM formats.

Known issues:
- document's geometry format was partially supported;
- geometry reconstruction was partially implemented;
- no transitions keywords support.
