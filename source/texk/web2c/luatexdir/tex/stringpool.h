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

#define STRING_OFFSET 0x200000
#define STRING_OFFSET_BITS 21

#define str_start_macro(a) str_start[(a) - STRING_OFFSET]
#define str_length(a) (str_start_macro((a)+1)-str_start_macro(a))

#define biggest_char 1114111
#define number_chars 1114112
#define special_char 1114113    /* |biggest_char+2| */


#define cur_length (pool_ptr - str_start_macro(str_ptr))

/* put |ASCII_code| \# at the end of |str_pool| */
#define append_char(A) str_pool[pool_ptr++]=(A)
#define str_room(A) check_pool_overflow((pool_ptr+(A)))

#define flush_string() do {					\
     decr(str_ptr);						\
     pool_ptr=str_start_macro(str_ptr);				\
   } while (0)
