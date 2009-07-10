/* pdfaction.h

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

#ifndef PDFACTION_H
#  define PDFACTION_H

/* pdf action spec */

#  define pdf_action_size 4

typedef enum {
    pdf_action_page = 0,
    pdf_action_goto,
    pdf_action_thread,
    pdf_action_user
} pdf_action_types;

#  define pdf_action_type(a)        type((a) + 1)
#  define pdf_action_named_id(a)    subtype((a) + 1)
#  define pdf_action_id(a)          vlink((a) + 1)
#  define pdf_action_file(a)        vinfo((a) + 2)
#  define pdf_action_new_window(a)  vlink((a) + 2)
#  define pdf_action_tokens(a)      vinfo((a) + 3)
#  define pdf_action_refcount(a)    vlink((a) + 3)

/*increase count of references to this action*/
#  define add_action_ref(a) pdf_action_refcount((a))++

/* decrease count of references to this
   action; free it if there is no reference to this action*/

#  define delete_action_ref(a) {                                        \
        if (pdf_action_refcount(a) == null) {                           \
            if (pdf_action_type(a) == pdf_action_user) {                \
                delete_token_ref(pdf_action_tokens(a));                 \
            } else {                                                    \
                if (pdf_action_file(a) != null)                         \
                    delete_token_ref(pdf_action_file(a));               \
                if (pdf_action_type(a) == pdf_action_page)              \
                    delete_token_ref(pdf_action_tokens(a));             \
                else if (pdf_action_named_id(a) > 0)                    \
                    delete_token_ref(pdf_action_id(a));                 \
            }                                                           \
            free_node(a, pdf_action_size);                              \
        } else {                                                        \
            decr(pdf_action_refcount(a));                               \
        }                                                               \
    }


#  define set_pdf_action_type(A,B) pdf_action_type(A)=B
#  define set_pdf_action_tokens(A,B) pdf_action_tokens(A)=B
#  define set_pdf_action_file(A,B) pdf_action_file(A)=B
#  define set_pdf_action_id(A,B) pdf_action_id(A)=B
#  define set_pdf_action_named_id(A,B) pdf_action_named_id(A)=B
#  define set_pdf_action_new_window(A,B) pdf_action_new_window(A)=B

extern halfword scan_action (PDF pdf);
extern void write_action(PDF pdf, halfword p);

#endif
