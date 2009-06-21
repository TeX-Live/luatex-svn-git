/* pdfthread.h

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

#ifndef PDFTHREAD_H
#  define PDFTHREADH

/* data structure of threads; words 1..4 represent the coordinates of the  corners */

#  define obj_thread_first           obj_aux    /* pointer to the first bead */

/* data structure of beads */
#  define pdfmem_bead_size          5   /* size of memory in |pdf_mem| which  |obj_bead_ptr| points to */

#  define obj_bead_ptr              obj_aux     /* pointer to |pdf_mem| */
#  define obj_bead_rect(A)          pdf_mem[obj_bead_ptr(A)]
#  define obj_bead_page(A)          pdf_mem[obj_bead_ptr(A) + 1]
#  define obj_bead_next(A)          pdf_mem[obj_bead_ptr(A) + 2]
#  define obj_bead_prev(A)          pdf_mem[obj_bead_ptr(A) + 3]
#  define obj_bead_attr(A)          pdf_mem[obj_bead_ptr(A) + 4]
#  define set_obj_bead_rect(A,B) obj_bead_rect(A)=B

#  define set_pdf_thread_attr(A,B) pdf_thread_attr(A)=B
#  define set_pdf_thread_id(A,B) pdf_thread_id(A)=B
#  define set_pdf_thread_named_id(A,B) pdf_thread_named_id(A)=B

/* pointer to the corresponding whatsit node; |obj_bead_rect| is needed only when the bead
   rectangle has been written out and after that |obj_bead_data| is not needed any more
   so we can use this field for both */
#  define obj_bead_data             obj_bead_rect

extern void append_bead(halfword p);
extern void do_thread(halfword parent_box, halfword p, scaled x, scaled y);
extern void append_thread(halfword parent_box, scaled x, scaled y);
extern void end_thread(void);

extern void thread_title(integer t);
extern void pdf_fix_thread(integer t);
extern void out_thread(integer t);
extern void scan_thread_id (void);

#endif
