/* pdfglyph.c
   
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

static const char __svn_version[] =
    "$Id: pdfrule.c 2645 2009-06-27 12:01:54Z oneiros $"
    "$URL: http://foundry.supelec.fr/svn/luatex/trunk/source/texk/web2c/luatexdir/pdf/pdfrule.c $";

/* not useful yet, selected stuff from pdfpage.c should go here, TODO... */

void pdf_place_glyph(PDF pdf, scaledpos pos, internal_font_number f, integer c)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    if (!char_exists(f, c))
        return;
    if (f != p->f_cur || is_textmode(p) || is_pagemode(p)) {
        goto_textmode(pdf);
        if (f != p->f_cur)
            setup_fontparameters(p, f);
        set_font(pdf);
        set_textmatrix(pdf, pos);
        begin_chararray(pdf);
    }
    assert(is_charmode(p) || is_chararraymode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        if ((p->wmode == WMODE_H
             && (p->pdf_bt_pos.v.m + p->tm[5].m) != p->pdf.v.m)
            || (p->wmode == WMODE_V
                && (p->pdf_bt_pos.h.m + p->tm[4].m) != p->pdf.h.m)
            || abs(p->tj_delta.m) >= 1000000) {
            goto_textmode(pdf);
            set_textmatrix(pdf, pos);
            begin_chararray(pdf);
            move = calc_pdfpos(p, pos);
        }
        if (move == TRUE) {
            assert((p->wmode == WMODE_H
                    && (p->pdf_bt_pos.v.m + p->tm[5].m) == p->pdf.v.m)
                   || (p->wmode == WMODE_V
                       && (p->pdf_bt_pos.h.m + p->tm[4].m) == p->pdf.h.m));
            if (is_charmode(p))
                end_charmode(pdf);
            assert(p->tj_delta.m != 0);
            print_pdffloat(pdf, &(p->tj_delta));
            p->cw.m -= p->tj_delta.m * ten_pow[p->cw.e - p->tj_delta.e];
        }
    }
    if (is_chararraymode(p))
        begin_charmode(pdf, f, p);
    pdf_print_char(pdf, f, c);  /* this also does pdf_mark_char() */
    p->cw.m += pdf_char_width(p, p->f_pdf, c);  /* aka adv_char_width() */
}
