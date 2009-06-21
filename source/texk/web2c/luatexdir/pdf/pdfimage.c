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

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define pdf_pagebox int_par(param_pdf_pagebox_code)
#define pdf_force_pagebox int_par(param_pdf_force_pagebox_code)
#define pdf_option_always_use_pdfpagebox int_par(param_pdf_option_always_use_pdfpagebox_code)
#define pdf_option_pdf_inclusion_errorlevel int_par(param_pdf_option_pdf_inclusion_errorlevel_code)

halfword alt_rule = null;

static boolean warn_pdfpagebox = true;

#define scan_normal_dimen() scan_dimen(false,false,false)

void output_image(integer idx)
{
    pdf_goto_pagemode();
    out_image(idx, pos.h, pos.v);
    if (pdf_lookup_list(pdf_ximage_list, image_objnum(idx)) == null)
        pdf_append_list(image_objnum(idx), pdf_ximage_list);
}

/* write an image */
void pdf_write_image(integer n)
{
    pdf_begin_dict(n, 0);
    if (fixed_pdf_draftmode == 0)
        write_image(obj_data_ptr(n));
}

/* scans PDF pagebox specification */
integer scan_pdf_box_spec (void)
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
void scan_alt_rule (void) 
{
  if (alt_rule == null)
    alt_rule = new_rule();
  width(alt_rule) = null_flag;
  height(alt_rule) = null_flag;
  depth(alt_rule) = null_flag;
RESWITCH:
  if (scan_keyword("width")) {
    scan_normal_dimen();
    width(alt_rule) = cur_val;
    goto RESWITCH;
  }
  if (scan_keyword("height")) {
    scan_normal_dimen();
    height(alt_rule) = cur_val;
    goto RESWITCH;
  }
  if (scan_keyword("depth")) {
    scan_normal_dimen();
    depth(alt_rule) = cur_val;
    goto RESWITCH;
  }
}

void scan_image (void)
{
  integer k, img_wd, img_ht, img_dp, ref;
  str_number named, attr;
  str_number s;
  integer page, pagebox, colorspace;
  incr(pdf_ximage_count);
  pdf_create_obj(obj_type_ximage, pdf_ximage_count);
  k = obj_ptr;
  scan_alt_rule(); /* scans |<rule spec>| to |alt_rule| */
  img_wd = width(alt_rule);
  img_ht = height(alt_rule);
  img_dp = depth(alt_rule);
  attr = 0;
  named = 0;
  page = 1;
  colorspace = 0;
  if (scan_keyword("attr")) {
    scan_pdf_ext_toks();
    attr = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
  }
  if (scan_keyword("named")) {
    scan_pdf_ext_toks();
    named = tokens_to_string(def_ref);
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
  s = tokens_to_string(def_ref);
  delete_token_ref(def_ref);
  if (pdf_option_always_use_pdfpagebox != 0) {
    pdf_warning(maketexstring("PDF inclusion"), 
		maketexstring("Primitive \\pdfoptionalwaysusepdfpagebox is obsolete; " 
			      "use \\pdfpagebox instead."), 
		true, true);
    pdf_force_pagebox = pdf_option_always_use_pdfpagebox;
    pdf_option_always_use_pdfpagebox = 0; /* warn once */
    warn_pdfpagebox = false;
  }
  if (pdf_option_pdf_inclusion_errorlevel != 0) {
    pdf_warning(maketexstring("PDF inclusion"), 
		maketexstring("Primitive \\pdfoptionpdfinclusionerrorlevel is obsolete; "
			      "use \\pdfinclusionerrorlevel instead."), 
		true, true);
    pdf_inclusion_errorlevel = pdf_option_pdf_inclusion_errorlevel;
    pdf_option_pdf_inclusion_errorlevel = 0; /* warn once */
  }
  if (pdf_force_pagebox > 0) {
    if (warn_pdfpagebox) {
      pdf_warning(maketexstring("PDF inclusion"), 
		  maketexstring("Primitive \\pdfforcepagebox is obsolete; " 
				"use \\pdfpagebox instead."), 
		  true, true);
      warn_pdfpagebox = false;
    }
    pagebox = pdf_force_pagebox;
  }
  if (pagebox == 0) /* no pagebox specification given */
    pagebox = pdf_box_spec_crop;
  ref = read_image(k, pdf_ximage_count, s, page, named, attr, colorspace, pagebox,
		   pdf_minor_version, pdf_inclusion_errorlevel);
  flush_str(s);
  set_obj_data_ptr(k, ref);
  if (named != 0)
    flush_str(named);
  set_image_dimensions(ref, img_wd, img_ht, img_dp);
  if (attr != 0) 
    flush_str(attr);
  scale_image(ref);
  pdf_last_ximage = k;
  pdf_last_ximage_pages = image_pages(ref);
  pdf_last_ximage_colordepth = image_colordepth(ref);
}
