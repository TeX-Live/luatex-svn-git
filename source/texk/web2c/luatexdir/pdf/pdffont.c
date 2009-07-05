/* pdffont.c
 
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

#define text(A) hash[(A)].rh
#define font_id_text(A) text(font_id_base+(A))  /* a frozen font identifier's name */
#define define(A,B,C) do { if (a>=4) geq_define(A,B,C); else eq_define(A,B,C); } while (0)

integer pk_dpi;                 /* PK pixel density value from \.{texmf.cnf} */

/*
As \pdfTeX{} should also act as a back-end driver, it needs to support virtual
fonts too. Information about virtual fonts can be found in the source of some
\.{DVI}-related programs.

Whenever we want to write out a character in a font to PDF output, we
should check whether the used character is a virtual or read character.
The |has_packet()| C macro checks for this condition.
*/

/* The following code typesets a character to PDF output */

scaled output_one_char(PDF pdf, internal_font_number ffi, integer c)
{
    scaled_whd ci;              /* the real width, height and depth of the character */
    posstructure localpos = *(pdf->posstruct);  /* for the final transform */
    ci = get_charinfo_whd(ffi, c);

    switch (box_direction(localpos.dir)) {
    case dir_TL_:
        switch (font_direction(localpos.dir)) {
        case dir__LL:
        case dir__LR:
            lpos_down((ci.ht - ci.dp) / 2);
            break;
        case dir__LT:
            break;
        case dir__LB:
            lpos_down(ci.ht);
            break;
        }
        break;
    case dir_TR_:
        lpos_left(ci.wd);
        switch (font_direction(localpos.dir)) {
        case dir__RL:
        case dir__RR:
            lpos_down((ci.ht - ci.dp) / 2);
            break;
        case dir__RT:
            break;
        case dir__RB:
            lpos_down(ci.ht);
            break;
        }
        break;
    case dir_BL_:
        switch (font_direction(localpos.dir)) {
        case dir__LL:
        case dir__LR:
            lpos_down((ci.ht - ci.dp) / 2);
            break;
        case dir__LT:
            break;
        case dir__LB:
            lpos_down(ci.ht);
            break;
        }
        break;
    case dir_BR_:
        lpos_left(ci.wd);
        switch (font_direction(localpos.dir)) {
        case dir__RL:
        case dir__RR:
            lpos_down((ci.ht - ci.dp) / 2);
            break;
        case dir__RT:
            break;
        case dir__RB:
            lpos_down(ci.ht);
            break;
        }
        break;
    case dir_LT_:
        lpos_down(ci.ht);
        switch (font_direction(localpos.dir)) {
        case dir__TL:
            lpos_left(ci.wd);
            break;
        case dir__TR:
            break;
        case dir__TB:
        case dir__TT:
            lpos_left(ci.wd / 2);
            break;
        }
        break;
    case dir_RT_:
        lpos_down(ci.ht);
        switch (font_direction(localpos.dir)) {
        case dir__TL:
            lpos_left(ci.wd);
            break;
        case dir__TR:
            break;
        case dir__TB:
        case dir__TT:
            lpos_left(ci.wd / 2);
            break;
        }
        break;
    case dir_LB_:
        lpos_up(ci.dp);
        switch (font_direction(localpos.dir)) {
        case dir__BL:
            lpos_left(ci.wd);
            break;
        case dir__BR:
            break;
        case dir__BB:
        case dir__BT:
            lpos_left(ci.wd / 2);
            break;
        }
        break;
    case dir_RB_:
        lpos_up(ci.dp);
        switch (font_direction(localpos.dir)) {
        case dir__BL:
            lpos_left(ci.wd);
            break;
        case dir__BR:
            break;
        case dir__BB:
        case dir__BT:
            lpos_left(ci.wd / 2);
            break;
        }
        break;
    }
    *(pdf->posstruct) = localpos;

    if (has_packet(ffi, c))
        do_vf_packet(pdf, ffi, c);
    else
        pdf_place_glyph(pdf, ffi, c);

    switch (box_direction(dvi_direction)) {
    case dir_TL_:
    case dir_TR_:
    case dir_BL_:
    case dir_BR_:
        return ci.wd;           /* the horizontal movement (in box coordinate system) */
        break;
    case dir_LT_:
    case dir_RT_:
    case dir_LB_:
    case dir_RB_:
        return (ci.ht + ci.dp);
        break;
    }
}

/* mark |f| as a used font; set |font_used(f)|, |pdf_font_size(f)| and |pdf_font_num(f)| */
void pdf_use_font(internal_font_number f, integer fontnum)
{
    set_pdf_font_size(f, font_size(f));
    set_font_used(f, true);
    assert((fontnum > 0) || ((fontnum < 0) && (pdf_font_num(-fontnum) > 0)));
    set_pdf_font_num(f, fontnum);
}

/*
To set PDF font we need to find out fonts with the same name, because \TeX\
can load the same font several times for various sizes. For such fonts we
define only one font resource. The array |pdf_font_num| holds the object
number of font resource. A negative value of an entry of |pdf_font_num|
indicates that the corresponding font shares the font resource with the font
*/

/* create a font object */
void pdf_init_font(PDF pdf, internal_font_number f)
{
    internal_font_number k, b;
    integer i;
    assert(!font_used(f));

    /* if |f| is auto expanded then ensure the base font is initialized */
    if (pdf_font_auto_expand(f) && (pdf_font_blink(f) != null_font)) {
        b = pdf_font_blink(f);
        /* TODO: reinstate this check. disabled because wide fonts font have fmentries */
        if (false && (!hasfmentry(b)))
            pdf_error("font expansion",
                      "auto expansion is only possible with scalable fonts");
        if (!font_used(b))
            pdf_init_font(pdf, b);
        set_font_map(f, font_map(b));
    }
    /* check whether |f| can share the font object with some |k|: we have 2 cases
       here: 1) |f| and |k| have the same tfm name (so they have been loaded at
       different sizes, eg 'cmr10' and 'cmr10 at 11pt'); 2) |f| has been auto
       expanded from |k|
     */
    if (hasfmentry(f) || true) {
        i = pdf->head_tab[obj_type_font];
        while (i != 0) {
            k = obj_info(pdf, i);
            if (font_shareable(f, k)) {
                assert(pdf_font_num(k) != 0);
                if (pdf_font_num(k) < 0)
                    pdf_use_font(f, pdf_font_num(k));
                else
                    pdf_use_font(f, -k);
                return;
            }
            i = obj_link(pdf, i);
        }
    }
    /* create a new font object for |f| */
    pdf_create_obj(pdf, obj_type_font, f);
    pdf_use_font(f, pdf->obj_ptr);
}

/* set the actual font on PDF page */
internal_font_number pdf_set_font(PDF pdf, internal_font_number f)
{
    pdf_object_list *p, *q;
    internal_font_number k;
    integer ff;                 /* for use with |set_ff| */

    if (!font_used(f))
        pdf_init_font(pdf, f);
    set_ff(f);                  /* set |ff| to the tfm number of the font sharing the font object
                                   with |f|; |ff| is either |f| or some font with the same tfm name
                                   at different size and/or expansion */
    k = ff;
    if (pdf->font_list == NULL) {
        /* |font_list| is empty, append |f| */
        pdf->font_list = xmalloc(sizeof(pdf_object_list));
        pdf->font_list->link = NULL;
        pdf->font_list->info = f;
        return k;
    }
    p = pdf->font_list;
    while (p != NULL) {
        set_ff(p->info);
        if (ff == k)
            return k;
        q = p;
        p = p->link;
    }
    /* |f| not found in |font_list|, append it now */
    p = xmalloc(sizeof(pdf_object_list));
    q->link = p;
    p->link = NULL;
    p->info = f;
    return k;
}


/* Here come some subroutines to deal with expanded fonts for HZ-algorithm. */

void copy_expand_params(internal_font_number k, internal_font_number f,
                        integer e)
{                               /* set expansion-related parameters for an expanded font |k|, based on the base
                                   font |f| and the expansion amount |e| */
    set_pdf_font_expand_ratio(k, e);
    set_pdf_font_step(k, pdf_font_step(f));
    set_pdf_font_auto_expand(k, pdf_font_auto_expand(f));
    set_pdf_font_blink(k, f);
}

internal_font_number tfm_lookup(str_number s, scaled fs)
{                               /* looks up for a TFM with name |s| loaded at |fs| size; if found then flushes |s| */
    internal_font_number k;
    if (fs != 0) {
        for (k = 1; k <= max_font_id(); k++) {
            if (cmp_font_name(k, s) && (font_size(k) == fs)) {
                flush_str(s);
                return k;
            }
        }
    } else {
        for (k = 1; k <= max_font_id(); k++) {
            if (cmp_font_name(k, s)) {
                flush_string(); /* |font_name| */
                flush_str(s);
                return k;
            } else {
                flush_string();
            }
        }
    }
    return null_font;
}

internal_font_number load_expand_font(internal_font_number f, integer e)
{                               /* loads font |f| expanded by |e| thousandths into font memory; |e| is nonzero
                                   and is a multiple of |pdf_font_step(f)| */
    str_number s;               /* font name */
    internal_font_number k;
    integer font_id_base = get_font_id_base();
    s = expand_font_name(f, e);
    k = tfm_lookup(s, font_size(f));
    if (k == null_font) {
        if (pdf_font_auto_expand(f)) {
            k = auto_expand_font(f, e);
            font_id_text(k) = font_id_text(f);
        } else {
            k = read_font_info(get_nullcs(), s, font_size(f),
                               font_natural_dir(f));
        }
    }
    copy_expand_params(k, f, e);
    return k;
}

integer fix_expand_value(internal_font_number f, integer e)
{                               /* return the multiple of |pdf_font_step(f)| that is nearest to |e| */
    integer step;
    integer max_expand;
    boolean neg;
    if (e == 0)
        return 0;
    if (e < 0) {
        e = -e;
        neg = true;
        max_expand = -pdf_font_expand_ratio(pdf_font_shrink(f));
    } else {
        neg = false;
        max_expand = pdf_font_expand_ratio(pdf_font_stretch(f));
    }
    if (e > max_expand) {
        e = max_expand;
    } else {
        step = pdf_font_step(f);
        if (e % step > 0)
            e = step * round_xn_over_d(e, 1, step);
    }
    if (neg)
        e = -e;
    return e;
}

internal_font_number get_expand_font(internal_font_number f, integer e)
{                               /* look up and create if not found an expanded version of |f|; |f| is an
                                   expandable font; |e| is nonzero and is a multiple of |pdf_font_step(f)| */
    internal_font_number k;
    k = pdf_font_elink(f);
    while (k != null_font) {
        if (pdf_font_expand_ratio(k) == e)
            return k;
        k = pdf_font_elink(k);
    }
    k = load_expand_font(f, e);
    set_pdf_font_elink(k, pdf_font_elink(f));
    set_pdf_font_elink(f, k);
    return k;
}

internal_font_number expand_font(internal_font_number f, integer e)
{                               /* looks up for font |f| expanded by |e| thousandths, |e| is an arbitrary value
                                   between max stretch and max shrink of |f|; if not found then creates it */
    if (e == 0)
        return f;
    e = fix_expand_value(f, e);
    if (e == 0)
        return f;
    if (pdf_font_elink(f) == null_font)
        pdf_error("font expansion", "uninitialized pdf_font_elink");
    return get_expand_font(f, e);
}

void set_expand_params(internal_font_number f, boolean auto_expand,
                       integer stretch_limit, integer shrink_limit,
                       integer font_step, integer expand_ratio)
{                               /* expand a font with given parameters */
    set_pdf_font_step(f, font_step);
    set_pdf_font_auto_expand(f, auto_expand);
    if (stretch_limit > 0)
        set_pdf_font_stretch(f, get_expand_font(f, stretch_limit));
    if (shrink_limit > 0)
        set_pdf_font_shrink(f, get_expand_font(f, -shrink_limit));
    if (expand_ratio != 0)
        set_pdf_font_expand_ratio(f, expand_ratio);
}

void read_expand_font(void)
{                               /* read font expansion spec and load expanded font */
    integer shrink_limit, stretch_limit, font_step;
    internal_font_number f;
    boolean auto_expand;
    /* read font expansion parameters */
    scan_font_ident();
    f = cur_val;
    if (f == null_font)
        pdf_error("font expansion", "invalid font identifier");
    if (pdf_font_blink(f) != null_font)
        pdf_error("font expansion",
                  "\\pdffontexpand cannot be used this way (the base font has been expanded)");
    scan_optional_equals();
    scan_int();
    stretch_limit = fix_int(cur_val, 0, 1000);
    scan_int();
    shrink_limit = fix_int(cur_val, 0, 500);
    scan_int();
    font_step = fix_int(cur_val, 0, 100);
    if (font_step == 0)
        pdf_error("font expansion", "invalid step");
    stretch_limit = stretch_limit - stretch_limit % font_step;
    if (stretch_limit < 0)
        stretch_limit = 0;
    shrink_limit = shrink_limit - shrink_limit % font_step;
    if (shrink_limit < 0)
        shrink_limit = 0;
    if ((stretch_limit == 0) && (shrink_limit == 0))
        pdf_error("font expansion", "invalid limit(s)");
    auto_expand = false;
    if (scan_keyword("autoexpand")) {
        auto_expand = true;
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    }

    /* check if the font can be expanded */
    if (pdf_font_expand_ratio(f) != 0)
        pdf_error("font expansion",
                  "this font has been expanded by another font so it cannot be used now");
    if (pdf_font_step(f) != 0) {
        /* this font has been expanded, ensure the expansion parameters are identical */
        if (pdf_font_step(f) != font_step)
            pdf_error("font expansion",
                      "font has been expanded with different expansion step");

        if (((pdf_font_stretch(f) == null_font) && (stretch_limit != 0)) ||
            ((pdf_font_stretch(f) != null_font)
             && (pdf_font_expand_ratio(pdf_font_stretch(f)) != stretch_limit)))
            pdf_error("font expansion",
                      "font has been expanded with different stretch limit");

        if (((pdf_font_shrink(f) == null_font) && (shrink_limit != 0)) ||
            ((pdf_font_shrink(f) != null_font)
             && (-pdf_font_expand_ratio(pdf_font_shrink(f)) != shrink_limit)))
            pdf_error("font expansion",
                      "font has been expanded with different shrink limit");

        if (pdf_font_auto_expand(f) != auto_expand)
            pdf_error("font expansion",
                      "font has been expanded with different auto expansion value");
    } else {
        if (font_used(f))
            pdf_warning("font expansion",
                        "font should be expanded before its first use", true,
                        true);
        set_expand_params(f, auto_expand, stretch_limit, shrink_limit,
                          font_step, 0);
        if (font_type(f) == virtual_font_type)
            vf_expand_local_fonts(f);
    }
}

void new_letterspaced_font(small_number a)
{                               /* letter-space a font by creating a virtual font */
    pointer u;                  /* user's font identifier */
    str_number t;               /* name for the frozen font identifier */
    internal_font_number f, k;
    integer font_id_base = get_font_id_base();
    get_r_token();
    u = cur_cs;
    if (u >= hash_base)
        t = text(u);
    else
        t = maketexstring("FONT");
    define(u, set_font_cmd, null_font);
    scan_optional_equals();
    scan_font_ident();
    k = cur_val;
    scan_int();
    f = letter_space_font(u, k, fix_int(cur_val, -1000, 1000));
    equiv(u) = f;
    zeqtb[font_id_base + f] = zeqtb[u];
    font_id_text(f) = t;
}

void make_font_copy(small_number a)
{                               /* make a font copy for further use with font expansion */
    pointer u;                  /* user's font identifier */
    str_number t;               /* name for the frozen font identifier */
    internal_font_number f, k;
    integer font_id_base = get_font_id_base();
    get_r_token();
    u = cur_cs;
    if (u >= hash_base)
        t = text(u);
    else
        t = maketexstring("FONT");
    define(u, set_font_cmd, null_font);
    scan_optional_equals();
    scan_font_ident();
    k = cur_val;
    f = copy_font_info(k);
    equiv(u) = f;
    zeqtb[font_id_base + f] = zeqtb[u];
    font_id_text(f) = t;
}


void pdf_include_chars(PDF pdf)
{
    str_number s;
    pool_pointer k;             /* running indices */
    internal_font_number f;
    scan_font_ident();
    f = cur_val;
    if (f == null_font)
        pdf_error("font", "invalid font identifier");
    pdf_check_vf(cur_val);
    if (!font_used(f))
        pdf_init_font(pdf, f);
    scan_pdf_ext_toks();
    s = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    k = str_start_macro(s);
    while (k < str_start_macro(s + 1)) {
        pdf_mark_char(f, str_pool[k]);
        incr(k);
    }
    flush_str(s);
}

void glyph_to_unicode(void)
{
    str_number s1, s2;
    scan_pdf_ext_toks();
    s1 = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    scan_pdf_ext_toks();
    s2 = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    def_tounicode(s1, s2);
    flush_str(s2);
    flush_str(s1);
}
