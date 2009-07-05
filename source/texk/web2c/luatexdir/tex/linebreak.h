/* linebreak.h
   
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

#ifndef LINEBREAK_H
#  define LINEBREAK_H

#  define left_side 0
#  define right_side 1

extern halfword just_box;       /* the |hlist_node| for the last line of the new paragraph */
extern integer paragraph_dir;   /* main direction of paragraph */
extern integer line_break_dir;  /* current direction within paragraph */
extern integer line_break_context;      /* the surrounding state for |line_break| calls */
extern int break_c_htdp;        /* height-depth entry in |char_info| */
extern integer temp_no_whatsits;        /* used when closing group */
extern integer temp_no_dirs;    /* used when closing group */
extern integer temporary_dir;
extern halfword dir_ptr, dir_tmp, dir_rover;

extern void line_break(boolean d);

#  define inf_bad 10000         /* infinitely bad value */
#  define awful_bad 07777777777 /* more than a billion demerits */

void initialize_active(void);

halfword find_protchar_left(halfword l, boolean d);
halfword find_protchar_right(halfword l, halfword r);

#endif
