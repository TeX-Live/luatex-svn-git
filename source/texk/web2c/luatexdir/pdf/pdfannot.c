/* pdfannot.c
   
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

#include "ptexlib.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

halfword pdf_annot_list;        /* list of annotations in the current page */

void do_annot(halfword p, halfword parent_box, scaled x, scaled y)
{
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("annotations cannot be inside an XForm"));
    if (doing_leaders)
        return;
    if (is_obj_scheduled(pdf_annot_objnum(p)))
        pdf_annot_objnum(p) = pdf_new_objnum();
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), 0);
    obj_annot_ptr(pdf_annot_objnum(p)) = p;
    pdf_append_list(pdf_annot_objnum(p), pdf_annot_list);
    set_obj_scheduled(pdf_annot_objnum(p));
}

/* create a new whatsit node for annotation */
void new_annot_whatsit(small_number w)
{
    scaled_whd alt_rule;
    new_whatsit(w);
    alt_rule = scan_alt_rule();            /* scans |<rule spec>| to |alt_rule| */
    set_pdf_width(cur_list.tail_field, alt_rule.w);
    set_pdf_height(cur_list.tail_field, alt_rule.h);
    set_pdf_depth(cur_list.tail_field, alt_rule.d);
    if ((w == pdf_thread_node) || (w == pdf_start_thread_node)) {
        if (scan_keyword("attr")) {
            scan_pdf_ext_toks();
            set_pdf_thread_attr(cur_list.tail_field, def_ref);
        } else {
            set_pdf_thread_attr(cur_list.tail_field, null);
        }
    }
}
