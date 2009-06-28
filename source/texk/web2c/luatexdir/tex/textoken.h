/* textoken.h
   
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#ifndef TEXTOKEN_H
#  define TEXTOKEN_H

#define token_list 0
#define null 0
#define cs_token_flag 0x1FFFFFFF

#define left_brace_token 0x200000   /* $2^{21}\cdot|left_brace|$ */
#define right_brace_token 0x400000      /* $2^{21}\cdot|right_brace|$ */

typedef struct smemory_word_ {
  halfword hhrh;
  halfword hhlh;
} smemory_word;

#define fix_mem_init 10000

extern smemory_word *fixmem;
extern halfword fix_mem_min;
extern halfword fix_mem_max;

extern integer dyn_used;

#define token_info(a)    fixmem[(a)].hhlh
#define token_link(a)    fixmem[(a)].hhrh
#define set_token_info(a,b) fixmem[(a)].hhlh=(b)
#define set_token_link(a,b) fixmem[(a)].hhrh=(b)

extern halfword avail; /* head of the list of available one-word nodes */
extern halfword fix_mem_end; /* the last one-word node used in |mem| */

extern halfword get_avail (void);

/* A one-word node is recycled by calling |free_avail|.
This routine is part of \TeX's ``inner loop,'' so we want it to be fast.
*/

#define free_avail(A) do { /* single-word node liberation */	\
    token_link(A)=avail; avail=(A); decr(dyn_used);		\
  } while (0)

/*
There's also a |fast_get_avail| routine, which saves the procedure-call
overhead at the expense of extra programming. This routine is used in
the places that would otherwise account for the most calls of |get_avail|.
*/

#define fast_get_avail(A) do {						\
    (A)=avail; /* avoid |get_avail| if possible, to save time */	\
    if ((A)==null)  { (A)=get_avail(); }				\
    else  { avail=token_link((A)); token_link((A))=null; incr(dyn_used); } \
  } while (0)


extern void flush_list(halfword p);
extern void show_token_list(integer p, integer q, integer l);
extern void token_show(halfword p);

#  define token_ref_count(a) token_info((a))  /* reference count preceding a token list */
#  define set_token_ref_count(a,b) token_info((a))=b
#  define add_token_ref(a)   token_ref_count(a)++ /* new reference to a token list */

#  define store_new_token(a) do {				\
    q=get_avail(); token_link(p)=q; token_info(q)=(a); p=q;	\
  } while (0)

#  define fast_store_new_token(a) do {					\
    fast_get_avail(q); token_link(p)=q; token_info(q)=(a); p=q;	\
  } while (0)

extern void delete_token_ref(halfword p);

extern void make_token_table(lua_State *L, int cmd, int chr, int cs);

#endif
