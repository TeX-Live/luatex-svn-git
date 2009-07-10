/* dvigen.h

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

#ifndef DVIGEN_H
#  define DVIGEN_H

extern integer total_pages;
extern scaled max_v;
extern scaled max_h;
extern integer max_push;
extern integer last_bop;
extern integer dead_cycles;
extern boolean doing_leaders;
extern integer c, f;
extern integer oval, ocmd;
extern halfword g;
extern integer lq, lr;
extern integer cur_s;

typedef int dvi_index;          /* an index into the output buffer */

extern integer dvi_buf_size;
extern eight_bits *dvi_buf;     /* 0 is unused */
extern dvi_index half_buf;
extern dvi_index dvi_limit;
extern dvi_index dvi_ptr;
extern integer dvi_offset;
extern integer dvi_gone;

/*
To put a byte in the buffer without paying the cost of invoking a procedure
each time, we use the macro |dvi_out|.
*/

#  define dvi_out(A) do {				\
    dvi_buf[dvi_ptr++]=A;			\
    if (dvi_ptr==dvi_limit) dvi_swap();		\
  } while (0)


extern void dvi_swap(void);
extern void dvi_four(integer x);
extern void dvi_pop(integer l);
extern void out_cmd(void);
extern void dvi_font_def(internal_font_number f);

#  define dvi_set(A,B)  do {			 \
    synch_dvi_with_pos();			 \
    oval=A; ocmd=set1; out_cmd(); dvi.h += (B);	 \
  } while (0)

#  define dvi_put(A)  do {			\
    synch_dvi_with_pos();			\
    oval=A; ocmd=put1; out_cmd();		\
  } while (0)

#  define dvi_set_rule(A,B)  do {			\
    synch_dvi_with_pos();			\
    dvi_out(set_rule); dvi_four(A);		\
    dvi_four(B); dvi.h += (B);			\
  } while (0)

#  define dvi_put_rule(A,B)  do {	      \
    synch_dvi_with_pos();	      \
    dvi_out(put_rule);		      \
    dvi_four(A); dvi_four(B);	      \
} while (0)


#  define location(A) varmem[(A)+1].cint

extern halfword down_ptr, right_ptr;    /* heads of the down and right stacks */

/*
The |vinfo| fields in the entries of the down stack or the right stack
have six possible settings: |y_here| or |z_here| mean that the \.{DVI}
command refers to |y| or |z|, respectively (or to |w| or |x|, in the
case of horizontal motion); |yz_OK| means that the \.{DVI} command is
\\{down} (or \\{right}) but can be changed to either |y| or |z| (or
to either |w| or |x|); |y_OK| means that it is \\{down} and can be changed
to |y| but not |z|; |z_OK| is similar; and |d_fixed| means it must stay
\\{down}.

The four settings |yz_OK|, |y_OK|, |z_OK|, |d_fixed| would not need to
be distinguished from each other if we were simply solving the
digit-subscripting problem mentioned above. But in \TeX's case there is
a complication because of the nested structure of |push| and |pop|
commands. Suppose we add parentheses to the digit-subscripting problem,
redefining hits so that $\delta_y\ldots \delta_y$ is a hit if all $y$'s between
the $\delta$'s are enclosed in properly nested parentheses, and if the
parenthesis level of the right-hand $\delta_y$ is deeper than or equal to
that of the left-hand one. Thus, `(' and `)' correspond to `|push|'
and `|pop|'. Now if we want to assign a subscript to the final 1 in the
sequence
$$2_y\,7_d\,1_d\,(\,8_z\,2_y\,8_z\,)\,1$$
we cannot change the previous $1_d$ to $1_y$, since that would invalidate
the $2_y\ldots2_y$ hit. But we can change it to $1_z$, scoring a hit
since the intervening $8_z$'s are enclosed in parentheses.
*/

typedef enum {
    y_here = 1,                 /* |vinfo| when the movement entry points to a |y| command */
    z_here = 2,                 /* |vinfo| when the movement entry points to a |z| command */
    yz_OK = 3,                  /* |vinfo| corresponding to an unconstrained \\{down} command */
    y_OK = 4,                   /* |vinfo| corresponding to a \\{down} that can't become a |z| */
    z_OK = 5,                   /* |vinfo| corresponding to a \\{down} that can't become a |y| */
    d_fixed = 6,                /* |vinfo| corresponding to a \\{down} that can't change */
} movement_codes;

/* As we search through the stack, we are in one of three states,
   |y_seen|, |z_seen|, or |none_seen|, depending on whether we have
   encountered |y_here| or |z_here| nodes. These states are encoded as
   multiples of 6, so that they can be added to the |info| fields for quick
   decision-making. */

#  define none_seen 0           /* no |y_here| or |z_here| nodes have been encountered yet */
#  define y_seen 6              /* we have seen |y_here| but not |z_here| */
#  define z_seen 12             /* we have seen |z_here| but not |y_here| */

extern void movement(scaled w, eight_bits o);
extern void prune_movements(integer l);

/*
The actual distances by which we want to move might be computed as the
sum of several separate movements. For example, there might be several
glue nodes in succession, or we might want to move right by the width of
some box plus some amount of glue. More importantly, the baselineskip
distances are computed in terms of glue together with the depth and
height of adjacent boxes, and we want the \.{DVI} file to lump these
three quantities together into a single motion.

Therefore, \TeX\ maintains two pairs of global variables: |dvi.h| and |dvi.v|
are the |h| and |v| coordinates corresponding to the commands actually
output to the \.{DVI} file, while |cur.h| and |cur.v| are the coordinates
corresponding to the current state of the output routines. Coordinate
changes will accumulate in |cur.h| and |cur.v| without being reflected
in the output, until such a change becomes necessary or desirable; we
can call the |movement| procedure whenever we want to make |dvi.h=pos.h|
or |dvi.v=pos.v|.

The current font reflected in the \.{DVI} output is called |dvi_f|;
there is no need for a `\\{cur\_f}' variable.

The depth of nesting of |hlist_out| and |vlist_out| is called |cur_s|;
this is essentially the depth of |push| commands in the \.{DVI} output.
*/

extern scaled_whd rule;

#  define synch_h() do {				\
    if (pos.h != dvi.h) {			\
      movement(pos.h - dvi.h, right1);		\
      dvi.h = pos.h;				\
    }						\
  } while (0)

#  define synch_v() do {				\
    if (pos.v != dvi.v) {			\
      movement(dvi.v - pos.v, down1);		\
      dvi.v = pos.v;				\
    }						\
  } while (0)

#  define synch_dvi_with_pos() do { synch_h(); synch_v(); } while (0)

#  define synch_pos_with_cur() do {					\
    switch (box_direction(dvi_direction)) {				\
    case dir_TL_: pos.h = box_pos.h + cur.h; pos.v = box_pos.v - cur.v; break; \
    case dir_TR_: pos.h = box_pos.h - cur.h; pos.v = box_pos.v - cur.v; break; \
    case dir_BL_: pos.h = box_pos.h + cur.h; pos.v = box_pos.v + cur.v; break; \
    case dir_BR_: pos.h = box_pos.h - cur.h; pos.v = box_pos.v + cur.v; break; \
    case dir_LT_: pos.h = box_pos.h + cur.v; pos.v = box_pos.v - cur.h; break; \
    case dir_RT_: pos.h = box_pos.h - cur.v; pos.v = box_pos.v - cur.h; break; \
    case dir_LB_: pos.h = box_pos.h + cur.v; pos.v = box_pos.v + cur.h; break; \
    case dir_RB_: pos.h = box_pos.h - cur.v; pos.v = box_pos.v + cur.h; break; \
    }									\
  } while (0)

#  define synch_dvi_with_cur()  do {		\
    synch_pos_with_cur();			\
    synch_dvi_with_pos();			\
  } while (0)

#  define set_to_zero(A) do { A.h = 0; A.v = 0; } while (0)

extern scaledpos synch_p_with_c(scaledpos cur);

extern scaledpos cur;
extern scaledpos box_pos;
extern scaledpos pos;
extern scaledpos dvi;
extern internal_font_number dvi_f;
extern scaledpos cur_page_size; /* width and height of page being shipped */

extern integer get_cur_v(void);
extern integer get_cur_h(void);

#  define billion 1000000000.0
#  define vet_glue(A) do { glue_temp=A;		\
    if (glue_temp>billion)			\
      glue_temp=billion;			\
    else if (glue_temp<-billion)		\
      glue_temp=-billion;			\
  } while (0)

extern void hlist_out(void);
extern void vlist_out(void);

extern void expand_macros_in_tokenlist(halfword p);
extern void write_out(halfword p);
extern void out_what(halfword p);
extern void special_out(halfword p);

extern void dvi_ship_out(halfword p);
extern void finish_dvi_file(int version, int revision);

#endif
