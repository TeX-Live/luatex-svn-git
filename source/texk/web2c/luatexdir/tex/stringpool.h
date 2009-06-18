/* stringpool.h
   
   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */

/* $Id$ */

extern packed_ASCII_code *str_pool;
extern pool_pointer *str_start;
extern pool_pointer pool_ptr;
extern str_number str_ptr;
extern pool_pointer init_pool_ptr;
extern str_number init_str_ptr;

#define STRING_OFFSET 0x200000
#define STRING_OFFSET_BITS 21

#define str_start_macro(a) str_start[(a) - STRING_OFFSET]

#define biggest_char 1114111
#define number_chars 1114112
#define special_char 1114113    /* |biggest_char+2| */

/*
  Several of the elementary string operations are performed using
  macros instead of procedures, because many of the
  operations are done quite frequently and we want to avoid the
  overhead of procedure calls. For example, here is
  a simple macro that computes the length of a string.
*/

#define str_length(a) (str_start_macro((a)+1)-str_start_macro(a))

/* The length of the current string is called |cur_length|: */

#define cur_length (pool_ptr - str_start_macro(str_ptr))

/* Strings are created by appending character codes to |str_pool|.
   The |append_char| macro, defined here, does not check to see if the
   value of |pool_ptr| has gotten too high; this test is supposed to be
   made before |append_char| is used. There is also a |flush_char|
   macro, which erases the last character appended.

   To test if there is room to append |l| more characters to |str_pool|,
   we shall write |str_room(l)|, which aborts \TeX\ and gives an
   apologetic error message if there isn't enough room.
*/

/* put |ASCII_code| \# at the end of |str_pool| */
#define append_char(A) str_pool[pool_ptr++]=(A)

#define str_room(A) check_pool_overflow((pool_ptr+(A)))

#define flush_char() decr(pool_ptr) /* forget the last character in the pool */

/* To destroy the most recently made string, we say |flush_string|. */

#define flush_string() do {					\
     decr(str_ptr);						\
     pool_ptr=str_start_macro(str_ptr);				\
   } while (0)

extern str_number make_string (void);
extern boolean str_eq_buf(str_number s, integer k);
extern boolean str_eq_str(str_number s, str_number t);
extern boolean get_strings_started (void) ;

extern str_number search_string(str_number search) ;
extern str_number slow_make_string (void);
