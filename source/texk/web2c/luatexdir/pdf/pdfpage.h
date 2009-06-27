/* pdfpage.h

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

#ifndef PDFPAGE_H
#  define PDFPAGE_H

#  define is_pagemode(p)      ((p)->mode == PMODE_PAGE)
#  define is_textmode(p)      ((p)->mode == PMODE_TEXT)
#  define is_chararraymode(p) ((p)->mode == PMODE_CHARARRAY)
#  define is_charmode(p)      ((p)->mode == PMODE_CHAR)

#  define setpdffloat(a,b,c) {(a).m = (b); (a).e = (c);}

#  ifdef hz
/* AIX 4.3 defines hz as 100 in system headers */
#    undef hz
#  endif

/**********************************************************************/

boolean calc_pdfpos(pdfstructure * p, scaledpos pos);

void pdf_page_init(PDF pdf);
void pdf_end_string_nl(PDF pdf);
void pdf_goto_pagemode(PDF pdf);
void pdf_place_glyph(PDF pdf, scaledpos pos, internal_font_number f, integer c);
void pdf_place_rule(PDF pdf, scaled h, scaled v, scaled wd, scaled ht);
void pdf_print_charwidth(PDF pdf, internal_font_number f, int i);
void pdf_print_cm(PDF pdf, pdffloat * cm);
void pdf_set_pos(PDF pdf, scaledpos pos);
void pdf_set_pos_temp(PDF pdf, scaledpos pos);
void pos_finish(PDF pdf, pdfstructure * p);
void print_pdffloat(PDF pdf, pdffloat * f);

#endif
