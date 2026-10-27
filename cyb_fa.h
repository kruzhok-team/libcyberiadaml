/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The EXT-FA rules: deterministic finite automaton documents (type/ fa)
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#ifndef __CYBERIADA_FA_H
#define __CYBERIADA_FA_H

#include "cyberiadaml.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* Check the EXT-FA rules of a finite automaton document (compat-tests testing
	   spec 2.9, docs/FA-extension.md 3-5); the caller checks cyberiada_document_is_fa.
	   Returns CYBERIADA_FA_ERROR at the first violation. */
	int cyberiada_check_fa(const CyberiadaDocument* doc);

#ifdef __cplusplus
}
#endif

#endif
