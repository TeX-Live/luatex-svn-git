/* pdflink.c
   
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

#define pdf_link_margin          dimen_par(param_pdf_link_margin_code)

halfword pdf_link_list;         /* list of link annotations in the current page */


/*
To implement nested link annotations, we need a stack to hold copy of
|pdf_start_link_node|'s that are being written out, together with their box
nesting level.
*/

pdf_link_stack_record pdf_link_stack[(pdf_max_link_level + 1)];

integer pdf_link_stack_ptr = 0;

void push_link_level(halfword p)
{
    if (pdf_link_stack_ptr >= pdf_max_link_level)
        overflow(maketexstring("pdf link stack size"), pdf_max_link_level);
    assert(((type(p) == whatsit_node) && (subtype(p) == pdf_start_link_node)));
    incr(pdf_link_stack_ptr);
    pdf_link_stack[pdf_link_stack_ptr].nesting_level = cur_s;
    pdf_link_stack[pdf_link_stack_ptr].link_node = copy_node_list(p);
    pdf_link_stack[pdf_link_stack_ptr].ref_link_node = p;
}

void pop_link_level(void)
{
    assert(pdf_link_stack_ptr > 0);
    flush_node_list(pdf_link_stack[pdf_link_stack_ptr].link_node);
    decr(pdf_link_stack_ptr);
}

void do_link(halfword p, halfword parent_box, scaled x, scaled y)
{
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("link annotations cannot be inside an XForm"));
    assert(type(parent_box) == hlist_node);
    if (is_obj_scheduled(pdf_link_objnum(p)))
        pdf_link_objnum(p) = pdf_new_objnum();
    push_link_level(p);
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), pdf_link_margin);
    obj_annot_ptr(pdf_link_objnum(p)) = p;      /* the reference for the pdf annot object must be set here */
    pdf_append_list(pdf_link_objnum(p), pdf_link_list);
    set_obj_scheduled(pdf_link_objnum(p));
}

void end_link(void)
{
    halfword p;
    scaledpos tmp1, tmp2;
    if (pdf_link_stack_ptr < 1)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("pdf_link_stack empty, \\pdfendlink used without \\pdfstartlink?"));
    if (pdf_link_stack[pdf_link_stack_ptr].nesting_level != cur_s)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("\\pdfendlink ended up in different nesting level than \\pdfstartlink"));

    /* N.B.: test for running link must be done on |link_node| and not |ref_link_node|,
       as |ref_link_node| can be set by |do_link| or |append_link| already */

    if (is_running(pdf_width(pdf_link_stack[pdf_link_stack_ptr].link_node))) {
        p = pdf_link_stack[pdf_link_stack_ptr].ref_link_node;
        if (is_shipping_page && matrixused()) {
            matrixrecalculate(cur.h + pdf_link_margin);
            pdf_ann_left(p) = getllx() - pdf_link_margin;
            pdf_ann_top(p) = cur_page_size.v - getury() - pdf_link_margin;
            pdf_ann_right(p) = geturx() + pdf_link_margin;
            pdf_ann_bottom(p) = cur_page_size.v - getlly() + pdf_link_margin;
        } else {
            tmp1.h = cur.h;
            tmp1.v = cur.v;
            tmp2 = synch_p_with_c(tmp1);
            switch (box_direction(dvi_direction)) {
            case dir_TL_:
            case dir_BL_:
                pdf_ann_right(p) = tmp2.h + pdf_link_margin;
                break;
            case dir_TR_:
            case dir_BR_:
                pdf_ann_left(p) = tmp2.h - pdf_link_margin;
                break;
            case dir_LT_:
            case dir_RT_:
                pdf_ann_bottom(p) = tmp2.v - pdf_link_margin;
                break;
            case dir_LB_:
            case dir_RB_:
                pdf_ann_top(p) = tmp2.v + pdf_link_margin;
                break;
            }
        }
    }
    pop_link_level();
}

/*
For ``running'' annotations we must append a new node when the end of
annotation is in other box than its start. The new created node is identical to
corresponding whatsit node representing the start of annotation,  but its
|info| field is |max_halfword|. We set |info| field just before destroying the
node, in order to use |flush_node_list| to do the job.
*/

/* append a new pdf annot to |pdf_link_list| */

void append_link(halfword parent_box, scaled x, scaled y, small_number i)
{
    halfword p;
    assert(type(parent_box) == hlist_node);
    p = copy_node(pdf_link_stack[(int) i].link_node);
    pdf_link_stack[(int) i].ref_link_node = p;
    subtype(p) = pdf_link_data_node;    /* this node is not a normal link node */
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), pdf_link_margin);
    pdf_create_obj(obj_type_others, 0);
    obj_annot_ptr(obj_ptr) = p;
    pdf_append_list(obj_ptr, pdf_link_list);
}
