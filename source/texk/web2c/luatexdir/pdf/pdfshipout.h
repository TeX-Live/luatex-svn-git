/* pdfshipout.h

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

#ifndef PDFSHIPOUT_H
#  define PDFSHIPOUT_H

extern integer page_divert_val;
extern halfword pdf_info_toks; /* additional keys of Info dictionary */
extern halfword pdf_catalog_toks; /* additional keys of Catalog dictionary */
extern halfword pdf_catalog_openaction;
extern halfword pdf_names_toks; /* additional keys of Names dictionary */
extern halfword pdf_trailer_toks; /* additional keys of Trailer dictionary */
extern boolean is_shipping_page; /* set to |shipping_page| when |pdf_ship_out| starts */

extern void fix_pdfoutput(void);
extern void pdf_ship_out(PDF pdf, halfword p, boolean shipping_page);
extern void print_pdf_pages_attr(PDF pdf);
extern void finish_pdf_file(PDF pdf, integer maj, str_number min);

extern boolean str_less_str(str_number s1, str_number s2);

#endif
