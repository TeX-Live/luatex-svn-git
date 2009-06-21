/* pdfoutline.h

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

#ifndef PDFOUTLINE_H
#  define PDFOUTLINE_H

/* Data structure of outlines; it's not able to write out outline entries
before all outline entries are defined, so memory allocated for outline
entries can't not be deallocated and will stay in memory. For this reason we
will store data of outline entries in |pdf_mem| instead of |mem|
*/

#  define pdfmem_outline_size      8    /* size of memory in |pdf_mem| which |obj_outline_ptr| points to */

#  define obj_outline_count         obj_info    /* count of all opened children */
#  define obj_outline_ptr           obj_aux     /* pointer to |pdf_mem| */
#  define obj_outline_title(A)      pdf_mem[obj_outline_ptr(A)]
#  define obj_outline_parent(A)     pdf_mem[obj_outline_ptr(A) + 1]
#  define obj_outline_prev(A)       pdf_mem[obj_outline_ptr(A) + 2]
#  define obj_outline_next(A)       pdf_mem[obj_outline_ptr(A) + 3]
#  define obj_outline_first(A)      pdf_mem[obj_outline_ptr(A) + 4]
#  define obj_outline_last(A)       pdf_mem[obj_outline_ptr(A) + 5]
#  define obj_outline_action_objnum(A)  pdf_mem[obj_outline_ptr(A) + 6] /* object number of action */
#  define obj_outline_attr(A)       pdf_mem[obj_outline_ptr(A) + 7]

#  define set_obj_outline_ptr(A,B) obj_outline_ptr(A)=B
#  define set_obj_outline_action_objnum(A,B) obj_outline_action_objnum(A)=B
#  define set_obj_outline_count(A,B) obj_outline_count(A)=B
#  define set_obj_outline_title(A,B) obj_outline_title(A)=B
#  define set_obj_outline_prev(A,B) obj_outline_prev(A)=B
#  define set_obj_outline_next(A,B) obj_outline_next(A)=B
#  define set_obj_outline_first(A,B) obj_outline_first(A)=B
#  define set_obj_outline_last(A,B) obj_outline_last(A)=B
#  define set_obj_outline_parent(A,B) obj_outline_parent(A)=B
#  define set_obj_outline_attr(A,B) obj_outline_attr(A)=B

extern integer open_subentries(halfword p);
extern void scan_pdfoutline (void) ;

#endif
