/* pdflink.h

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

#ifndef PDFLINK_H
#  define PDFLINK_H

#  define set_pdf_link_attr(A,B) pdf_link_attr(A)=B
#  define set_pdf_link_action(A,B) pdf_link_action(A)=B
#  define set_pdf_link_objnum(A,B) pdf_link_objnum(A)=B

#  define pdf_max_link_level  10        /* maximum depth of link nesting */

extern halfword pdf_link_list;  /* list of link annotations in the current page */

typedef struct pdf_link_stack_record {
    integer nesting_level;
    halfword link_node;         /* holds a copy of the corresponding |pdf_start_link_node| */
    halfword ref_link_node;     /* points to original |pdf_start_link_node|, or a
                                   copy of |link_node| created by |append_link| in
                                   case of multi-line link */
} pdf_link_stack_record;

extern pdf_link_stack_record pdf_link_stack[(pdf_max_link_level + 1)];
extern integer pdf_link_stack_ptr;

extern void push_link_level(halfword p);
extern void pop_link_level(void);
extern void do_link(PDF pdf, halfword p, halfword parent_box, scaled x, scaled y);
extern void end_link(void);
extern void append_link(PDF pdf, halfword parent_box, scaled x, scaled y,
                        small_number i);

extern void scan_startlink (PDF pdf) ;


#endif
