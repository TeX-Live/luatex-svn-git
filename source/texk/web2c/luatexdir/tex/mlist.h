/* mlist.h
   
   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

#ifndef MLIST_H
#  define MLIST_H 1

extern pointer cur_mlist;
extern integer cur_style;
extern boolean mlist_penalties;
extern integer cur_size;

void run_mlist_to_hlist(halfword, integer, boolean);
void fixup_math_parameters(integer fam_id, integer size_id, integer f,
                           integer lvl);


scaled get_math_quad(int a);
boolean check_necessary_fonts(void);

void mlist_to_hlist(void);

#endif
