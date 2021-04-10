/* fn.h: arbitrarily long filenames (or just strings).

   Copyright 2001, 2005 Olaf Weber.
   Copyright 1993, 2008, 2011 Karl Berry.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef KPATHSEA_FN_H
#define KPATHSEA_FN_H

#ifdef MAKE_KPSE_DLL /* libkpathsea internal only */

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Arbitrarily long filenames; it's inconvenient to use obstacks here,
   because we want to maintain a null terminator.  Also used for
   dynamically growing strings even when the null byte isn't necessary,
   e.g., in `variable.c', since I don't want to pass obstacks around
   everywhere, and one can't free parts of an obstack arbitrarily.  */

typedef struct
{
  string str;
  unsigned allocated;
  unsigned length; /* includes the terminating null byte, if any */
} fn_type;

#define FN_STRING(fn) ((fn).str)
#define FN_ALLOCATED(fn) ((fn).allocated)
#define FN_LENGTH(fn) ((fn).length)


/* Create a new empty fn.  */
extern fn_type fn_init (void);

/* Create a new fn from the first LEN characters from S and a null.  */
extern fn_type fn_copy0 (const_string s,  unsigned len);

/* Free what's been allocated.  Can also just free the string if it's
   been extracted out.  Fatal error if nothing allocated in F.  */
extern void fn_free (fn_type *f);

/* Append the character C to the fn F.  Don't append trailing null.  */
extern void fn_1grow (fn_type *f, char c);

/* Append LENGTH bytes from SOURCE to F.  */
extern void fn_grow (fn_type *f, const_string source, unsigned length);

/* Concatenate the component S to the fn F.  Assumes string currently in
   F is null terminated.  */
extern void fn_str_grow (fn_type *f, const_string s);

/* Add a null to F's string at position LOC, and update its length.
   Fatal error if LOC is past the end of the string.  */
extern void fn_shrink_to (fn_type *f, unsigned loc);

#endif /* MAKE_KPSE_DLL */

#endif /* not KPATHSEA_FN_H */
