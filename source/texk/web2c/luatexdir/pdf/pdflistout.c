/* pdflistout.c

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

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"

pos_info_structure pos_info;    /* to be accessed from Lua */

backend_node_function *backend_out = NULL;
backend_whatsit_function *backend_out_whatsit = NULL;

static void missing_backend_function(PDF pdf, halfword p)
{
    char *b = NULL, *n, *s;
    char backend_string[15];
    char err_string[60];
    if (type(p) == whatsit_node)
        s = "whatsit";
    else
        s = "node";
    n = get_node_name(type(p), subtype(p));
    switch (pdf->o_mode) {
    case OMODE_DVI:
        b = "DVI";
        break;
    case OMODE_PDF:
        b = "PDF";
        break;
    case OMODE_LUA:
        b = "Lua";
        break;
    default:
        assert(0);
    }
    snprintf(backend_string, 14, "%s back-end", b);
    snprintf(err_string, 59, "no output function for \"%s\" %s", n, s);
    pdf_error(backend_string, err_string);
}

static backend_node_function *new_backend_out()
{
    assert(backend_out == NULL);
    return xmalloc((MAX_NODE_TYPE + 1) * sizeof(backend_node_function));
}

static backend_node_function *new_backend_out_whatsit()
{
    assert(backend_out_whatsit == NULL);
    return xmalloc((MAX_WHATSIT_TYPE + 1) * sizeof(backend_whatsit_function));
}

static void init_pdf_backend_functionpointers()
{
    backend_out[rule_node] = &pdf_place_rule;   /* 2 */
    backend_out[glyph_node] = &pdf_place_glyph; /* 37 */
    /* ...these are all (?) */
    backend_out_whatsit[special_node] = &pdf_special;   /* 3 */
    backend_out_whatsit[pdf_literal_node] = &pdf_out_literal;   /* 8 */
    backend_out_whatsit[pdf_refobj_node] = &pdf_ref_obj;        /* 10 */
    backend_out_whatsit[pdf_refxform_node] = &pdf_place_form;   /* 12 */
    backend_out_whatsit[pdf_refximage_node] = &pdf_place_image; /* 14 */
    backend_out_whatsit[pdf_annot_node] = &do_annot;    /* 15 */
    backend_out_whatsit[pdf_start_link_node] = &do_link;        /* 16 */
    backend_out_whatsit[pdf_end_link_node] = &end_link; /* 17 */
    backend_out_whatsit[pdf_dest_node] = &do_dest;      /* 19 */
    backend_out_whatsit[pdf_thread_node] = &do_thread;  /* 20 */
    backend_out_whatsit[pdf_end_thread_node] = &end_thread;     /* 22 */
    backend_out_whatsit[late_lua_node] = &late_lua;     /* 35 */
    backend_out_whatsit[pdf_colorstack_node] = &pdf_out_colorstack;     /* 39 */
    backend_out_whatsit[pdf_setmatrix_node] = &pdf_out_setmatrix;       /* 40 */
    backend_out_whatsit[pdf_save_node] = &pdf_out_save; /* 41 */
    backend_out_whatsit[pdf_restore_node] = &pdf_out_restore;   /* 42 */
}

static void init_dvi_backend_functionpointers()
{
    backend_out[rule_node] = &dvi_place_rule;   /* 2 */
    backend_out[glyph_node] = &dvi_place_glyph; /* 37 */
    /* ...these are all (?) */
    backend_out_whatsit[special_node] = &dvi_special;   /* 3 */
    backend_out_whatsit[late_lua_node] = &late_lua;     /* 35 */
}

static void init_lua_backend_functionpointers()
{
    backend_out[rule_node] = &lua_place_rule;   /* 2 */
    backend_out[glyph_node] = &lua_place_glyph; /* 37 */
    /* ...these are all (?) */
    backend_out_whatsit[late_lua_node] = &late_lua;     /* 35 */
}

void init_backend_functionpointers(PDF pdf)
{
    int i;
    if (backend_out == NULL || backend_out_whatsit == NULL) {
        backend_out = new_backend_out();
        backend_out_whatsit = new_backend_out_whatsit();
    }
    for (i = 0; i < MAX_NODE_TYPE + 1; i++)
        backend_out[i] = &missing_backend_function;
    for (i = 0; i < MAX_WHATSIT_TYPE + 1; i++)
        backend_out_whatsit[i] = &missing_backend_function;
    switch (pdf->o_mode) {
    case OMODE_NONE:           /* all node types give errors */
        break;
    case OMODE_DVI:
        init_dvi_backend_functionpointers();
        break;
    case OMODE_PDF:
        init_pdf_backend_functionpointers();
        break;
    case OMODE_LUA:
        init_lua_backend_functionpointers();
        break;
    default:
        assert(0);
    }
}

/***********************************************************************/

/*
This code scans forward to the ending |dir_node| while keeping
track of the needed width in |w|. When it finds the node that will end
this segment, it stores the accumulated with in the |dir_dvi_h| field
of that end node, so that when that node is found later in the
processing, the correct glue correction can be applied.
*/

static scaled simple_advance_width(halfword p)
{
    halfword q = p;
    scaled w = 0;
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q)) {
            w += glyph_width(q);
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w += width(q);
                break;
            case disc_node:
                if (vlink(no_break(q)) != null)
                    w += simple_advance_width(no_break(q));
            default:
                break;
            }
        }
    }
    return w;
}

static halfword calculate_width_to_enddir(halfword p, real cur_glue,
                                          scaled cur_g, halfword this_box)
{
    int dir_nest = 1;
    halfword q = p, enddir_ptr = p;
    scaled w = 0;
    halfword g;                 /* this is normally a global variable, but that is just too hideous */
    /* to me, it looks like the next is needed. but Aleph doesn't do that, so let us not do it either */
    real glue_temp;             /* glue value before rounding */
    /* |w:=w-cur_g; cur_glue:=0;| */
    integer g_sign = glue_sign(this_box);
    integer g_order = glue_order(this_box);
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q))
            w += pack_width(box_dir(this_box), glyph_dir, q, true);
        else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
                w += pack_width(box_dir(this_box), box_dir(q), q, false);
                break;
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w += width(q);
                break;
            case glue_node:
                g = glue_ptr(q);
                w += width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                w += cur_g;
                break;
            case disc_node:
                if (vlink(no_break(q)) != null)
                    w += simple_advance_width(no_break(q));
                break;
            case math_node:
                w += surround(q);
                break;
            case whatsit_node:
                if (subtype(q) == dir_node) {
                    if (dir_dir(q) >= 0)
                        incr(dir_nest);
                    else
                        decr(dir_nest);
                    if (dir_nest == 0) {
                        enddir_ptr = q;
                        dir_cur_h(enddir_ptr) = w;
                        q = null;
                    }
                } else if ((subtype(q) == pdf_refxform_node)
                           || (subtype(q) == pdf_refximage_node))
                    w += width(q);
                break;
            default:
                break;
            }
        }
    }
    if (enddir_ptr == p)        /* no enddir found, just transport w by begindir */
        dir_cur_h(enddir_ptr) = w;
    return enddir_ptr;
}

/**********************************************************************/

/*
The |out_what| procedure takes care of outputting the whatsit nodes for
|vlist_out| and |hlist_out|, which are similar for either case.
*/

void out_what(PDF pdf, halfword p)
{
    int j;                      /* write stream number */
    switch (subtype(p)) {
        /* function(pdf, p) */
    case pdf_save_node:        /* pdf_out_save(pdf, p); */
    case pdf_restore_node:     /* pdf_out_restore(pdf, p); */
    case pdf_end_link_node:    /* end_link(pdf, p); */
    case pdf_end_thread_node:  /* end_thread(pdf, p); */
    case pdf_literal_node:     /* pdf_out_literal(pdf, p); */
    case pdf_colorstack_node:  /* pdf_out_colorstack(pdf, p); */
    case pdf_setmatrix_node:   /* pdf_out_setmatrix(pdf, p); */
    case special_node:         /* pdf_special(pdf, p); */
    case late_lua_node:        /* late_lua(pdf, p); */
    case pdf_refobj_node:      /* pdf_ref_obj(pdf, p) */
        backend_out_whatsit[subtype(p)] (pdf, p);
        break;
    case open_node:
    case write_node:
    case close_node:
        /* Do some work that has been queued up for \.{\\write} */
        /* We don't implement \.{\\write} inside of leaders. (The reason is that
           the number of times a leader box appears might be different in different
           implementations, due to machine-dependent rounding in the glue calculations.)
           @^leaders@> */
        if (!doing_leaders) {
            j = write_stream(p);
            if (subtype(p) == write_node) {
                write_out(p);
            } else {
                if (write_open[j])
                    lua_a_close_out(write_file[j]);
                if (subtype(p) == close_node) {
                    write_open[j] = false;
                } else if (j < 16) {
                    cur_name = open_name(p);
                    cur_area = open_area(p);
                    cur_ext = open_ext(p);
                    if (cur_ext == get_nullstr())
                        cur_ext = maketexstring(".tex");
                    pack_file_name(cur_name, cur_area, cur_ext);
                    while (!lua_a_open_out(write_file[j], (j + 1)))
                        prompt_file_name("output file name", ".tex");
                    write_file[j] = name_file_pointer;
                    write_open[j] = true;
                }
            }
        }
        break;
    case dir_node:             /* in a vlist */
        break;
    case local_par_node:
    case cancel_boundary_node:
    case user_defined_node:
        break;
    default:
        /* this should give an error about missing whatsit backend function */
        backend_out_whatsit[subtype(p)] (pdf, p);
    }
}

/**********************************************************************/

void hlist_out(PDF pdf, halfword this_box)
{

    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    scaledpos cur = { 0, 0 }, tmpcur, basepoint;
    scaledpos size = { 0, 0 };  /* rule dimensions */
    scaled effective_horizontal;
    scaled save_h;              /* what |cur.h| should pop to */
    scaled edge;                /* right edge of sub-box or leader space */
    halfword enddir_ptr;        /* temporary pointer to enddir node */
    /* label move_past, fin_rule, next_p; */
    /* scaled w;                     temporary value for directional width calculation  */
    integer g_order;            /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p, q;              /* current position in the hlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_wd;           /* width of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    halfword prev_p;            /* one step behind |p| */
    real glue_temp;             /* glue value before rounding */
    real cur_glue = 0.0;        /* glue seen so far */
    scaled cur_g = 0;           /* rounded equivalent of |cur_glue| times the glue ratio */
    scaled_whd rule, ci;
    int i;                      /* index to scan |pdf_link_stack| */
    integer save_loc = 0;       /* DVI! \.{DVI} byte location upon entry */
    scaledpos save_dvi = { 0, 0 };      /* DVI! what |dvi| should pop to */

    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.pos = refpos->pos;
    localpos.dir = box_dir(this_box);

    incr(cur_s);
    if (cur_s > max_push)
        max_push = cur_s;

    if (pdf->o_mode == OMODE_DVI) {
        if (cur_s > 0) {        /* DVI! */
            dvi_push();         /* DVI! */
            save_dvi = dvi;     /* DVI! */
        }
        save_loc = dvi_offset + dvi_ptr /* DVI! */ ;
    }

    prev_p = this_box + list_offset;
    /* Create link annotations for the current hbox if needed */
    for (i = 1; i <= pdf->link_stack_ptr; i++) {
        assert(is_running(width(pdf->link_stack[i].link_node)));
        if (pdf->link_stack[i].nesting_level == cur_s)
            append_link(pdf, this_box, cur, i);
    }

    /* Start hlist {\sl Sync\TeX} information record */
    if (synctexoption == 1)
        synctex_hlist(this_box);

    while (p != null) {
        if (is_char_node(p)) {
            do {
                if (x_displace(p) != 0 || y_displace(p) != 0) {
                    tmpcur.h = cur.h + x_displace(p);
                    tmpcur.v = cur.v - y_displace(p);
                    synch_pos_with_cur(pdf->posstruct, refpos, tmpcur);
                }
                output_one_char(pdf, font(p), character(p));
                ci = get_charinfo_whd(font(p), character(p));
                if (dir_parallel
                    (dir_secondary[localpos.dir], dir_secondary[dir_TLT]))
                    cur.h += ci.wd;
                else
                    cur.h += ci.ht + ci.dp;
                synch_pos_with_cur(pdf->posstruct, refpos, cur);
                p = vlink(p);
            } while (is_char_node(p));
            /* Record current point {\sl Sync\TeX} information */
            if (synctexoption == 1)
                synctex_current();
        } else {
            /* Output the non-|char_node| |p| for |hlist_out|
               and move to the next node */
            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in an hlist */

                if (dir_parallel
                    (dir_secondary[box_dir(p)], dir_secondary[localpos.dir])) {
                    effective_horizontal = width(p);
                    basepoint.v = 0;
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[localpos.dir]))
                        basepoint.h = width(p);
                    else
                        basepoint.h = 0;
                } else {
                    effective_horizontal = height(p) + depth(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[localpos.dir]))
                            basepoint.h = height(p);
                        else
                            basepoint.h = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[localpos.dir]))
                            basepoint.h = depth(p);
                        else
                            basepoint.h = height(p);
                    }
                    if (is_rotated(localpos.dir)) {
                        if (dir_eq
                            (dir_secondary[box_dir(p)],
                             dir_primary[localpos.dir]))
                            basepoint.v = -width(p) / 2;        /* `up' */
                        else
                            basepoint.v = width(p) / 2; /* `down' */
                    } else if (is_mirrored(localpos.dir)) {
                        if (dir_eq
                            (dir_secondary[box_dir(p)],
                             dir_primary[localpos.dir]))
                            basepoint.v = 0;
                        else
                            basepoint.v = width(p);     /* `down' */
                    } else {
                        if (dir_eq
                            (dir_secondary[box_dir(p)],
                             dir_primary[localpos.dir]))
                            basepoint.v = -width(p);    /* `up' */
                        else
                            basepoint.v = 0;
                    }
                }
                if (!is_mirrored(localpos.dir))
                    basepoint.v = basepoint.v + shift_amount(p);        /* shift the box `down' */
                else
                    basepoint.v = basepoint.v - shift_amount(p);        /* shift the box `up' */
                if (list_ptr(p) == null) {
                    /* Record void list {\sl Sync\TeX} information */
                    if (synctexoption == 1) {
                        if (type(p) == vlist_node)
                            synctex_void_vlist(p, this_box);
                        else
                            synctex_void_hlist(p, this_box);
                    }
                } else {
                    assert(cur.v == 0);
                    tmpcur.h = cur.h + basepoint.h;
                    tmpcur.v = basepoint.v;
                    synch_pos_with_cur(pdf->posstruct, refpos, tmpcur);
                    if (type(p) == vlist_node)
                        vlist_out(pdf, p);
                    else
                        hlist_out(pdf, p);
                }
                cur.h += effective_horizontal;
                break;
            case disc_node:
                if (vlink(no_break(p)) != null) {
                    if (subtype(p) != select_disc) {
                        q = tail_of_list(vlink(no_break(p)));   /* TODO, this should be a tlink */
                        vlink(q) = vlink(p);
                        q = vlink(no_break(p));
                        vlink(no_break(p)) = null;
                        vlink(p) = q;
                    }
                }
                break;
            case rule_node:
                if (dir_parallel
                    (dir_primary[rule_dir(p)], dir_primary[localpos.dir])) {
                    rule.ht = height(p);
                    rule.dp = depth(p);
                    rule.wd = width(p);
                } else {
                    rule.ht = width(p) / 2;
                    rule.dp = width(p) / 2;
                    rule.wd = height(p) + depth(p);
                }
                goto FIN_RULE;
                break;
            case whatsit_node:
                /* Output the whatsit node |p| in |hlist_out| */
                switch (subtype(p)) {
                case pdf_save_pos_node:
                    pdf_last_pos = pdf->posstruct->pos;
                    pos_info.curpos = pdf->posstruct->pos;
                    pos_info.boxpos.pos = refpos->pos;
                    pos_info.boxpos.dir = localpos.dir;
                    pos_info.boxdim.wd = width(this_box);
                    pos_info.boxdim.ht = height(this_box);
                    pos_info.boxdim.dp = depth(this_box);
                    break;
                    /* function(pdf, p, this_box, cur); too many args for out_what() */
                case pdf_annot_node:   /* do_annot(pdf, p, this_box, cur); */
                case pdf_start_link_node:      /* do_link(pdf, p, this_box, cur); */
                case pdf_dest_node:    /* do_dest(pdf, p, this_box, cur); */
                case pdf_start_thread_node:
                case pdf_thread_node:  /* do_thread(pdf, p, this_box, cur); */
                    backend_out_whatsit[subtype(p)] (pdf, p, this_box, cur);
                    break;
                case pdf_refxform_node:        /* pdf_place_form(pdf, pdf_xform_objnum(p)); */
                case pdf_refximage_node:       /* pdf_place_image(pdf, pdf_ximage_idx(p)); */
                    /* Output a Form node in a hlist */
                    /* Output an Image node in a hlist */
                    switch (box_direction(localpos.dir)) {
                    case dir_TL_:
                        pos_down(depth(p));
                        break;
                    case dir_TR_:
                        pos_left(width(p));
                        pos_down(depth(p));
                        break;
                    case dir_BL_:
                        pos_down(height(p));
                        break;
                    case dir_BR_:
                        pos_left(width(p));
                        pos_down(height(p));
                        break;
                    case dir_LT_:
                        pos_left(height(p));
                        pos_down(width(p));
                        break;
                    case dir_RT_:
                        pos_left(depth(p));
                        pos_down(width(p));
                        break;
                    case dir_LB_:
                        pos_left(height(p));
                        break;
                    case dir_RB_:
                        pos_left(depth(p));
                        break;
                    }
                    backend_out_whatsit[subtype(p)] (pdf, p);
                    cur.h += width(p);
                    break;
                case dir_node:
                    /* Output a reflection instruction if the direction has changed */
                    if (dir_dir(p) >= 0) {
                        /* Calculate the needed width to the matching |enddir|, return the |enddir| node,
                           with width info */
                        enddir_ptr =
                            calculate_width_to_enddir(p, cur_glue, cur_g,
                                                      this_box);
                        if (dir_parallel
                            (dir_secondary[dir_dir(p)],
                             dir_secondary[localpos.dir])) {
                            dir_cur_h(enddir_ptr) += cur.h;
                            if (dir_opposite
                                (dir_secondary[dir_dir(p)],
                                 dir_secondary[localpos.dir]))
                                cur.h = dir_cur_h(enddir_ptr);
                        } else
                            dir_cur_h(enddir_ptr) = cur.h;
                        if (enddir_ptr != p) {  /* only if it is an enddir */
                            dir_cur_v(enddir_ptr) = cur.v;
                            dir_refpos_h(enddir_ptr) = refpos->pos.h;
                            dir_refpos_v(enddir_ptr) = refpos->pos.v;
                            dir_dir(enddir_ptr) = localpos.dir - 64;    /* negative: mark it as |enddir| */
                        }
                        /* fake a nested |hlist_out| */
                        synch_pos_with_cur(pdf->posstruct, refpos, cur);
                        refpos->pos = pdf->posstruct->pos;
                        localpos.dir = dir_dir(p);
                        cur.h = 0;
                        cur.v = 0;
                    } else {
                        refpos->pos.h = dir_refpos_h(p);
                        refpos->pos.v = dir_refpos_v(p);
                        localpos.dir = dir_dir(p) + 64;
                        cur.h = dir_cur_h(p);
                        cur.v = dir_cur_v(p);
                    }
                    break;
                default:
                    out_what(pdf, p);
                }
                break;
            case glue_node:
                /* (\pdfTeX) Move right or output leaders */

                g = glue_ptr(p);
                rule.wd = width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);        /* real multiplication */
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                rule.wd = rule.wd + cur_g;
                if (subtype(p) >= a_leaders) {
                    /* (\pdfTeX) Output leaders in an hlist, |goto fin_rule| if a rule
                       or to |next_p| if done */
                    leader_box = leader_ptr(p);
                    if (type(leader_box) == rule_node) {
                        rule.ht = height(leader_box);
                        rule.dp = depth(leader_box);
                        goto FIN_RULE;
                    }
                    if (dir_parallel
                        (dir_secondary[box_dir(leader_box)],
                         dir_secondary[localpos.dir]))
                        leader_wd = width(leader_box);
                    else
                        leader_wd = height(leader_box) + depth(leader_box);
                    if ((leader_wd > 0) && (rule.wd > 0)) {
                        rule.wd = rule.wd + 10; /* compensate for floating-point rounding */
                        edge = cur.h + rule.wd;
                        lx = 0;
                        /* Let |cur.h| be the position of the first box, and set |leader_wd+lx|
                           to the spacing between corresponding parts of boxes */

                        if (subtype(p) == g_leaders) {
                            save_h = cur.h;
                            switch (dir_secondary[localpos.dir]) {
                            case dir_L:
                                cur.h += refpos->pos.h - shipbox_refpos.h;
                                cur.h = leader_wd * (cur.h / leader_wd);
                                cur.h -= refpos->pos.h - shipbox_refpos.h;
                                break;
                            case dir_R:
                                cur.h =
                                    refpos->pos.h - shipbox_refpos.h - cur.h;
                                cur.h = leader_wd * (cur.h / leader_wd);
                                cur.h =
                                    refpos->pos.h - shipbox_refpos.h - cur.h;
                                break;
                            case dir_T:
                                cur.h =
                                    refpos->pos.v - shipbox_refpos.v - cur.h;
                                cur.h = leader_wd * (cur.h / leader_wd);
                                cur.h =
                                    refpos->pos.v - shipbox_refpos.v - cur.h;
                                break;
                            case dir_B:
                                cur.h += refpos->pos.v - shipbox_refpos.v;
                                cur.h = leader_wd * (cur.h / leader_wd);
                                cur.h -= refpos->pos.v - shipbox_refpos.v;
                                break;
                            }
                            if (cur.h < save_h)
                                cur.h += leader_wd;
                        } else if (subtype(p) == a_leaders) {
                            save_h = cur.h;
                            cur.h = leader_wd * (cur.h / leader_wd);
                            if (cur.h < save_h)
                                cur.h += leader_wd;
                        } else {
                            lq = rule.wd / leader_wd;   /* the number of box copies */
                            lr = rule.wd % leader_wd;   /* the remaining space */
                            if (subtype(p) == c_leaders) {
                                cur.h += lr / 2;
                            } else {
                                lx = lr / (lq + 1);
                                cur.h += (lr - (lq - 1) * lx) / 2;
                            }
                        }

                        while (cur.h + leader_wd <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.h|,
                               then advance |cur.h| by |leader_wd+lx| */

                            if (dir_parallel
                                (dir_primary[box_dir(leader_box)],
                                 dir_primary[localpos.dir])) {
                                basepoint.v = 0;
                                if (dir_opposite
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_secondary[localpos.dir]))
                                    basepoint.h = width(leader_box);
                                else
                                    basepoint.h = 0;
                            } else {
                                if (!is_mirrored(box_dir(leader_box))) {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[localpos.dir]))
                                        basepoint.h = height(leader_box);
                                    else
                                        basepoint.h = depth(leader_box);
                                } else {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[localpos.dir]))
                                        basepoint.h = depth(leader_box);
                                    else
                                        basepoint.h = height(leader_box);
                                }
                                if (dir_eq
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_primary[localpos.dir]))
                                    basepoint.v = -(width(leader_box) / 2);
                                else
                                    basepoint.v = (width(leader_box) / 2);
                            }
                            if (!is_mirrored(localpos.dir))
                                basepoint.v = basepoint.v + shift_amount(leader_box);   /* shift the box `down' */
                            else
                                basepoint.v = basepoint.v - shift_amount(leader_box);   /* shift the box `up' */
                            assert(cur.v == 0);
                            tmpcur.h = cur.h + basepoint.h;
                            tmpcur.v = basepoint.v;
                            synch_pos_with_cur(pdf->posstruct, refpos, tmpcur);
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                vlist_out(pdf, leader_box);
                            else
                                hlist_out(pdf, leader_box);
                            doing_leaders = outer_doing_leaders;
                            cur.h += leader_wd + lx;
                        }
                        cur.h = edge - 10;
                        goto NEXTP;
                    }
                }
                goto MOVE_PAST;
                break;
            case margin_kern_node:
                cur.h += width(p);
                break;
            case kern_node:
                /* Record |kern_node| {\sl Sync\TeX} information */
                if (synctexoption == 1)
                    synctex_kern(p, this_box);
                cur.h += width(p);
                break;
            case math_node:
                /* Record |math_node| {\sl Sync\TeX} information */
                if (synctexoption == 1)
                    synctex_math(p, this_box);
                cur.h += surround(p);
                break;
            default:
                break;
            }
            goto NEXTP;
          FIN_RULE:
            /* (\pdfTeX) Output a rule in an hlist */
            if (is_running(rule.ht))
                rule.ht = height(this_box);
            if (is_running(rule.dp))
                rule.dp = depth(this_box);
            if ((rule.ht + rule.dp) > 0 && rule.wd > 0) {       /* we don't output empty rules */
                switch (box_direction(localpos.dir)) {
                case dir_TL_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_down(rule.dp);
                    break;
                case dir_TR_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_left(size.h);
                    pos_down(rule.dp);
                    break;
                case dir_BL_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_down(rule.ht);
                    break;
                case dir_BR_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_left(size.h);
                    pos_down(rule.ht);
                    break;
                case dir_LT_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(rule.ht);
                    pos_down(size.v);
                    break;
                case dir_RT_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(rule.dp);
                    pos_down(size.v);
                    break;
                case dir_LB_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(rule.ht);
                    break;
                case dir_RB_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(rule.dp);
                    break;
                default:;
                }
                backend_out[rule_node] (pdf, p, size);  /* pdf_place_rule(pdf, p, rule.ht + rule.dp, rule.wd); */
            }
          MOVE_PAST:
            cur.h += rule.wd;
            /* Record horizontal |rule_node| or |glue_node| {\sl Sync\TeX} information */
            if (synctexoption == 1) {
                synch_pos_with_cur(pdf->posstruct, refpos, cur);
                synctex_horizontal_rule_or_glue(p, this_box);
            }
          NEXTP:
            prev_p = p;
            p = vlink(p);
            synch_pos_with_cur(pdf->posstruct, refpos, cur);
        }
    }

    /* Finish hlist {\sl Sync\TeX} information record */
    if (synctexoption == 1)
        synctex_tsilh(this_box);

    if (pdf->o_mode == OMODE_DVI) {
        prune_movements(save_loc);      /* DVI! */
        if (cur_s > 0) {        /* DVI! */
            dvi_pop(save_loc);  /* DVI! */
            dvi = save_dvi;     /* DVI! */
        }
    }
    decr(cur_s);
    pdf->posstruct = refpos;
}

void vlist_out(PDF pdf, halfword this_box)
{
    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    scaledpos cur, tmpcur, basepoint;
    scaledpos size = { 0, 0 };  /* rule dimensions */
    scaled effective_vertical;
    scaled save_v;              /* what |cur.v| should pop to */
    scaled top_edge;            /* the top coordinate for this box */
    scaled edge;                /* bottom boundary of leader space */
    glue_ord g_order;           /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p;                 /* current position in the vlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_ht;           /* height of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    real glue_temp;             /* glue value before rounding */
    real cur_glue = 0.0;        /* glue seen so far */
    scaled cur_g = 0;           /* rounded equivalent of |cur_glue| times the glue ratio */
    scaled_whd rule;
    integer save_loc = 0;       /* DVI! \.{DVI} byte location upon entry */
    scaledpos save_dvi = { 0, 0 };      /* DVI! what |dvi| should pop to */

    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);

    cur.h = 0;
    cur.v = -height(this_box);
    top_edge = cur.v;

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.dir = box_dir(this_box);
    synch_pos_with_cur(pdf->posstruct, refpos, cur);

    incr(cur_s);
    if (cur_s > max_push)
        max_push = cur_s;

    if (pdf->o_mode == OMODE_DVI) {
        if (cur_s > 0) {        /* DVI! */
            dvi_push();         /* DVI! */
            save_dvi = dvi;     /* DVI! */
        }
        save_loc = dvi_offset + dvi_ptr;        /* DVI! */
    }

    /* Start vlist {\sl Sync\TeX} information record */
    if (synctexoption == 1)
        synctex_vlist(this_box);

    /* Create thread for the current vbox if needed */
    check_running_thread(pdf, this_box, cur);

    while (p != null) {
        if (is_char_node(p)) {
            confusion("pdfvlistout");   /* this can't happen */
        } else {
            /* Output the non-|char_node| |p| for |vlist_out| */

            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in a vlist */

                /* TODO: the direct test to switch between |width(p)| and |-width(p)|
                   is definately wrong, because it does not nest properly. But at least
                   it fixes a very obvious problem that otherwise occured with
                   \.{\\pardir TLT} in a document with \.{\\bodydir TRT}, and so it
                   will have to do for now.
                 */
                if (dir_parallel
                    (dir_primary[box_dir(p)], dir_primary[localpos.dir])) {
                    effective_vertical = height(p) + depth(p);
                    if ((type(p) == hlist_node) && is_mirrored(box_dir(p)))
                        basepoint.v = depth(p);
                    else
                        basepoint.v = height(p);
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[localpos.dir]))
                        basepoint.h = width(p);
                    else
                        basepoint.h = 0;
                } else {
                    effective_vertical = width(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[localpos.dir]))
                            basepoint.h = height(p);
                        else
                            basepoint.h = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[localpos.dir]))
                            basepoint.h = depth(p);
                        else
                            basepoint.h = height(p);
                    }
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[localpos.dir]))
                        basepoint.v = 0;
                    else
                        basepoint.v = width(p);
                }
                basepoint.h = basepoint.h + shift_amount(p);    /* shift the box `right' */
                if (list_ptr(p) == null) {
                    cur.v += effective_vertical;
                    /* Record void list {\sl Sync\TeX} information */
                    if (synctexoption == 1) {
                        synch_pos_with_cur(pdf->posstruct, refpos, cur);
                        if (type(p) == vlist_node)
                            synctex_void_vlist(p, this_box);
                        else
                            synctex_void_hlist(p, this_box);
                    }
                } else {
                    assert(cur.h == 0);
                    tmpcur.h = basepoint.h;
                    tmpcur.v = cur.v + basepoint.v;
                    synch_pos_with_cur(pdf->posstruct, refpos, tmpcur);
                    if (type(p) == vlist_node)
                        vlist_out(pdf, p);
                    else
                        hlist_out(pdf, p);
                    cur.v += effective_vertical;
                }
                break;
            case rule_node:
                if (dir_parallel
                    (dir_primary[rule_dir(p)], dir_primary[localpos.dir])) {
                    rule.ht = height(p);
                    rule.dp = depth(p);
                    rule.wd = width(p);
                } else {
                    rule.ht = width(p) / 2;
                    rule.dp = width(p) / 2;
                    rule.wd = height(p) + depth(p);
                }
                goto FIN_RULE;
                break;
            case whatsit_node:
                /* Output the whatsit node |p| in |vlist_out| */
                switch (subtype(p)) {
                case pdf_save_pos_node:
                    pdf_last_pos = pdf->posstruct->pos;
                    pos_info.curpos = pdf->posstruct->pos;
                    pos_info.boxpos.pos = refpos->pos;
                    pos_info.boxpos.dir = localpos.dir;
                    pos_info.boxdim.wd = width(this_box);
                    pos_info.boxdim.ht = height(this_box);
                    pos_info.boxdim.dp = depth(this_box);
                    break;
                    /* function(pdf, p, this_box, cur); too many args for out_what() */
                case pdf_annot_node:   /* do_annot(pdf, p, this_box, cur); */
                case pdf_start_link_node:      /* do_link(pdf, p, this_box, cur); */
                case pdf_dest_node:    /* do_dest(pdf, p, this_box, cur); */
                case pdf_start_thread_node:
                case pdf_thread_node:  /* do_thread(pdf, p, this_box, cur); */
                    backend_out_whatsit[subtype(p)] (pdf, p, this_box, cur);
                    break;
                case pdf_refxform_node:        /* pdf_place_form(pdf, pdf_xform_objnum(p)); */
                case pdf_refximage_node:       /* pdf_place_image(pdf, pdf_ximage_idx(p)); */
                    /* Output a Form node in a vlist */
                    /* Output an Image node in a vlist */
                    switch (box_direction(localpos.dir)) {
                    case dir_TL_:
                        pos_down(height(p) + depth(p));
                        break;
                    case dir_TR_:
                        pos_left(width(p));
                        pos_down(height(p) + depth(p));
                        break;
                    case dir_BL_:
                        break;
                    case dir_BR_:
                        pos_left(width(p));
                        break;
                    case dir_LT_:
                        pos_down(width(p));
                        break;
                    case dir_RT_:
                        pos_left(height(p) + depth(p));
                        pos_down(width(p));
                        break;
                    case dir_LB_:
                        break;
                    case dir_RB_:
                        pos_left(height(p) + depth(p));
                        break;
                    default:
                        break;
                    }
                    backend_out_whatsit[subtype(p)] (pdf, p);
                    cur.v += height(p) + depth(p);
                    break;
                default:
                    out_what(pdf, p);
                }
                break;
            case glue_node:
                /* (\pdfTeX) Move down or output leaders */
                g = glue_ptr(p);
                rule.ht = width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);        /* real multiplication */
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                rule.ht = rule.ht + cur_g;
                if (subtype(p) >= a_leaders) {
                    /* (\pdfTeX) Output leaders in a vlist, |goto fin_rule| if a rule or to |next_p| if done */
                    leader_box = leader_ptr(p);
                    if (type(leader_box) == rule_node) {
                        rule.wd = width(leader_box);
                        rule.dp = 0;
                        goto FIN_RULE;
                    }
                    leader_ht = height(leader_box) + depth(leader_box);
                    if ((leader_ht > 0) && (rule.ht > 0)) {
                        rule.ht = rule.ht + 10; /* compensate for floating-point rounding */
                        edge = cur.v + rule.ht;
                        lx = 0;
                        /* Let |cur.v| be the position of the first box, and set |leader_ht+lx|
                           to the spacing between corresponding parts of boxes */

                        if (subtype(p) == g_leaders) {
                            save_v = cur.v;
                            switch (dir_primary[localpos.dir]) {
                            case dir_L:
                                cur.v += refpos->pos.h - shipbox_refpos.h;
                                cur.v = leader_ht * (cur.v / leader_ht);
                                cur.v -= refpos->pos.h - shipbox_refpos.h;
                                break;
                            case dir_R:
                                cur.v =
                                    refpos->pos.h - shipbox_refpos.h - cur.v;
                                cur.v = leader_ht * (cur.v / leader_ht);
                                cur.v =
                                    refpos->pos.h - shipbox_refpos.h - cur.v;
                                break;
                            case dir_T:
                                cur.v =
                                    refpos->pos.v - shipbox_refpos.v - cur.v;
                                cur.v = leader_ht * (cur.v / leader_ht);
                                cur.v =
                                    refpos->pos.v - shipbox_refpos.v - cur.v;
                                break;
                            case dir_B:
                                cur.v += refpos->pos.v - shipbox_refpos.v;
                                cur.v = leader_ht * (cur.v / leader_ht);
                                cur.v -= refpos->pos.v - shipbox_refpos.v;
                                break;
                            }
                            if (cur.v < save_v)
                                cur.v += leader_ht;
                        } else if (subtype(p) == a_leaders) {
                            save_v = cur.v;
                            cur.v =
                                top_edge +
                                leader_ht * ((cur.v - top_edge) / leader_ht);
                            if (cur.v < save_v)
                                cur.v += leader_ht;
                        } else {
                            lq = rule.ht / leader_ht;   /* the number of box copies */
                            lr = rule.ht % leader_ht;   /* the remaining space */
                            if (subtype(p) == c_leaders) {
                                cur.v += lr / 2;
                            } else {
                                lx = lr / (lq + 1);
                                cur.v += (lr - (lq - 1) * lx) / 2;
                            }
                        }

                        while (cur.v + leader_ht <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.v|,
                               then advance |cur.v| by |leader_ht+lx| */
                            assert(cur.h == 0);
                            tmpcur.h = shift_amount(leader_box);
                            tmpcur.v = cur.v + height(leader_box);
                            synch_pos_with_cur(pdf->posstruct, refpos, tmpcur);
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                vlist_out(pdf, leader_box);
                            else
                                hlist_out(pdf, leader_box);
                            doing_leaders = outer_doing_leaders;
                            cur.v += leader_ht + lx;
                        }
                        cur.v = edge - 10;
                        goto NEXTP;
                    }
                }
                goto MOVE_PAST;
                break;
            case kern_node:
                cur.v += width(p);
                break;
            default:
                break;
            }
            goto NEXTP;
          FIN_RULE:
            /* (\pdfTeX) Output a rule in a vlist, |goto next_p| */
            if (is_running(rule.wd))
                rule.wd = width(this_box);
            if ((rule.ht + rule.dp) > 0 && rule.wd > 0) {       /* we don't output empty rules */
                switch (box_direction(localpos.dir)) {
                case dir_TL_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_down(size.v);
                    break;
                case dir_BL_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    break;
                case dir_TR_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_left(size.h);
                    pos_down(size.v);
                    break;
                case dir_BR_:
                    size.h = rule.wd;
                    size.v = rule.ht + rule.dp;
                    pos_left(size.h);
                    break;
                case dir_LT_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_down(size.v);
                    break;
                case dir_RT_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(size.h);
                    pos_down(size.v);
                    break;
                case dir_LB_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    break;
                case dir_RB_:
                    size.h = rule.ht + rule.dp;
                    size.v = rule.wd;
                    pos_left(size.h);
                    break;
                default:;
                }
                backend_out[rule_node] (pdf, p, size);  /* pdf_place_rule(pdf, rule.ht, rule.wd); */
                cur.v += rule.ht + rule.dp;
            }
            goto NEXTP;
          MOVE_PAST:
            cur.v += rule.ht;
          NEXTP:
            p = vlink(p);
        }
        synch_pos_with_cur(pdf->posstruct, refpos, cur);
    }
    /* Finish vlist {\sl Sync\TeX} information record */
    if (synctexoption == 1)
        synctex_tsilv(this_box);

    if (pdf->o_mode == OMODE_DVI) {
        prune_movements(save_loc);      /* DVI! */
        if (cur_s > 0) {        /* DVI! */
            dvi_pop(save_loc);  /* DVI! */
            dvi = save_dvi;     /* DVI! */
        }
    }
    decr(cur_s);
    pdf->posstruct = refpos;
}
