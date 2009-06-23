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

static const char __svn_version[] =
    "$Id$"
    "$URL$";


void output_form(PDF pdf, halfword p)
{
    pdf_goto_pagemode(pdf);
    pdf_place_form(pdf, pos.h, pos.v, obj_info(pdf_xform_objnum(p)));
    if (pdf_lookup_list(pdf_xform_list, pdf_xform_objnum(p)) == null)
        pdf_append_list(pdf_xform_objnum(p), pdf_xform_list);
}
