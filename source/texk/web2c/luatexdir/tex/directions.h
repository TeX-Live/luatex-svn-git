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

#  define dir_TLT  0
#  define dir_TL  0
#  define dir_TR  4
#  define dir_LT  9
#  define dir_RT  24
#  define glyph_dir dir_TLT

#  define box_direction(a)    ((a) / 4)
#  define font_direction(a)   ((a) % 16)
#  define dir_eq(a,b)         ((a)==(b))
#  define dir_opposite(a,b)   ((((a)+2) % 4)==((b) % 4))
#  define dir_parallel(a,b)   (((a) % 2)==((b) % 2))
#  define dir_orthogonal(a,b) (!dir_parallel((a),(b)))
#  define is_mirrored(a)      (dir_opposite(dir_primary[(a)],dir_tertiary[(a)]))
#  define is_rotated(a)       dir_parallel(dir_secondary[(a)],dir_tertiary[(a)])
#  define line_horizontal(a)  dir_parallel(dir_secondary[(a)],dir_secondary[dir_TLT])
#  define line_vertical(a)    (!line_horizontal(a))

#  define push_dir(a)                           \
   { halfword dir_tmp=new_dir((a));		\
    vlink(dir_tmp)=dir_ptr;			\
    dir_ptr=dir_tmp;                            \
  }

#  define push_dir_node(a)                      \
   { halfword dir_tmp=copy_node((a));		\
     vlink(dir_tmp)=dir_ptr;			\
     dir_ptr=dir_tmp;				\
  }

#  define pop_dir_node()                        \
   { halfword dir_tmp=dir_ptr;			\
    dir_ptr=vlink(dir_tmp);                     \
    flush_node(dir_tmp);                        \
  }

extern halfword dir_ptr;

extern integer dvi_direction;
extern int dir_primary[32];
extern int dir_secondary[32];
extern int dir_tertiary[32];
extern str_number dir_names[4];
extern halfword text_dir_ptr;

extern void initialize_directions(void);
extern halfword new_dir(int s);
extern void print_dir(int d);

extern void scan_direction(void);

extern halfword do_push_dir_node(halfword p, halfword a);
extern halfword do_pop_dir_node(halfword p);

scaled pack_width(int curdir, int pdir, halfword p, boolean isglyph);
scaled_whd pack_width_height_depth(int curdir, int pdir, halfword p,
                                   boolean isglyph);

#endif
