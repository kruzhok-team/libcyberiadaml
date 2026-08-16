/* -----------------------------------------------------------------------------
 * The Cyberiada GraphML library implemention
 *
 * The regular expressions platform wrapper
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

#ifndef __CYBERIADA_REGEX_H
#define __CYBERIADA_REGEX_H

/* The single regexp implementation is compiled against the POSIX regex
   library on Linux and against the pcre2 POSIX wrapper elsewhere.
   PCRE2 requires REG_DOTALL to match the POSIX default where the dot
   matches newline characters. */

#ifdef CYBERIADA_PCRE2_REGEXPS
#include <pcre2posix.h>
#define cyb_regcomp(preg, pattern) pcre2_regcomp(preg, pattern, REG_DOTALL)
#define cyb_regexec                pcre2_regexec
#define cyb_regfree                pcre2_regfree
#else
#include <regex.h>
#define cyb_regcomp(preg, pattern) regcomp(preg, pattern, REG_EXTENDED)
#define cyb_regexec                regexec
#define cyb_regfree                regfree
#endif

#endif
