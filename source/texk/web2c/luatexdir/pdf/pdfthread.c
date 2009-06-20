/* pdfthread.c
   
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

#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define info(A) fixmem[(A)].hhlh

#define pdf_thread_margin        dimen_par(param_pdf_thread_margin_code)

/* Threads are handled in similar way as link annotations */

void append_bead(halfword p)
{
    integer a, b, c, t;
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("threads cannot be inside an XForm"));
    t = get_obj(obj_type_thread, pdf_thread_id(p), pdf_thread_named_id(p));
    b = pdf_new_objnum();
    obj_bead_ptr(b) = pdf_get_mem(pdfmem_bead_size);
    obj_bead_page(b) = pdf_last_page;
    obj_bead_data(b) = p;
    if (pdf_thread_attr(p) != null)
        obj_bead_attr(b) = tokens_to_string(pdf_thread_attr(p));
    else
        obj_bead_attr(b) = 0;
    if (obj_thread_first(t) == 0) {
        obj_thread_first(t) = b;
        obj_bead_next(b) = b;
        obj_bead_prev(b) = b;
    } else {
        a = obj_thread_first(t);
        c = obj_bead_prev(a);
        obj_bead_prev(b) = c;
        obj_bead_next(b) = a;
        obj_bead_prev(a) = b;
        obj_bead_next(c) = b;
    }
    pdf_append_list(b, pdf_bead_list);
}

void do_thread(halfword parent_box, halfword p, scaled x, scaled y)
{
    if (doing_leaders)
        return;
    if (subtype(p) == pdf_start_thread_node) {
        pdf_thread_wd = pdf_width(p);
        pdf_thread_ht = pdf_height(p);
        pdf_thread_dp = pdf_depth(p);
        pdf_last_thread_id = pdf_thread_id(p);
        pdf_last_thread_named_id = (pdf_thread_named_id(p) > 0);
        if (pdf_last_thread_named_id)
            add_token_ref(pdf_thread_id(p));
        pdf_thread_level = cur_s;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(p);
    last_thread = p;
}

void append_thread(halfword parent_box, scaled x, scaled y)
{
    halfword p;
    p = new_node(whatsit_node, pdf_thread_data_node);
    pdf_width(p) = pdf_thread_wd;
    pdf_height(p) = pdf_thread_ht;
    pdf_depth(p) = pdf_thread_dp;
    pdf_thread_attr(p) = null;
    pdf_thread_id(p) = pdf_last_thread_id;
    if (pdf_last_thread_named_id) {
        add_token_ref(pdf_thread_id(p));
        pdf_thread_named_id(p) = 1;
    } else {
        pdf_thread_named_id(p) = 0;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(p);
    last_thread = p;
}

void end_thread(void)
{
    scaledpos tmp1, tmp2;
    if (pdf_thread_level != cur_s)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("\\pdfendthread ended up in different nesting level than \\pdfstartthread"));
    if (is_running(pdf_thread_dp) && (last_thread != null)) {
        tmp1.h = cur.h;
        tmp1.v = cur.v;
        tmp2 = synch_p_with_c(tmp1);
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
        case dir_TR_:
            pdf_ann_bottom(last_thread) = tmp2.v - pdf_thread_margin;
            break;
        case dir_BL_:
        case dir_BR_:
            pdf_ann_top(last_thread) = tmp2.v + pdf_thread_margin;
            break;
        case dir_LT_:
        case dir_LB_:
            pdf_ann_right(last_thread) = tmp2.h + pdf_thread_margin;
            break;
        case dir_RT_:
        case dir_RB_:
            pdf_ann_left(last_thread) = tmp2.h - pdf_thread_margin;
            break;
        }
    }
    if (pdf_last_thread_named_id)
        delete_token_ref(pdf_last_thread_id);
    last_thread = null;
}
