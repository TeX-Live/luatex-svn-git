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

#  define dir_TLT  0
#  define dir_TRT  4
#  define dir_LTL  9
#  define dir_RTT  24

#  define dir_opposite(a,b)   ((((a)+2) % 4)==((b) % 4))
#  define dir_parallel(a,b)   (((a) % 2)==((b) % 2))
#  define dir_orthogonal(a,b) (!dir_parallel((a),(b)))
#  define is_mirrored(a)      (dir_opposite(dir_primary[(a)],dir_tertiary[(a)]))
#  define is_rotated(a)       dir_parallel(dir_secondary[(a)],dir_tertiary[(a)])

#define textdir_parallel(a,b)                           \
   dir_parallel(dir_secondary[(a)], dir_secondary[(b)])

#define pardir_parallel(a,b)                        \
   dir_parallel(dir_primary[(a)], dir_primary[(b)])
   
#define pardir_opposite(a,b)                        \
   dir_opposite(dir_primary[(a)], dir_primary[(b)])

#define textdir_opposite(a,b)                           \
   dir_opposite(dir_secondary[(a)], dir_secondary[(b)])

#define glyphdir_opposite(a,b)                          \
   dir_opposite(dir_tertiary[(a)], dir_tertiary[(b)])

#define pardir_eq(a,b)                          \
   (dir_primary[(a)] == dir_primary[(b)])
   
#define textdir_eq(a,b)                             \
   (dir_secondary[(a)] == dir_secondary[(b)])
   
#define glyphdir_eq(a,b)                        \
   (dir_tertiary[(a)] == dir_tertiary[(b)])

#define partextdir_eq(a,b)                      \
   (dir_primary[(a)] == dir_secondary[(b)])

#define textdir_is(a,b) (dir_secondary[(a)]==(b))


#  define push_dir(a)                           \
   { halfword dir_tmp=new_dir((a));             \
       vlink(dir_tmp)=dir_ptr;                  \
       dir_ptr=dir_tmp;                         \
   }
   
#  define push_dir_node(a)                  \
   { halfword dir_tmp=copy_node((a));		\
       vlink(dir_tmp)=dir_ptr;              \
       dir_ptr=dir_tmp;                     \
   }

#  define pop_dir_node()                        \
   { halfword dir_tmp=dir_ptr;                  \
       dir_ptr=vlink(dir_tmp);                  \
       flush_node(dir_tmp);                     \
   }
   
extern halfword dir_ptr;

extern int dir_primary[32];
extern int dir_secondary[32];
extern int dir_tertiary[32];
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
