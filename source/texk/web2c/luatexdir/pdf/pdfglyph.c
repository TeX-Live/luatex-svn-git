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
    "$Id$"
    "$URL$";

#define lround(a) (long) floor((a) + 0.5)
#define pdf2double(a) ((double) (a).m / ten_pow[(a).e])

/* eternal constants */
#define one_bp ((double) 65536 * (double) 72.27 / 72)   /* number of sp per 1bp */
#define e_tj 3                  /* must be 3; movements in []TJ are in fontsize/10^3 units */

/**********************************************************************/

static long pdf_char_width(pdfstructure * p, internal_font_number f, int i)
{
    /* use exactly this formula also for calculating the /Width array values */
    return
        lround((double) char_width(f, i) / font_size(f) *
               ten_pow[e_tj + p->cw.e]);
}

void pdf_print_charwidth(PDF pdf, internal_font_number f, int i)
{
    pdffloat cw;
    pdfstructure *p = pdf->pstruct;
    assert(pdf_font_blink(f) == null_font);     /* must use unexpanded font! */
    cw.m = pdf_char_width(p, f, i);
    cw.e = p->cw.e;
    print_pdffloat(pdf, cw);
}

/**********************************************************************/

static void setup_fontparameters(PDF pdf, internal_font_number f)
{
    pdfstructure *p = pdf->pstruct;
    pdf->f_cur = f;
    p->f_pdf = pdf_set_font(pdf, f);
    p->tj_delta.e = p->cw.e - 1;        /* "- 1" makes less corrections inside []TJ */
    /* no need to be more precise than TeX (1sp) */
    while (p->tj_delta.e > 0
           && (double) font_size(f) / ten_pow[p->tj_delta.e + e_tj] < 0.5)
        p->tj_delta.e--;        /* happens for very tiny fonts */
    assert(p->cw.e >= p->tj_delta.e);   /* else we would need, e. g., ten_pow[-1] */
    p->fs.m = lround(font_size(f) / one_bp * ten_pow[p->fs.e]);
    p->hz.m = pdf_font_expand_ratio(f) + ten_pow[p->hz.e];
    p->tm[0].m =
        lround(pdf2double(p->hz) * pdf2double(p->ext) * ten_pow[p->tm[0].e]);
    p->tm[2].m = lround(font_slant(f) * (font_extend(f) / 1000.0));     /* tm[2].e = 3 */
    p->k2 =
        ten_pow[e_tj +
                p->cw.e] / (ten_pow[p->pdf.h.e] * pdf2double(p->fs) *
                            pdf2double(p->tm[0]));
}

static void set_font(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    pdf_printf(pdf, "/F%d", (int) p->f_pdf);
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " ");
    print_pdffloat(pdf, p->fs);
    pdf_printf(pdf, " Tf ");
}

static void print_tm(PDF pdf, pdffloat * tm)
{
    print_pdf_matrix(pdf, tm);
    pdf_printf(pdf, " Tm ");
}

static void set_textmatrix(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        print_tm(pdf, p->tm);
        p->pdf.h.m = p->pdf_bt_pos.h.m + p->tm[4].m;    /* Tm replaces */
        p->pdf.v.m = p->pdf_bt_pos.v.m + p->tm[5].m;
    }
}

static void begin_charmode(PDF pdf, internal_font_number f, pdfstructure * p)
{
    assert(is_chararraymode(p));
    if (font_encodingbytes(f) == 2) {
        p->ishex = 1;
        pdf_printf(pdf, "<");
    } else {
        p->ishex = 0;
        pdf_printf(pdf, "(");
    }
    p->mode = PMODE_CHAR;
}

void end_charmode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_charmode(p));
    if (p->ishex == 1) {
        p->ishex = 0;
        pdf_printf(pdf, ">");
    } else {
        pdf_printf(pdf, ")");
    }
    p->mode = PMODE_CHARARRAY;
}

static void begin_chararray(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    p->pdf_tj_pos = p->pdf;
    p->cw.m = 0;
    pdf_printf(pdf, "[");
    p->mode = PMODE_CHARARRAY;
}

void end_chararray(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_chararraymode(p));
    pdf_printf(pdf, "]TJ\n");
    p->pdf = p->pdf_tj_pos;
    p->mode = PMODE_TEXT;
}

/**********************************************************************/

void pdf_place_glyph(PDF pdf, internal_font_number f, int c)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    scaledpos pos = pdf->posstruct->pos;
    if (!char_exists(f, c))
        return;
    if (f != pdf->f_cur || is_textmode(p) || is_pagemode(p)) {
        pdf_goto_textmode(pdf);
        if (f != pdf->f_cur)
            setup_fontparameters(pdf, f);
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
            pdf_goto_textmode(pdf);
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
            print_pdffloat(pdf, p->tj_delta);
            p->cw.m -= p->tj_delta.m * ten_pow[p->cw.e - p->tj_delta.e];
        }
    }
    if (is_chararraymode(p))
        begin_charmode(pdf, f, p);
    pdf_mark_char(f, c);
    if (font_encodingbytes(f) == 2)
        pdf_print_wide_char(pdf, char_index(f, c));
    else
        pdf_print_char(pdf, c);
    p->cw.m += pdf_char_width(p, p->f_pdf, c);  /* aka adv_char_width() */
}
