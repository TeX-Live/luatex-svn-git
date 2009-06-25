/* pdfimage.c
   
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
#include "luatex-api.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define pdf_pagebox int_par(param_pdf_pagebox_code)

halfword pdf_ximage_list;       /* list of images in the current page */
integer pdf_ximage_count;       /* counter of images */
integer image_orig_x, image_orig_y;     /* origin of cropped PDF images */

#define scan_normal_dimen() scan_dimen(false,false,false)

void output_image(PDF pdf, integer idx)
{
    pdf_goto_pagemode(pdf);
    out_image(pdf, idx, pos.h, pos.v);
    if (pdf_lookup_list(pdf_ximage_list, image_objnum(idx)) == null)
        pdf_append_list(image_objnum(idx), pdf_ximage_list);
}

/* write an image */
void pdf_write_image(PDF pdf, integer n)
{
    pdf_begin_dict(pdf, n, 0);
    if (pdf->draftmode == 0)
        write_image(pdf, obj_data_ptr(n));
}

/* scans PDF pagebox specification */
integer scan_pdf_box_spec(void)
{
    if (scan_keyword("mediabox"))
        return pdf_box_spec_media;
    else if (scan_keyword("cropbox"))
        return pdf_box_spec_crop;
    else if (scan_keyword("bleedbox"))
        return pdf_box_spec_bleed;
    else if (scan_keyword("trimbox"))
        return pdf_box_spec_trim;
    else if (scan_keyword("artbox"))
        return pdf_box_spec_art;
    return 0;
}

/* scans rule spec to |alt_rule| */
scaled_whd scan_alt_rule(void)
{
    scaled_whd alt_rule;
    alt_rule.w = null_flag;
    alt_rule.h = null_flag;
    alt_rule.d = null_flag;
  RESWITCH:
    if (scan_keyword("width")) {
        scan_normal_dimen();
        alt_rule.w = cur_val;
        goto RESWITCH;
    }
    if (scan_keyword("height")) {
        scan_normal_dimen();
        alt_rule.h = cur_val;
        goto RESWITCH;
    }
    if (scan_keyword("depth")) {
        scan_normal_dimen();
        alt_rule.d = cur_val;
        goto RESWITCH;
    }
    return alt_rule;
}

void scan_image(PDF pdf)
{
scaled_whd alt_rule;
    integer k, ref;
    integer page, pagebox, colorspace;
    char *named = NULL, *attr = NULL, *s = NULL;
    incr(pdf_ximage_count);
    pdf_create_obj(obj_type_ximage, pdf_ximage_count);
    k = obj_ptr;
    alt_rule= scan_alt_rule();            /* scans |<rule spec>| to |alt_rule| */
    attr = 0;
    named = 0;
    page = 1;
    colorspace = 0;
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        attr = tokenlist_to_cstring(def_ref, true, NULL);
        delete_token_ref(def_ref);
    }
    if (scan_keyword("named")) {
        scan_pdf_ext_toks();
        named = tokenlist_to_cstring(def_ref, true, NULL);
        delete_token_ref(def_ref);
        page = 0;
    } else if (scan_keyword("page")) {
        scan_int();
        page = cur_val;
    }
    if (scan_keyword("colorspace")) {
        scan_int();
        colorspace = cur_val;
    }
    pagebox = scan_pdf_box_spec();
    if (pagebox == 0)
        pagebox = pdf_pagebox;
    scan_pdf_ext_toks();
    s = tokenlist_to_cstring(def_ref, true, NULL);
    assert(s != NULL);
    delete_token_ref(def_ref);
    if (pagebox == 0)           /* no pagebox specification given */
        pagebox = pdf_box_spec_crop;
    ref =
        read_image(pdf,
                   k, pdf_ximage_count, s, page, named, attr, colorspace,
                   pagebox, pdf_minor_version, pdf_inclusion_errorlevel);
    xfree(s);
    set_obj_data_ptr(k, ref);
    if (named != NULL)
        xfree(named);
    set_image_dimensions(ref, alt_rule.w, alt_rule.h, alt_rule.d);
    if (attr != NULL)
        xfree(attr);
    scale_image(ref);
    pdf_last_ximage = k;
    pdf_last_ximage_pages = image_pages(ref);
    pdf_last_ximage_colordepth = image_colordepth(ref);
}
