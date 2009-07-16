/* mathcodes.h
   
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

#ifndef MATHCODES_H
#  define MATHCODES_H

/* mathcodes.c */

#  define no_mathcode 0         /* this is a flag for |scan_delimiter| */
#  define tex_mathcode 8
#  define aleph_mathcode 16
#  define xetex_mathcode 21
#  define xetexnum_mathcode 22

typedef struct mathcodeval {
    integer class_value;
    integer origin_value;
    integer family_value;
    integer character_value;
} mathcodeval;

void set_math_code(integer n,
                   integer commandorigin,
                   integer mathclass,
                   integer mathfamily, integer mathcharacter, quarterword gl);

mathcodeval get_math_code(integer n);
integer get_math_code_num(integer n);
integer get_del_code_num(integer n);
mathcodeval scan_mathchar(int extcode);
mathcodeval scan_delimiter_as_mathchar(int extcode);

mathcodeval mathchar_from_integer(integer value, int extcode);
void show_mathcode_value(mathcodeval d);

typedef struct delcodeval {
    integer class_value;
    integer origin_value;
    integer small_family_value;
    integer small_character_value;
    integer large_family_value;
    integer large_character_value;
} delcodeval;

void set_del_code(integer n,
                  integer commandorigin,
                  integer smathfamily,
                  integer smathcharacter,
                  integer lmathfamily, integer lmathcharacter, quarterword gl);

delcodeval get_del_code(integer n);

void unsave_math_codes(quarterword grouplevel);
void initialize_math_codes(void);
void dump_math_codes(void);
void undump_math_codes(void);

#endif

