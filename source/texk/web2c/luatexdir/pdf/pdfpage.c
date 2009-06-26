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
#define setpdffloat(a,b,c) {(a).m = (b); (a).e = (c);}
#define pdf2double(a) ((double) (a).m / ten_pow[(a).e])

/* eternal constants */
#define one_bp ((double) 65536 * (double) 72.27 / 72)   /* number of sp per 1bp */
#define e_tj 3                  /* must be 3; movements in []TJ are in fontsize/10^3 units */

/* definitions from luatex.web */
#define pdf_font_blink(a) font_tables[a]->_pdf_font_blink

pdfstructure *pstruct = NULL;

/**********************************************************************/

static pdfstructure *new_pdfstructure(void)
{
    return xmalloc(sizeof(pdfstructure));
}

static void calc_k1(pdfstructure * p)
{
    p->k1 = ten_pow[p->pdf.h.e] / one_bp;
}

static void calc_k2(pdfstructure * p)
{
    p->tm[0].m =
        lround(pdf2double(p->hz) * pdf2double(p->ext) * ten_pow[p->tm[0].e]);
    p->k2 =
        ten_pow[e_tj +
                p->cw.e] / (ten_pow[p->pdf.h.e] * pdf2double(p->fs) *
                            pdf2double(p->tm[0]));
}

void pdf_page_init(PDF pdf)
{
    pdfstructure *p;
    int decimal_digits = pdf->decimal_digits;
    if (pstruct == NULL)
        pstruct = new_pdfstructure();
    p = pstruct;
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
    p->ishex = false;
    calc_k1(p);
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

static void print_pdf_matrix(PDF pdf, pdffloat * tm)
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

static void pdf_print_tm(PDF pdf, pdffloat * tm)
{
    print_pdf_matrix(pdf, tm);
    pdf_printf(pdf, " Tm ");
}

static void set_pos(PDF pdf, pdfstructure * p, scaledpos pos)
{
    boolean move;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        pdf_print_cm(pdf, p->cm);
        p->pdf.h.m += p->cm[4].m;
        p->pdf.v.m += p->cm[5].m;
    }
}

static void set_pos_temp(PDF pdf, pdfstructure * p, scaledpos pos)
{
    boolean move;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE)
        pdf_print_cm(pdf, p->cm);
}

void pdf_set_pos(PDF pdf, scaledpos pos)
{
    set_pos(pdf, pstruct, pos);
}

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
    assert(pdf_font_blink(f) == null_font);     /* must use unexpanded font! */
    cw.m = pdf_char_width(pstruct, f, i);
    cw.e = pstruct->cw.e;
    print_pdffloat(pdf, &cw);
}

/**********************************************************************/

static void begin_text(PDF pdf, pdfstructure * p)
{
    assert(is_pagemode(p));
    p->pdf_bt_pos = p->pdf;
    pdf_printf(pdf, "BT\n");
    p->mode = PMODE_TEXT;
}

static void end_text(PDF pdf, pdfstructure * p)
{
    assert(is_textmode(p));
    pdf_printf(pdf, "ET\n");
    p->pdf = p->pdf_bt_pos;
    p->mode = PMODE_PAGE;
}

static void begin_charmode(PDF pdf, internal_font_number f, pdfstructure * p)
{
    assert(is_chararraymode(p));
    if (font_encodingbytes(f)==2) {
        p->ishex = true;
        pdf_printf(pdf, "<");
    } else {
        p->ishex = false;
        pdf_printf(pdf, "(");
    }
    p->mode = PMODE_CHAR;
}

static void end_charmode(PDF pdf, pdfstructure * p)
{
    assert(is_charmode(p));
    if (p->ishex) {
        p->ishex = false;
        pdf_printf(pdf, ">");
    } else {
        pdf_printf(pdf, ")");
    }
    p->mode = PMODE_CHARARRAY;
}

static void begin_chararray(PDF pdf, pdfstructure * p)
{
    assert(is_textmode(p));
    p->pdf_tj_pos = p->pdf;
    p->cw.m = 0;
    pdf_printf(pdf, "[");
    p->mode = PMODE_CHARARRAY;
}

static void end_chararray(PDF pdf, pdfstructure * p)
{
    assert(is_chararraymode(p));
    pdf_printf(pdf, "]TJ\n");
    p->pdf = p->pdf_tj_pos;
    p->mode = PMODE_TEXT;
}

void pdf_end_string_nl(PDF pdf)
{
    if (is_charmode(pstruct))
        end_charmode(pdf, pstruct);
    if (is_chararraymode(pstruct))
        end_chararray(pdf, pstruct);
}

/**********************************************************************/

static void goto_pagemode(PDF pdf, pdfstructure * p)
{
    assert(p != NULL);
    if (!is_pagemode(p)) {
        if (is_charmode(p))
            end_charmode(pdf, p);
        if (is_chararraymode(p))
            end_chararray(pdf, p);
        if (is_textmode(p))
            end_text(pdf, p);
        assert(is_pagemode(p));
    }
}

void pdf_goto_pagemode(PDF pdf)
{
    goto_pagemode(pdf, pstruct);
}

static void goto_textmode(PDF pdf, pdfstructure * p)
{
    scaledpos origin = {
        0, 0
    };
    if (!is_textmode(p)) {
        if (is_pagemode(p)) {
            set_pos(pdf, p, origin);    /* reset to page origin */
            begin_text(pdf, p);
        } else {
            if (is_charmode(p))
                end_charmode(pdf, p);
            if (is_chararraymode(p))
                end_chararray(pdf, p);
        }
        assert(is_textmode(p));
    }
}

void pos_finish(PDF pdf, pdfstructure * p)
{
    goto_pagemode(pdf, p);
}

/**********************************************************************/

static void place_rule(PDF pdf,
                       pdfstructure * p, scaledpos pos, scaled wd, scaled ht)
{
    pdfpos dim;
    goto_pagemode(pdf, p);
    dim.h.m = lround(wd * p->k1);
    dim.h.e = p->pdf.h.e;
    dim.v.m = lround(ht * p->k1);
    dim.v.e = p->pdf.v.e;
    pdf_printf(pdf, "q\n");
    if (ht <= one_bp) {
        pos.v += lround(0.5 * ht);
        set_pos_temp(pdf, p, pos);
        pdf_printf(pdf, "[]0 d 0 J ");
        print_pdffloat(pdf, &(dim.v));
        pdf_printf(pdf, " w 0 0 m ");
        print_pdffloat(pdf, &(dim.h));
        pdf_printf(pdf, " 0 l S\n");
    } else if (wd <= one_bp) {
        pos.h += lround(0.5 * wd);
        set_pos_temp(pdf, p, pos);
        pdf_printf(pdf, "[]0 d 0 J ");
        print_pdffloat(pdf, &(dim.h));
        pdf_printf(pdf, " w 0 0 m 0 ");
        print_pdffloat(pdf, &(dim.v));
        pdf_printf(pdf, " l S\n");
    } else {
        set_pos_temp(pdf, p, pos);
        pdf_printf(pdf, "0 0 ");
        print_pdffloat(pdf, &(dim.h));
        pdf_printf(pdf, " ");
        print_pdffloat(pdf, &(dim.v));
        pdf_printf(pdf, " re f\n");
    }
    pdf_printf(pdf, "Q\n");
}

void pdf_place_rule(PDF pdf, scaled h, scaled v, scaled wd, scaled ht)
{
    scaledpos pos;
    pos.h = h;
    pos.v = v;
    place_rule(pdf, pstruct, pos, wd, ht);
}

/**********************************************************************/

static void setup_fontparameters(pdfstructure * p, internal_font_number f)
{
    p->f_cur = f;
    p->f_pdf = pdf_set_font(f);
    p->tj_delta.e = p->cw.e - 1;        /* "- 1" makes less corrections inside []TJ */
    /* no need to be more precise than TeX (1sp) */
    while (p->tj_delta.e > 0
           && (double) font_size(f) / ten_pow[p->tj_delta.e + e_tj] < 0.5)
        p->tj_delta.e--;        /* happens for very tiny fonts */
    assert(p->cw.e >= p->tj_delta.e);   /* else we would need, e. g., ten_pow[-1] */
    p->fs.m = lround(font_size(f) / one_bp * ten_pow[p->fs.e]);
    p->hz.m = pdf_font_expand_ratio(f) + ten_pow[p->hz.e];
    calc_k2(p);
}

static void set_textmatrix(PDF pdf, pdfstructure * p, scaledpos pos)
{
    boolean move;
    assert(is_textmode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        pdf_print_tm(pdf, p->tm);
        p->pdf.h.m = p->pdf_bt_pos.h.m + p->tm[4].m;    /* Tm replaces */
        p->pdf.v.m = p->pdf_bt_pos.v.m + p->tm[5].m;
    }
}

static void set_font(PDF pdf, pdfstructure * p)
{
    pdf_printf(pdf, "/F%d", (int) p->f_pdf);
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " ");
    print_pdffloat(pdf, &(p->fs));
    pdf_printf(pdf, " Tf ");
}

/**********************************************************************/

static void
place_glyph(PDF pdf,
            pdfstructure * p, scaledpos pos, internal_font_number f, integer c)
{
    boolean move;
    if (f != p->f_cur || is_textmode(p) || is_pagemode(p)) {
        goto_textmode(pdf, p);
        if (f != p->f_cur)
            setup_fontparameters(p, f);
        set_font(pdf, p);
        set_textmatrix(pdf, p, pos);
        begin_chararray(pdf, p);
    }
    assert(is_charmode(p) || is_chararraymode(p));
    move = calc_pdfpos(p, pos);
    if (move == TRUE) {
        if ((p->wmode == WMODE_H
             && (p->pdf_bt_pos.v.m + p->tm[5].m) != p->pdf.v.m)
            || (p->wmode == WMODE_V
                && (p->pdf_bt_pos.h.m + p->tm[4].m) != p->pdf.h.m)
            || abs(p->tj_delta.m) >= 1000000) {
            goto_textmode(pdf, p);
            set_textmatrix(pdf, p, pos);
            begin_chararray(pdf, p);
            move = calc_pdfpos(p, pos);
        }
        if (move == TRUE) {
            assert((p->wmode == WMODE_H
                    && (p->pdf_bt_pos.v.m + p->tm[5].m) == p->pdf.v.m)
                   || (p->wmode == WMODE_V
                       && (p->pdf_bt_pos.h.m + p->tm[4].m) == p->pdf.h.m));
            if (is_charmode(p))
                end_charmode(pdf, p);
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

void pdf_place_glyph(PDF pdf, internal_font_number f, integer c)
{
    if (char_exists(f, c))
        place_glyph(pdf, pstruct, pos, f, c);
}

/**********************************************************************/

static void place_form(PDF pdf, pdfstructure * p, integer i, scaledpos pos)
{
    goto_pagemode(pdf, p);
    pdf_printf(pdf, "q\n");
    set_pos_temp(pdf, p, pos);
    pdf_printf(pdf, "/Fm%d", (int) i);
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " Do\nQ\n");
}

void pdf_place_form(PDF pdf, integer i, scaledpos pos)
{
    place_form(pdf, pstruct, i, pos);
}
