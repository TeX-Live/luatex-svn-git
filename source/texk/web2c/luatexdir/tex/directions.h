/* directions.h

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

#ifndef DIRECTIONS_H
#  define DIRECTIONS_H

#  define dir_T 0
#  define dir_L 1
#  define dir_B 2
#  define dir_R 3

#  define dir_eq(A,B) ((A)==(B))
#  define dir_opposite(A,B) ((((A)+2) % 4)==((B) % 4))
#  define dir_parallel(a,b) (((a) % 2)==((b) % 2))
#  define dir_orthogonal(a,b) (((a) % 2)!=((b) % 2))

#  define dir_TL_ 0
#  define dir_TR_ 1
#  define dir_LT_ 2
#  define dir_LB_ 3
#  define dir_BL_ 4
#  define dir_BR_ 5
#  define dir_RT_ 6
#  define dir_RB_ 7

/* font directions */
#  define dir__LT  0
#  define dir__LL  1
#  define dir__LB  2
#  define dir__LR  3
#  define dir__RT  4
#  define dir__RL  5
#  define dir__RB  6
#  define dir__RR  7
#  define dir__TT  8
#  define dir__TL  9
#  define dir__TB 10
#  define dir__TR 11
#  define dir__BT 12
#  define dir__BL 13
#  define dir__BB 14
#  define dir__BR 15

#  define is_mirrored(A) (dir_opposite(dir_primary[(A)],dir_tertiary[(A)]))
#  define font_direction(A) (A % 16)
#  define box_direction(A) (A / 4)
#  define is_rotated(a) dir_parallel(dir_secondary[(a)],dir_tertiary[(a)])


#  define push_dir(a)                           \
  { dir_tmp=new_dir((a));                       \
    vlink(dir_tmp)=dir_ptr; dir_ptr=dir_tmp;    \
    dir_ptr=dir_tmp;                            \
  }

#  define push_dir_node(a)                      \
  { dir_tmp=new_node(whatsit_node,dir_node);    \
    dir_dir(dir_tmp)=dir_dir((a));              \
    dir_level(dir_tmp)=dir_level((a));          \
    dir_dvi_h(dir_tmp)=dir_dvi_h((a));          \
    dir_dvi_ptr(dir_tmp)=dir_dvi_ptr((a));      \
    vlink(dir_tmp)=dir_ptr; dir_ptr=dir_tmp;    \
  }

#  define pop_dir_node()                        \
  { dir_tmp=dir_ptr;                            \
    dir_ptr=vlink(dir_tmp);                     \
    flush_node(dir_tmp);                        \
  }

extern void scan_direction(void);

extern halfword do_push_dir_node(halfword p, halfword a);
extern halfword do_pop_dir_node(halfword p);

#endif
