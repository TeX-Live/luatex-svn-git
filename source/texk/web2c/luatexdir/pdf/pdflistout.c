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

#include "ptexlib.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

static void latelua(PDF pdf, halfword s, halfword r)
{
    pool_pointer b;             /* current character code position */
    b = pool_ptr;
    luacall(s, r);
    if (b < pool_ptr) {
        pdf_goto_pagemode(pdf);
        while (b < pool_ptr) {
            pdf_out(pdf, str_pool[b]);
            incr(b);
        }
        pdf_print_nl(pdf);
    }
}

static void do_late_lua(PDF pdf, halfword p)
{
    if (!doing_leaders) {
        expand_macros_in_tokenlist(p);
        latelua(pdf, def_ref, late_lua_name(p));
        flush_list(def_ref);
    }
}

/***********************************************************************/
/* TODO: remove these: */

#define push 141

#define dvi_out(A) do {                 \
    dvi_buf[dvi_ptr++]=A;               \
    if (dvi_ptr==dvi_limit) dvi_swap(); \
  } while (0)

/***********************************************************************/

node_output_function backend_out[MAX_NODE_TYPE + 1];
whatsit_output_function backend_out_whatsit[MAX_WHATSIT_TYPE + 1];

static void missing_node_function()
{
    pdf_error("pdflistout", "undefined node output function");
}

static void missing_whatsit_function()
{
    pdf_error("pdflistout", "undefined whatsit node output function");
}

void init_pdf_output_functions(PDF pdf)
{
    int i;

    pdf->o_mode = OMODE_PDF;

    for (i = 0; i < MAX_NODE_TYPE + 1; i++)
        backend_out[i] = &missing_node_function;

    for (i = 0; i < MAX_WHATSIT_TYPE + 1; i++)
        backend_out_whatsit[i] = &missing_whatsit_function;

    backend_out[rule_node] = &pdf_place_rule;   /* 2 */
    backend_out[glyph_node] = &pdf_place_glyph; /* 37 */
    /* ...these are all (?) */

    backend_out_whatsit[special_node] = &pdf_special;   /* 3 */
    backend_out_whatsit[pdf_literal_node] = &pdf_out_literal;   /* 8 */
    backend_out_whatsit[pdf_refxform_node] = &pdf_place_form;   /* 12 */
    backend_out_whatsit[pdf_refximage_node] = &pdf_place_image; /* 14 */
    backend_out_whatsit[pdf_annot_node] = &do_annot;    /* 15 */
    backend_out_whatsit[pdf_start_link_node] = &do_link;        /* 16 */
    backend_out_whatsit[pdf_end_link_node] = &end_link; /* 17 */
    backend_out_whatsit[pdf_dest_node] = &do_dest;      /* 19 */
    backend_out_whatsit[pdf_thread_node] = &do_thread;  /* 20 */
    backend_out_whatsit[pdf_end_thread_node] = &end_thread;     /* 22 */
    backend_out_whatsit[late_lua_node] = &do_late_lua;  /* 35 */
    backend_out_whatsit[pdf_colorstack_node] = &pdf_out_colorstack;     /* 39 */
    backend_out_whatsit[pdf_setmatrix_node] = &pdf_out_setmatrix;       /* 40 */
    backend_out_whatsit[pdf_save_node] = &pdf_out_save; /* 41 */
    backend_out_whatsit[pdf_restore_node] = &pdf_out_restore;   /* 42 */
}

void init_dvi_output_functions(PDF pdf)
{
    int i;

    pdf->o_mode = OMODE_DVI;

    for (i = 0; i < MAX_NODE_TYPE + 1; i++)
        backend_out[i] = &missing_node_function;

    for (i = 0; i < MAX_WHATSIT_TYPE + 1; i++)
        backend_out_whatsit[i] = &missing_whatsit_function;

    backend_out[rule_node] = &dvi_place_rule;   /* 2 */
    backend_out[glyph_node] = &dvi_place_glyph; /* 37 */
    /* ...these are all (?) */

    backend_out_whatsit[special_node] = &dvi_special;   /* 3 */
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
        if (is_char_node(q)) {
            w += glyph_width(q);        /* TODO no vertical support for now */
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
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
                if (vlink(no_break(q)) != null) {
                    w += simple_advance_width(no_break(q));
                }
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
                    w += pdf_width(q);
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
The |out_what| procedure takes care of outputting whatsit nodes for
|vlist_out| and |hlist_out|. */

void out_what(PDF pdf, halfword p)
{
    int j;                      /* write stream number */
    assert(subtype(p) == open_node ||
           subtype(p) == write_node || subtype(p) == close_node);
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
    scaled edge_h;
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
    scaledpos save_dvi;         /* DVI! what |dvi| should pop to */

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
        if (cur_s > 0)          /* DVI! */
            dvi_out(push);      /* DVI! */
        save_loc = dvi_offset + dvi_ptr /* DVI! */ ;
    }

    prev_p = this_box + list_offset;
    /* Create link annotations for the current hbox if needed */
    for (i = 1; i <= pdf->link_stack_ptr; i++) {
        assert(is_running(pdf_width(pdf->link_stack[i].link_node)));
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
                if (!dir_orthogonal
                    (dir_primary[localpos.dir], dir_primary[dir_TLT]))
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
            /* Output the non-|char_node| |p| for |pdf_hlist_out|
               and move to the next node */
            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in an hlist */

                if (!dir_orthogonal
                    (dir_primary[box_dir(p)], dir_primary[localpos.dir])) {
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
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[localpos.dir]))
                        basepoint.v = -(width(p) / 2);
                    else
                        basepoint.v = (width(p) / 2);
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
                    cur.h += effective_horizontal;
                } else {
                    edge_h = cur.h;
                    cur.h += basepoint.h;
                    cur.v = basepoint.v;
                    synch_pos_with_cur(pdf->posstruct, refpos, cur);
                    save_dvi = dvi;     /* DVI! */
                    if (type(p) == vlist_node)
                        vlist_out(pdf, p);
                    else
                        hlist_out(pdf, p);
                    dvi = save_dvi;     /* DVI! */
                    cur.h = edge_h + effective_horizontal;
                    cur.v = 0;
                }
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
                if (!dir_orthogonal
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
                /* Output the whatsit node |p| in |pdf_hlist_out| */
                switch (subtype(p)) {
                case pdf_literal_node:
                    backend_out_whatsit[pdf_literal_node] (pdf, p);     /* pdf_out_literal(pdf, p); */
                    break;
                case pdf_colorstack_node:
                    backend_out_whatsit[pdf_colorstack_node] (pdf, p);  /* pdf_out_colorstack(pdf, p); */
                    break;
                case pdf_setmatrix_node:
                    backend_out_whatsit[pdf_setmatrix_node] (pdf, p);   /* pdf_out_setmatrix(pdf, p); */
                    break;
                case pdf_save_node:
                    backend_out_whatsit[pdf_save_node] (pdf);   /* pdf_out_save(pdf); */
                    break;
                case pdf_restore_node:
                    backend_out_whatsit[pdf_restore_node] (pdf);        /* pdf_out_restore(pdf); */
                    break;
                case late_lua_node:
                    backend_out_whatsit[late_lua_node] (pdf, p);        /* do_late_lua(pdf, p); */
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf, pdf_obj_objnum(p))) {
                        append_object_list(pdf, obj_type_obj,
                                           pdf_obj_objnum(p));
                        set_obj_scheduled(pdf, pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                case pdf_refximage_node:
                    /* Output a Form node in a hlist */
                    /* Output an Image node in a hlist */
                    switch (box_direction(localpos.dir)) {
                    case dir_TL_:
                        pos_down(pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_depth(p));
                        break;
                    case dir_BL_:
                        pos_down(pdf_height(p));
                        break;
                    case dir_BR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_height(p));
                        break;
                    case dir_LT_:
                        pos_left(pdf_height(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        pos_left(pdf_depth(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        pos_left(pdf_height(p));
                        break;
                    case dir_RB_:
                        pos_left(pdf_depth(p));
                        break;
                    }
                    if (subtype(p) == pdf_refximage_node)
                        backend_out_whatsit[pdf_refximage_node] (pdf, pdf_ximage_idx(p));       /* pdf_place_image(pdf, pdf_ximage_idx(p)); */
                    else
                        backend_out_whatsit[pdf_refxform_node] (pdf, pdf_xform_objnum(p));      /* pdf_place_form(pdf, pdf_xform_objnum(p)); */
                    cur.h += pdf_width(p);
                    break;
                case pdf_annot_node:
                    backend_out_whatsit[pdf_annot_node] (pdf, p, this_box, cur);        /* do_annot(pdf, p, this_box, cur); */
                    break;
                case pdf_start_link_node:
                    backend_out_whatsit[pdf_start_link_node] (pdf, p, this_box, cur);   /* do_link(pdf, p, this_box, cur); */
                    break;
                case pdf_end_link_node:
                    backend_out_whatsit[pdf_end_link_node] (pdf);       /* end_link(pdf); */
                    break;
                case pdf_dest_node:
                    backend_out_whatsit[pdf_dest_node] (pdf, p, this_box, cur); /* do_dest(pdf, p, this_box, cur); */
                    break;
                case pdf_thread_node:
                    backend_out_whatsit[pdf_thread_node] (pdf, p, this_box, cur);       /* do_thread(pdf, p, this_box, cur); */
                    break;
                case pdf_start_thread_node:
                    pdf_error("ext4", "\\pdfstartthread ended up in hlist");
                    break;
                case pdf_end_thread_node:
                    pdf_error("ext4", "\\pdfendthread ended up in hlist");
                    break;
                case pdf_save_pos_node:
                    pdf_last_x_pos = localpos.pos.h;
                    pdf_last_y_pos = localpos.pos.v;
                    break;
                case special_node:
                    backend_out_whatsit[special_node] (pdf, p); /* pdf_special(pdf, p); */
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
                case local_par_node:
                case cancel_boundary_node:
                case user_defined_node:
                    break;
                case open_node:
                case write_node:
                case close_node:
                    out_what(pdf, p);
                    break;
                default:
                    confusion("ext4");
                    /* those will be pdf extension nodes in dvi mode, most likely */
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
                    if (!dir_orthogonal
                        (dir_primary[box_dir(leader_box)],
                         dir_primary[localpos.dir]))
                        leader_wd = width(leader_box);
                    else
                        leader_wd = height(leader_box) + depth(leader_box);
                    if ((leader_wd > 0) && (rule.wd > 0)) {
                        rule.wd = rule.wd + 10; /* compensate for floating-point rounding */
                        edge = cur.h + rule.wd;
                        lx = 0;
                        /* Let |cur.h| be the position of the first box, and set |leader_wd+lx|
                           to the spacing between corresponding parts of boxes */
                        {
                            if (subtype(p) == a_leaders) {
                                save_h = cur.h;
                                cur.h = leader_wd * (cur.h / leader_wd);
                                if (cur.h < save_h)
                                    cur.h += leader_wd;
                            } else {
                                lq = rule.wd / leader_wd;       /* the number of box copies */
                                lr = rule.wd % leader_wd;       /* the remaining space */
                                if (subtype(p) == c_leaders) {
                                    cur.h += lr / 2;
                                } else {
                                    lx = lr / (lq + 1);
                                    cur.h += (lr - (lq - 1) * lx) / 2;
                                }
                            }
                        }
                        while (cur.h + leader_wd <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.h|,
                               then advance |cur.h| by |leader_wd+lx| */

                            if (!dir_orthogonal
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
                            edge_h = cur.h;
                            cur.h += basepoint.h;
                            cur.v = basepoint.v;
                            synch_pos_with_cur(pdf->posstruct, refpos, cur);
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            save_dvi = dvi;     /* DVI! */
                            if (type(leader_box) == vlist_node)
                                vlist_out(pdf, leader_box);
                            else
                                hlist_out(pdf, leader_box);
                            dvi = save_dvi;     /* DVI! */
                            doing_leaders = outer_doing_leaders;
                            cur.h = edge_h + leader_wd + lx;
                            cur.v = 0;
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
                backend_out[rule_node] (pdf, size);     /* pdf_place_rule(pdf, rule.ht + rule.dp, rule.wd); */
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
        if (cur_s > 0)          /* DVI! */
            dvi_pop(save_loc);  /* DVI! */
    }
    decr(cur_s);
    pdf->posstruct = refpos;
}

void vlist_out(PDF pdf, halfword this_box)
{
    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    scaledpos cur, basepoint;
    scaledpos size = { 0, 0 };  /* rule dimensions */
    scaled effective_vertical;
    scaled save_v;              /* what |cur.v| should pop to */
    scaled top_edge;            /* the top coordinate for this box */
    scaled edge_v;
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
    scaledpos save_dvi;         /* DVI! what |dvi| should pop to */

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
        if (cur_s > 0)          /* DVI! */
            dvi_out(push);      /* DVI! */
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
            /* Output the non-|char_node| |p| for |pdf_vlist_out| */

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
                if (!dir_orthogonal
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
                    cur.h = basepoint.h;
                    edge_v = cur.v;
                    cur.v += basepoint.v;
                    synch_pos_with_cur(pdf->posstruct, refpos, cur);
                    save_dvi = dvi;     /* DVI! */
                    if (type(p) == vlist_node)
                        vlist_out(pdf, p);
                    else
                        hlist_out(pdf, p);
                    dvi = save_dvi;     /* DVI! */
                    cur.h = 0;
                    cur.v = edge_v + effective_vertical;
                }
                break;
            case rule_node:
                if (!dir_orthogonal
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
                /* Output the whatsit node |p| in |pdf_vlist_out| */
                switch (subtype(p)) {
                case pdf_literal_node:
                    backend_out_whatsit[pdf_literal_node] (pdf, p);     /* pdf_out_literal(pdf, p); */
                    break;
                case pdf_colorstack_node:
                    backend_out_whatsit[pdf_colorstack_node] (pdf, p);  /* pdf_out_colorstack(pdf, p); */
                    break;
                case pdf_setmatrix_node:
                    backend_out_whatsit[pdf_setmatrix_node] (pdf, p);   /* pdf_out_setmatrix(pdf, p); */
                    break;
                case pdf_save_node:
                    backend_out_whatsit[pdf_save_node] (pdf);   /* pdf_out_save(pdf); */
                    break;
                case pdf_restore_node:
                    backend_out_whatsit[pdf_restore_node] (pdf);        /* pdf_out_restore(pdf); */
                    break;
                case late_lua_node:
                    backend_out_whatsit[late_lua_node] (pdf, p);        /* do_late_lua(pdf, p); */
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf, pdf_obj_objnum(p))) {
                        append_object_list(pdf, obj_type_obj,
                                           pdf_obj_objnum(p));
                        set_obj_scheduled(pdf, pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                case pdf_refximage_node:
                    /* Output a Form node in a vlist */
                    /* Output an Image node in a vlist */
                    switch (box_direction(localpos.dir)) {
                    case dir_TL_:
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_BL_:
                        break;
                    case dir_BR_:
                        pos_left(pdf_width(p));
                        break;
                    case dir_LT_:
                        pos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        pos_left(pdf_height(p) + pdf_depth(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        break;
                    case dir_RB_:
                        pos_left(pdf_height(p) + pdf_depth(p));
                        break;
                    default:
                        break;
                    }
                    if (subtype(p) == pdf_refximage_node)
                        backend_out_whatsit[pdf_refximage_node] (pdf, pdf_ximage_idx(p));       /* pdf_place_image(pdf, pdf_ximage_idx(p)); */
                    else
                        backend_out_whatsit[pdf_refxform_node] (pdf, pdf_xform_objnum(p));      /* pdf_place_form(pdf, pdf_xform_objnum(p)); */
                    cur.v += pdf_height(p) + pdf_depth(p);
                    break;
                case pdf_annot_node:
                    backend_out_whatsit[pdf_annot_node] (pdf, p, this_box, cur);        /* do_annot(pdf, p, this_box, cur); */
                    break;
                case pdf_start_link_node:
                    pdf_error("ext4", "\\pdfstartlink ended up in vlist");
                    break;
                case pdf_end_link_node:
                    pdf_error("ext4", "\\pdfendlink ended up in vlist");
                    break;
                case pdf_dest_node:
                    backend_out_whatsit[pdf_dest_node] (pdf, p, this_box, cur); /* do_dest(pdf, p, this_box, cur); */
                    break;
                case pdf_thread_node:
                case pdf_start_thread_node:
                    backend_out_whatsit[pdf_thread_node] (pdf, p, this_box, cur);       /* do_thread(pdf, p, this_box, cur); */
                    break;
                case pdf_end_thread_node:
                    backend_out_whatsit[pdf_end_thread_node] (pdf);     /* end_thread(pdf); */
                    break;
                case pdf_save_pos_node:
                    pdf_last_x_pos = localpos.pos.h;
                    pdf_last_y_pos = localpos.pos.v;
                    break;
                case special_node:
                    backend_out_whatsit[special_node] (pdf, p); /* pdf_special(pdf, p); */
                    break;
                case local_par_node:
                case cancel_boundary_node:
                case user_defined_node:
                    break;
                case open_node:
                case write_node:
                case close_node:
                    out_what(pdf, p);
                    break;
                default:
                    confusion("ext4");
                    /* those will be pdf extension nodes in dvi mode, most likely */

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
                        /* TODO: module can be shared with DVI */
                        if (subtype(p) == a_leaders) {
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
                            cur.h = shift_amount(leader_box);
                            cur.v += height(leader_box);
                            save_v = cur.v;
                            synch_pos_with_cur(pdf->posstruct, refpos, cur);
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            save_dvi = dvi;     /* DVI! */
                            if (type(leader_box) == vlist_node)
                                vlist_out(pdf, leader_box);
                            else
                                hlist_out(pdf, leader_box);
                            dvi = save_dvi;     /* DVI! */
                            doing_leaders = outer_doing_leaders;
                            cur.h = 0;
                            cur.v =
                                save_v - height(leader_box) + leader_ht + lx;
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
                backend_out[rule_node] (pdf, size);     /* pdf_place_rule(pdf, rule.ht, rule.wd); */
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
        if (cur_s > 0)          /* DVI! */
            dvi_pop(save_loc);  /* DVI! */
    }
    decr(cur_s);
    pdf->posstruct = refpos;
}
