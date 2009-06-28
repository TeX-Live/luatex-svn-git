/* pdfpage.c
   
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "ptexlib.h"

static const char __svn_version[] =
    "$Id$ "
    "$URL$";

#define lround(a) (long) floor((a) + 0.5)

/* eternal constants */
#define one_bp ((double) 65536 * (double) 72.27 / 72)   /* number of sp per 1bp */

/**********************************************************************/

void pdf_page_init(PDF pdf)
{
    pdfstructure *p;
    assert(pdf != NULL);
    int decimal_digits = pdf->decimal_digits;
    if (pdf->pstruct == NULL)
        pdf->pstruct = xmalloc(sizeof(pdfstructure));
    p = pdf->pstruct;
    setpdffloat(p->pdf.h, 0, decimal_digits);
    setpdffloat(p->pdf.v, 0, decimal_digits);
    p->cw.e = 1;
    p->fs.e = decimal_digits + 2;       /* "+ 2" makes less corrections inside []TJ */
    setpdffloat(p->hz, 1000, 3);        /* m = 1000 = default = unexpanded, e must be 3 */
    setpdffloat(p->ext, 1000, 3);       /* m = 1000 = default = unextended, e must be 3 */
    /* for placement outside BT...ET */
    setpdffloat(p->cm[0], 1, 0);
    setpdffloat(p->cm[1], 0, 0);
    setpdffloat(p->cm[2], 0, 0);
    setpdffloat(p->cm[3], 1, 0);
    setpdffloat(p->cm[4], 0, decimal_digits);   /* horizontal movement on page */
    setpdffloat(p->cm[5], 0, decimal_digits);   /* vertical movement on page */
    /* for placement inside BT...ET */
    setpdffloat(p->tm[0], ten_pow[6], 6);       /* mantissa holds HZ expand * ExtendFont */
    setpdffloat(p->tm[1], 0, 0);
    setpdffloat(p->tm[2], 0, 3);        /* mantissa holds SlantFont, 0 = default */
    setpdffloat(p->tm[3], 1, 0);
    setpdffloat(p->tm[4], 0, decimal_digits);   /* mantissa holds delta from pdf_bt_pos.h */
    setpdffloat(p->tm[5], 0, decimal_digits);   /* mantissa holds delta from pdf_bt_pos.v */
    /*  */
    p->f_cur = null_font;
    p->f_pdf = null_font;
    p->wmode = WMODE_H;
    p->mode = PMODE_PAGE;
    p->ishex = 0;
    p->k1 = ten_pow[p->pdf.h.e] / one_bp;
}

/**********************************************************************/

#ifdef SYNCH_POS_WITH_CUR
/* some time this will be needed */
void synch_pos_with_cur(scaledpos * pos, scaledpos * cur, scaledpos * box_pos)
{
    switch (dvi_direction) {
    case dir_TL_:
        pos->h = box_pos->h + cur->h;
        pos->v = box_pos->v - cur->v;
        break;
    case dir_TR_:
        pos->h = box_pos->h - cur->h;
        pos->v = box_pos->v - cur->v;
        break;
    case dir_BL_:
        pos->h = box_pos->h + cur->h;
        pos->v = box_pos->v + cur->v;
        break;
    case dir_BR_:
        pos->h = box_pos->h - cur->h;
        pos->v = box_pos->v + cur->v;
        break;
    case dir_LT_:
        pos->h = box_pos->h + cur->v;
        pos->v = box_pos->v - cur->h;
        break;
    case dir_RT_:
        pos->h = box_pos->h - cur->v;
        pos->v = box_pos->v - cur->h;
        break;
    case dir_LB_:
        pos->h = box_pos->h + cur->v;
        pos->v = box_pos->v + cur->h;
        break;
    case dir_RB_:
        pos->h = box_pos->h - cur->v;
        pos->v = box_pos->v + cur->h;
        break;
    default:;
    }
}
#endif

/**********************************************************************/

boolean calc_pdfpos(pdfstructure * p, scaledpos pos)
{
    scaledpos new;
    int move_pdfpos = FALSE;
    switch (p->mode) {
    case PMODE_PAGE:
        new.h = lround(pos.h * p->k1);
        new.v = lround(pos.v * p->k1);
        p->cm[4].m = new.h - p->pdf.h.m;        /* cm is concatenated */
        p->cm[5].m = new.v - p->pdf.v.m;
        if (new.h != p->pdf.h.m || new.v != p->pdf.v.m)
            move_pdfpos = TRUE;
        break;
    case PMODE_TEXT:
        new.h = lround(pos.h * p->k1);
        new.v = lround(pos.v * p->k1);
        p->tm[4].m = new.h - p->pdf_bt_pos.h.m; /* Tm replaces */
        p->tm[5].m = new.v - p->pdf_bt_pos.v.m;
        if (new.h != p->pdf.h.m || new.v != p->pdf.v.m)
            move_pdfpos = TRUE;
        break;
    case PMODE_CHAR:
    case PMODE_CHARARRAY:
        switch (p->wmode) {
        case WMODE_H:
            new.h = lround((pos.h * p->k1 - p->pdf_tj_pos.h.m) * p->k2);
            new.v = lround(pos.v * p->k1);
            p->tj_delta.m =
                -lround((new.h - p->cw.m) / ten_pow[p->cw.e - p->tj_delta.e]);
            p->tm[5].m = new.v - p->pdf_bt_pos.v.m;     /* p->tm[4] is meaningless */
            if (p->tj_delta.m != 0 || new.v != p->pdf.v.m)
                move_pdfpos = TRUE;
            break;
        case WMODE_V:
            new.h = lround(pos.h * p->k1);
            new.v = lround((p->pdf_tj_pos.v.m - pos.v * p->k1) * p->k2);
            p->tm[4].m = new.h - p->pdf_bt_pos.h.m;     /* p->tm[5] is meaningless */
            p->tj_delta.m =
                -lround((new.v - p->cw.m) / ten_pow[p->cw.e - p->tj_delta.e]);
            if (p->tj_delta.m != 0 || new.h != p->pdf.h.m)
                move_pdfpos = TRUE;
            break;
        default:
            assert(0);
        }
        break;
    default:
        assert(0);
    }
    return move_pdfpos;
}

/**********************************************************************/

void print_pdffloat(PDF pdf, pdffloat * f)
{
    char a[24];
    int e = f->e, i, j;
    long l, m = f->m;
    if (m < 0) {
        pdf_printf(pdf, "-");
        m *= -1;
    }
    l = m / ten_pow[e];
    pdf_print_int(pdf, l);
    l = m % ten_pow[e];
    if (l != 0) {
        pdf_printf(pdf, ".");
        j = snprintf(a, 23, "%ld", l + ten_pow[e]);
        assert(j < 23);
        for (i = e; i > 0; i--) {
            if (a[i] != '0')
                break;
            a[i] = '\0';
        }
        pdf_printf(pdf, "%s", a + 1);
    }
}

void print_pdf_matrix(PDF pdf, pdffloat * tm)
{
    int i;
    for (i = 0; i < 5; i++) {
        print_pdffloat(pdf, tm + i);
        pdf_printf(pdf, " ");
    }
    print_pdffloat(pdf, tm + i);
}

void pdf_print_cm(PDF pdf, pdffloat * cm)
{
    print_pdf_matrix(pdf, cm);
    pdf_printf(pdf, " cm\n");
}

void pdf_set_pos(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        pdf_print_cm(pdf, p->cm);
        p->pdf.h.m += p->cm[4].m;
        p->pdf.v.m += p->cm[5].m;
    }
}

void pdf_set_pos_temp(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE)
        pdf_print_cm(pdf, p->cm);
}

/**********************************************************************/

static void begin_text(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    p->pdf_bt_pos = p->pdf;
    pdf_printf(pdf, "BT\n");
    p->mode = PMODE_TEXT;
}

static void end_text(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    pdf_printf(pdf, "ET\n");
    p->pdf = p->pdf_bt_pos;
    p->mode = PMODE_PAGE;
}

void pdf_end_string_nl(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    if (is_charmode(p))
        end_charmode(pdf);
    if (is_chararraymode(p))
        end_chararray(pdf);
}

/**********************************************************************/

void pdf_goto_pagemode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(p != NULL);
    if (!is_pagemode(p)) {
        if (is_charmode(p))
            end_charmode(pdf);
        if (is_chararraymode(p))
            end_chararray(pdf);
        if (is_textmode(p))
            end_text(pdf);
        assert(is_pagemode(p));
    }
}

void pdf_goto_textmode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    scaledpos origin = {
        0, 0
    };
    if (!is_textmode(p)) {
        if (is_pagemode(p)) {
            pdf_set_pos(pdf, origin);   /* reset to page origin */
            begin_text(pdf);
        } else {
            if (is_charmode(p))
                end_charmode(pdf);
            if (is_chararraymode(p))
                end_chararray(pdf);
        }
        assert(is_textmode(p));
    }
}
