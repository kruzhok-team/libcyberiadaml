/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The listing of a document: an element per line, the textconv driver of git
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 * ----------------------------------------------------------------------------- */

#ifndef __CYBERIADA_PARSER_LISTING_H
#define __CYBERIADA_PARSER_LISTING_H

#include "cyberiadaml.h"

/* Print the listing of the document file to stdout (docs/ARCHITECTURE.md, The listing);
   an empty file prints nothing, a file the library cannot read is printed as it is with
   a line on stderr. Returns the exit code of the program. */
int print_listing(const char* path, CyberiadaXMLFormat format);

#endif
