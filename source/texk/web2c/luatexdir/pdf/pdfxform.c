/* pdfxform.c
   
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
#include "pdfpage.h"

#define box(A) zeqtb[box_base+(A)].hh.rh

static const char __svn_version[] =
    "$Id$"
    "$URL$";

halfword pdf_xform_list;        /* list of forms in the current page */
integer pdf_xform_count;        /* counter of forms */
integer pdf_cur_form;           /* the form being output */

void pdf_place_form(PDF pdf, integer objnum, scaledpos pos)
{
    pdf_goto_pagemode(pdf);
    pdf_printf(pdf, "q\n");
    pdf_set_pos_temp(pdf, pos);
    pdf_printf(pdf, "/Fm%d", (int) obj_info(pdf, objnum));
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " Do\nQ\n");
    if (pdf_lookup_list(pdf_xform_list, objnum) == null)
        pdf_append_list(objnum, pdf_xform_list);
}

/* todo: the trick with |box_base| is a cludge */
void scan_pdfxform(PDF pdf, integer box_base)
{
    integer k;
    halfword p;
    incr(pdf_xform_count);
    pdf_create_obj(pdf, obj_type_xform, pdf_xform_count);
    k = pdf->obj_ptr;
    set_obj_data_ptr(pdf, k, pdf_get_mem(pdf, pdfmem_xform_size));
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        set_obj_xform_attr(pdf, k, def_ref);
    } else {
        set_obj_xform_attr(pdf, k, null);
    }
    if (scan_keyword("resources")) {
        scan_pdf_ext_toks();
        set_obj_xform_resources(pdf, k, def_ref);
    } else {
        set_obj_xform_resources(pdf, k, null);
    }
    scan_int();
    p = box(cur_val);
    if (p == null)
        pdf_error("ext1", "\\pdfxform cannot be used with a void box");
    set_obj_xform_width(pdf, k, width(p));
    set_obj_xform_height(pdf, k, height(p));
    set_obj_xform_depth(pdf, k, depth(p));
    set_obj_xform_box(pdf, k, p);       /* save pointer to the box */
    box(cur_val) = null;
    pdf_last_xform = k;
}


void scan_pdfrefxform(PDF pdf)
{
    scan_int();
    pdf_check_obj(pdf, obj_type_xform, cur_val);
    new_whatsit(pdf_refxform_node);
    set_pdf_xform_objnum(cur_list.tail_field, cur_val);
    set_pdf_width(cur_list.tail_field, obj_xform_width(pdf, cur_val));
    set_pdf_height(cur_list.tail_field, obj_xform_height(pdf, cur_val));
    set_pdf_depth(cur_list.tail_field, obj_xform_depth(pdf, cur_val));
}
