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

/*
This code scans forward to the ending |dir_node| while keeping
track of the needed width in |w|. When it finds the node that will end
this segment, it stores the accumulated with in the |dir_dvi_h| field
of that end node, so that when that node is found later in the
processing, the correct glue correction can be applied.
*/

scaled simple_advance_width(halfword p)
{
    halfword q = p;
    scaled w = 0;
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q)) {
            w = w + glyph_width(q);
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w = w + width(q);
                break;
            case disc_node:
                if (vlink(no_break(q)) != null)
                    w = w + simple_advance_width(no_break(q));
            default:
                break;
            }
        }
    }
    return w;
}


void calculate_width_to_enddir(halfword p, real cur_glue, scaled cur_g,
                               halfword this_box, scaled * setw,
                               halfword * settemp_ptr)
{
    int dir_nest = 1;
    halfword q = p;
    scaled w = 0;
    halfword temp_ptr = *settemp_ptr;
    halfword g;                 /* this is normally a global variable, but that is just too hideous */
    /* to me, it looks like the next is needed. but Aleph doesn't do that, so let us not do it either */
    real glue_temp;             /* glue value before rounding */
    /* |w:=w-cur_g; cur_glue:=0;| */
    integer g_sign = glue_sign(this_box);
    integer g_order = glue_order(this_box);
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q)) {
            w = w + glyph_width(q);     /* TODO no vertical support for now */
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w = w + width(q);
                break;
            case glue_node:
                g = glue_ptr(q);
                w = w + width(g) - cur_g;
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
                w = w + cur_g;
                break;
            case disc_node:
                if (vlink(no_break(q)) != null) {
                    w = w + simple_advance_width(no_break(q));
                }
                break;
            case math_node:
                w = w + surround(q);
                break;
            case whatsit_node:
                if (subtype(q) == dir_node) {
                    if (dir_dir(q) >= 0)
                        incr(dir_nest);
                    else
                        decr(dir_nest);
                    if (dir_nest == 0) {
                        dir_dvi_h(q) = w;
                        temp_ptr = q;
                        q = null;
                    }
                } else if ((subtype(q) == pdf_refxform_node) ||
                           (subtype(q) == pdf_refximage_node)) {
                    w = w + pdf_width(q);
                }
                break;
            default:
                break;
            }
        }
    }
    *setw = w;
    *settemp_ptr = temp_ptr;
}


/* The implementation of procedure |pdf_hlist_out| is similiar to |hlist_out| */

void pdf_hlist_out(PDF pdf)
{
    /*label move_past, fin_rule, next_p; */
    scaled base_line;           /* the baseline coordinate for this box */
    scaled w;                   /*  temporary value for directional width calculation  */
    scaled edge_v;
    scaled edge_h;

    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    scaled effective_horizontal;
    scaled basepoint_horizontal;
    scaled basepoint_vertical;
    integer save_direction;
    halfword dvi_ptr, dir_ptr, dir_tmp;
    scaled left_edge;           /* the left coordinate for this box */
    scaled save_h;              /* what |cur.h| should pop to */
    halfword this_box;          /* pointer to containing box */
    integer g_order;            /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p, q;              /* current position in the hlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_wd;           /* width of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    scaled edge;                /* right edge of sub-box or leader space */
    halfword prev_p;            /* one step behind |p| */
    real glue_temp;             /* glue value before rounding */
    real cur_glue;              /* glue seen so far */
    scaled cur_g;               /* rounded equivalent of |cur_glue| times the glue ratio */
    scaledpos cur_orig;
    int i;                      /* index to scan |pdf_link_stack| */
    cur_g = 0;
    cur_glue = 0.0;
    this_box = temp_ptr;
    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.dir = box_dir(this_box);

    box_pos = refpos->pos;      /* all box_pos will go away */
    set_to_zero(cur);

    save_direction = dvi_direction;
    dvi_direction = box_dir(this_box);
    dvi_ptr = 0;
    lx = 0;
    /* Initialize |dir_ptr| for |ship_out| */
    {
        dir_ptr = null;
        push_dir(dvi_direction);
        dir_dvi_ptr(dir_ptr) = dvi_ptr;
    }
    incr(cur_s);
    base_line = cur.v;
    left_edge = cur.h;
    prev_p = this_box + list_offset;
    /* Create link annotations for the current hbox if needed */
    for (i = 1; i <= pdf_link_stack_ptr; i++) {
        assert(is_running(pdf_width(pdf_link_stack[i].link_node)));
        if (pdf_link_stack[i].nesting_level == cur_s) {
            cur_orig.h = left_edge;
            cur_orig.v = base_line;
            append_link(pdf, this_box, cur_orig, i);
        }
    }

    /* Start hlist {\sl Sync\TeX} information record */
    synctex_hlist(this_box);

    while (p != null) {
        if (is_char_node(p)) {
            do {
                if (x_displace(p) != 0)
                    cur.h = cur.h + x_displace(p);
                if (y_displace(p) != 0)
                    cur.v = cur.v - y_displace(p);
                (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                cur.h += output_one_char(pdf, font(p), character(p));
                if (x_displace(p) != 0)
                    cur.h = cur.h - x_displace(p);
                if (y_displace(p) != 0)
                    cur.v = cur.v + y_displace(p);
                p = vlink(p);
            } while (is_char_node(p));
            /* Record current point {\sl Sync\TeX} information */
            synctex_current();
        } else {
            /* Output the non-|char_node| |p| for |pdf_hlist_out|
               and move to the next node */
            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in an hlist */

                if (!
                    (dir_orthogonal
                     (dir_primary[box_dir(p)], dir_primary[dvi_direction]))) {
                    effective_horizontal = width(p);
                    basepoint_vertical = 0;
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[dvi_direction]))
                        basepoint_horizontal = width(p);
                    else
                        basepoint_horizontal = 0;
                } else {
                    effective_horizontal = height(p) + depth(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = height(p);
                        else
                            basepoint_horizontal = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = depth(p);
                        else
                            basepoint_horizontal = height(p);
                    }
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[dvi_direction]))
                        basepoint_vertical = -(width(p) / 2);
                    else
                        basepoint_vertical = (width(p) / 2);
                }
                if (!is_mirrored(dvi_direction))
                    basepoint_vertical = basepoint_vertical + shift_amount(p);  /* shift the box `down' */
                else
                    basepoint_vertical = basepoint_vertical - shift_amount(p);  /* shift the box `up' */
                if (list_ptr(p) == null) {
                    /* Record void list {\sl Sync\TeX} information */
                    if (type(p) == vlist_node)
                        synctex_void_vlist(p, this_box);
                    else
                        synctex_void_hlist(p, this_box);

                    cur.h = cur.h + effective_horizontal;
                } else {
                    temp_ptr = p;
                    edge = cur.h;
                    cur.h = cur.h + basepoint_horizontal;
                    edge_v = cur.v;
                    cur.v = base_line + basepoint_vertical;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    if (type(p) == vlist_node)
                        pdf_vlist_out(pdf);
                    else
                        pdf_hlist_out(pdf);
                    box_pos = refpos->pos;      /* all box_pos will go away */
                    cur.h = edge + effective_horizontal;
                    cur.v = base_line;
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
                if (!
                    (dir_orthogonal
                     (dir_primary[rule_dir(p)], dir_primary[dvi_direction]))) {
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
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    pdf_out_literal(pdf, p);
                    break;
                case pdf_colorstack_node:
                    pdf_out_colorstack(pdf, p);
                    break;
                case pdf_setmatrix_node:
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    pdf_out_setmatrix(pdf, p);
                    break;
                case pdf_save_node:
                    pdf_out_save(pdf);
                    break;
                case pdf_restore_node:
                    pdf_out_restore(pdf);
                    break;
                case late_lua_node:
                    do_late_lua(pdf, p);
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf, pdf_obj_objnum(p))) {
                        pdf_append_list(pdf_obj_objnum(p), pdf_obj_list);
                        set_obj_scheduled(pdf, pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                    /* Output a Form node in a hlist */
                    cur.v = base_line;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        lpos_down(pdf_depth(p));
                        break;
                    case dir_TR_:
                        lpos_down(pdf_depth(p));
                        lpos_left(pdf_width(p));
                        break;
                    }
                    pdf_place_form(pdf, pdf_xform_objnum(p));
                    edge = cur.h + pdf_width(p);
                    cur.h = edge;
                    break;
                case pdf_refximage_node:
                    /* Output an Image node in a hlist */
                    cur.v = base_line;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        lpos_down(pdf_depth(p));
                        break;
                    case dir_TR_:
                        lpos_left(pdf_width(p));
                        lpos_down(pdf_depth(p));
                        break;
                    case dir_BL_:
                        lpos_down(pdf_height(p));
                        break;
                    case dir_BR_:
                        lpos_left(pdf_width(p));
                        lpos_down(pdf_height(p));
                        break;
                    case dir_LT_:
                        lpos_left(pdf_height(p));
                        lpos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        lpos_left(pdf_depth(p));
                        lpos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        lpos_left(pdf_height(p));
                        break;
                    case dir_RB_:
                        lpos_left(pdf_depth(p));
                        break;
                    }
                    pdf_place_image(pdf, pdf_ximage_idx(p));
                    edge = cur.h + pdf_width(p);
                    cur.h = edge;
                    break;
                case pdf_annot_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = base_line;
                    do_annot(pdf, p, this_box, cur_orig);
                    break;
                case pdf_start_link_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = base_line;
                    do_link(pdf, p, this_box, cur_orig);
                    break;
                case pdf_end_link_node:
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    end_link(pdf);
                    break;
                case pdf_dest_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = base_line;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    do_dest(pdf, p, this_box, cur_orig);
                    break;
                case pdf_thread_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = base_line;
                    do_thread(pdf, p, this_box, cur_orig);
                    break;
                case pdf_start_thread_node:
                    pdf_error("ext4", "\\pdfstartthread ended up in hlist");
                    break;
                case pdf_end_thread_node:
                    pdf_error("ext4", "\\pdfendthread ended up in hlist");
                    break;
                case pdf_save_pos_node:
                    /* Save current position */
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    pdf_last_x_pos = localpos.pos.h;
                    pdf_last_y_pos = localpos.pos.v;
                    break;
                case special_node:
                    pdf_special(pdf, p);
                    break;
                case dir_node:
                    /* Output a reflection instruction if the direction has changed */
                    /* TODO: this whole case code block is the same in DVI mode */
                    if (dir_dir(p) >= 0) {
                        push_dir_node(p);
                        /* (PDF) Calculate the needed width to the matching |enddir|, and store it in |w|, as
                           well as in the enddirs |dir_dvi_h| */
                        calculate_width_to_enddir(p, cur_glue, cur_g, this_box,
                                                  &w, &temp_ptr);

                        if ((dir_opposite
                             (dir_secondary[dir_dir(dir_ptr)],
                              dir_secondary[dvi_direction]))
                            ||
                            (dir_eq
                             (dir_secondary[dir_dir(dir_ptr)],
                              dir_secondary[dvi_direction]))) {
                            dir_cur_h(temp_ptr) = cur.h + w;
                            if (dir_opposite
                                (dir_secondary[dir_dir(dir_ptr)],
                                 dir_secondary[dvi_direction]))
                                cur.h = cur.h + w;
                        } else {
                            dir_cur_h(temp_ptr) = cur.h;
                        }
                        dir_cur_v(temp_ptr) = cur.v;
                        dir_box_pos_h(temp_ptr) = refpos->pos.h;
                        dir_box_pos_v(temp_ptr) = refpos->pos.v;
                        pos = new_synch_pos_with_cur(&localpos, refpos, cur);   /* no need for |synch_dvi_with_cur|, as there is no DVI grouping */
                        refpos->pos = pos;      /* fake a nested |hlist_out| */
                        box_pos = refpos->pos;
                        localpos.dir = dir_dir(dir_ptr);
                        dvi_direction = localpos.dir;
                        set_to_zero(cur);
                    } else {
                        pop_dir_node();
                        refpos->pos.h = dir_box_pos_h(p);
                        refpos->pos.v = dir_box_pos_v(p);
                        box_pos = refpos->pos;
                        if (dir_ptr != null)
                            localpos.dir = dir_dir(dir_ptr);
                        else
                            localpos.dir = 0;
                        dvi_direction = localpos.dir;
                        cur.h = dir_cur_h(p);
                        cur.v = dir_cur_v(p);
                    }
                    break;
                default:
                    out_what(p);
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
                    if (!
                        (dir_orthogonal
                         (dir_primary[box_dir(leader_box)],
                          dir_primary[dvi_direction])))
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
                                cur.h =
                                    left_edge +
                                    leader_wd * ((cur.h - left_edge) /
                                                 leader_wd);
                                if (cur.h < save_h)
                                    cur.h = cur.h + leader_wd;
                            } else {
                                lq = rule.wd / leader_wd;       /* the number of box copies */
                                lr = rule.wd % leader_wd;       /* the remaining space */
                                if (subtype(p) == c_leaders) {
                                    cur.h = cur.h + (lr / 2);
                                } else {
                                    lx = lr / (lq + 1);
                                    cur.h = cur.h + ((lr - (lq - 1) * lx) / 2);
                                }
                            }
                        }
                        while (cur.h + leader_wd <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.h|,
                               then advance |cur.h| by |leader_wd+lx| */

                            if (!
                                (dir_orthogonal
                                 (dir_primary[box_dir(leader_box)],
                                  dir_primary[dvi_direction]))) {
                                basepoint_vertical = 0;
                                if (dir_opposite
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_secondary[dvi_direction]))
                                    basepoint_horizontal = width(leader_box);
                                else
                                    basepoint_horizontal = 0;
                            } else {
                                if (!is_mirrored(box_dir(leader_box))) {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[dvi_direction]))
                                        basepoint_horizontal =
                                            height(leader_box);
                                    else
                                        basepoint_horizontal =
                                            depth(leader_box);
                                } else {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[dvi_direction]))
                                        basepoint_horizontal =
                                            depth(leader_box);
                                    else
                                        basepoint_horizontal =
                                            height(leader_box);
                                }
                                if (dir_eq
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_primary[dvi_direction]))
                                    basepoint_vertical =
                                        -(width(leader_box) / 2);
                                else
                                    basepoint_vertical =
                                        (width(leader_box) / 2);
                            }
                            if (!is_mirrored(dvi_direction))
                                basepoint_vertical = basepoint_vertical + shift_amount(leader_box);     /* shift the box `down' */
                            else
                                basepoint_vertical = basepoint_vertical - shift_amount(leader_box);     /* shift the box `up' */
                            temp_ptr = leader_box;
                            edge_h = cur.h;
                            cur.h = cur.h + basepoint_horizontal;
                            edge_v = cur.v;
                            cur.v = base_line + basepoint_vertical;
                            (void) new_synch_pos_with_cur(&localpos, refpos,
                                                          cur);
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                pdf_vlist_out(pdf);
                            else
                                pdf_hlist_out(pdf);
                            box_pos = refpos->pos;
                            doing_leaders = outer_doing_leaders;
                            cur.h = edge_h + leader_wd + lx;
                            cur.v = base_line;
                        }
                        cur.h = edge - 10;
                        goto NEXTP;
                    }
                }
                goto MOVE_PAST;
                break;
            case margin_kern_node:
                cur.h = cur.h + width(p);
                break;
            case kern_node:
                /* Record |kern_node| {\sl Sync\TeX} information */
                synctex_kern(p, this_box);
                cur.h = cur.h + width(p);
                break;
            case math_node:
                /* Record |math_node| {\sl Sync\TeX} information */
                synctex_math(p, this_box);
                cur.h = cur.h + surround(p);
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
            if (((rule.ht + rule.dp) > 0) && (rule.wd > 0)) {   /* we don't output empty rules */
                (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                switch (box_direction(dvi_direction)) {
                case dir_TL_:
                    lpos_down(rule.dp);
                    pdf_place_rule(pdf, rule.wd, rule.ht + rule.dp);
                    break;
                case dir_BL_:
                    lpos_down(rule.ht);
                    pdf_place_rule(pdf, rule.wd, rule.ht + rule.dp);
                    break;
                case dir_TR_:
                    lpos_left(rule.wd);
                    lpos_down(rule.dp);
                    pdf_place_rule(pdf, rule.wd, rule.ht + rule.dp);
                    break;
                case dir_BR_:
                    lpos_left(rule.wd);
                    lpos_down(rule.ht);
                    pdf_place_rule(pdf, rule.wd, rule.ht + rule.dp);
                    break;
                case dir_LT_:
                    lpos_left(rule.ht);
                    lpos_down(rule.wd);
                    pdf_place_rule(pdf, rule.ht + rule.dp, rule.wd);
                    break;
                case dir_RT_:
                    lpos_left(rule.dp);
                    lpos_down(rule.wd);
                    pdf_place_rule(pdf, rule.ht + rule.dp, rule.wd);
                    break;
                case dir_LB_:
                    lpos_left(rule.ht);
                    pdf_place_rule(pdf, rule.ht + rule.dp, rule.wd);
                    break;
                case dir_RB_:
                    lpos_left(rule.dp);
                    pdf_place_rule(pdf, rule.ht + rule.dp, rule.wd);
                    break;
                }
            }
          MOVE_PAST:
            cur.h = cur.h + rule.wd;
            /* Record horizontal |rule_node| or |glue_node| {\sl Sync\TeX} information */
            synctex_horizontal_rule_or_glue(p, this_box);
          NEXTP:
            prev_p = p;
            p = vlink(p);
        }
    }

    /* Finish hlist {\sl Sync\TeX} information record */
    synctex_tsilh(this_box);

    decr(cur_s);
    dvi_direction = save_direction;
    /* DIR: Reset |dir_ptr| */
    {
        while (dir_ptr != null)
            pop_dir_node();
    }
    pdf->posstruct = refpos;
}


/* The |pdf_vlist_out| routine is similar to |pdf_hlist_out|, but a bit simpler. */

void pdf_vlist_out(PDF pdf)
{
    scaled left_edge;           /* the left coordinate for this box */
    scaled top_edge;            /* the top coordinate for this box */

    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    scaled save_v;              /* what |cur.v| should pop to */
    halfword this_box;          /* pointer to containing box */
    glue_ord g_order;           /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p;                 /* current position in the vlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_ht;           /* height of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    scaled edge;                /* bottom boundary of leader space */
    real glue_temp;             /* glue value before rounding */
    real cur_glue;              /* glue seen so far */
    scaled cur_g;               /* rounded equivalent of |cur_glue| times the glue ratio */
    scaledpos cur_orig;
    integer save_direction;
    scaled effective_vertical;
    scaled basepoint_horizontal;
    scaled basepoint_vertical;
    scaled edge_v;
    cur_g = 0;
    cur_glue = 0.0;
    this_box = temp_ptr;
    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.dir = box_dir(this_box);

    box_pos = refpos->pos;
    set_to_zero(cur);

    save_direction = dvi_direction;
    dvi_direction = box_dir(this_box);
    incr(cur_s);
    left_edge = cur.h;
    /* Start vlist {\sl Sync\TeX} information record */
    synctex_vlist(this_box);

    cur.v = cur.v - height(this_box);
    top_edge = cur.v;
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
                if (!
                    (dir_orthogonal
                     (dir_primary[box_dir(p)], dir_primary[dvi_direction]))) {
                    effective_vertical = height(p) + depth(p);
                    if ((type(p) == hlist_node) && is_mirrored(box_dir(p)))
                        basepoint_vertical = depth(p);
                    else
                        basepoint_vertical = height(p);
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[dvi_direction]))
                        basepoint_horizontal = width(p);
                    else
                        basepoint_horizontal = 0;
                } else {
                    effective_vertical = width(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = height(p);
                        else
                            basepoint_horizontal = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = depth(p);
                        else
                            basepoint_horizontal = height(p);
                    }
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[dvi_direction]))
                        basepoint_vertical = 0;
                    else
                        basepoint_vertical = width(p);
                }
                basepoint_horizontal = basepoint_horizontal + shift_amount(p);  /* shift the box `right' */
                if (list_ptr(p) == null) {
                    cur.v = cur.v + effective_vertical;
                    /* Record void list {\sl Sync\TeX} information */
                    if (type(p) == vlist_node)
                        synctex_void_vlist(p, this_box);
                    else
                        synctex_void_hlist(p, this_box);

                } else {
                    edge_v = cur.v;
                    cur.h = left_edge + basepoint_horizontal;
                    cur.v = cur.v + basepoint_vertical;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    temp_ptr = p;
                    if (type(p) == vlist_node)
                        pdf_vlist_out(pdf);
                    else
                        pdf_hlist_out(pdf);
                    box_pos = refpos->pos;
                    cur.h = left_edge;
                    cur.v = edge_v + effective_vertical;
                }
                break;
            case rule_node:
                if (!
                    (dir_orthogonal
                     (dir_primary[rule_dir(p)], dir_primary[dvi_direction]))) {
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
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    pdf_out_literal(pdf, p);
                    break;
                case pdf_colorstack_node:
                    pdf_out_colorstack(pdf, p);
                    break;
                case pdf_setmatrix_node:
                    pdf_out_setmatrix(pdf, p);
                    break;
                case pdf_save_node:
                    pdf_out_save(pdf);
                    break;
                case pdf_restore_node:
                    pdf_out_restore(pdf);
                    break;
                case late_lua_node:
                    do_late_lua(pdf, p);
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf, pdf_obj_objnum(p))) {
                        pdf_append_list(pdf_obj_objnum(p), pdf_obj_list);
                        set_obj_scheduled(pdf, pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                    /* Output a Form node in a vlist */
                    cur.h = left_edge;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        lpos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_TR_:
                        lpos_left(pdf_width(p));
                        lpos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    default:
                        break;
                    }
                    pdf_place_form(pdf, pdf_xform_objnum(p));
                    cur.v = cur.v + pdf_height(p) + pdf_depth(p);
                    break;
                case pdf_refximage_node:
                    /* Output an Image node in a vlist */
                    cur.h = left_edge;
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        lpos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_TR_:
                        lpos_left(pdf_width(p));
                        lpos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_BL_:
                        break;
                    case dir_BR_:
                        lpos_left(pdf_width(p));
                        break;
                    case dir_LT_:
                        lpos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        lpos_left(pdf_height(p) + pdf_depth(p));
                        lpos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        break;
                    case dir_RB_:
                        lpos_left(pdf_height(p) + pdf_depth(p));
                        break;
                    default:
                        break;
                    }
                    pdf_place_image(pdf, pdf_ximage_idx(p));
                    cur.v = cur.v + pdf_height(p) + pdf_depth(p);
                    break;
                case pdf_annot_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = top_edge + height(this_box);
                    do_annot(pdf, p, this_box, cur_orig);
                    break;
                case pdf_start_link_node:
                    pdf_error("ext4", "\\pdfstartlink ended up in vlist");
                    break;
                case pdf_end_link_node:
                    pdf_error("ext4", "\\pdfendlink ended up in vlist");
                    break;
                case pdf_dest_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = top_edge + height(this_box);
                    do_dest(pdf, p, this_box, cur_orig);
                    break;
                case pdf_thread_node:
                case pdf_start_thread_node:
                    cur_orig.h = left_edge;
                    cur_orig.v = top_edge + height(this_box);
                    do_thread(pdf, p, this_box, cur_orig);
                    break;
                case pdf_end_thread_node:
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    end_thread(pdf);
                    break;
                case pdf_save_pos_node:
                    /* Save current position */
                    (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                    pdf_last_x_pos = localpos.pos.h;
                    pdf_last_y_pos = localpos.pos.v;
                    break;
                case special_node:
                    pdf_special(pdf, p);
                    break;
                default:
                    out_what(p);
                    break;
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
                                cur.v = cur.v + leader_ht;
                        } else {
                            lq = rule.ht / leader_ht;   /* the number of box copies */
                            lr = rule.ht % leader_ht;   /* the remaining space */
                            if (subtype(p) == c_leaders) {
                                cur.v = cur.v + (lr / 2);
                            } else {
                                lx = lr / (lq + 1);
                                cur.v = cur.v + ((lr - (lq - 1) * lx) / 2);
                            }
                        }

                        while (cur.v + leader_ht <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.v|,
                               then advance |cur.v| by |leader_ht+lx| */
                            cur.h = left_edge + shift_amount(leader_box);
                            cur.v = cur.v + height(leader_box);
                            save_v = cur.v;
                            (void) new_synch_pos_with_cur(&localpos, refpos,
                                                          cur);
                            temp_ptr = leader_box;
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                pdf_vlist_out(pdf);
                            else
                                pdf_hlist_out(pdf);
                            box_pos = refpos->pos;
                            doing_leaders = outer_doing_leaders;
                            cur.h = left_edge;
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
                cur.v = cur.v + width(p);
                break;
            default:
                break;
            }
            goto NEXTP;
          FIN_RULE:
            /* (\pdfTeX) Output a rule in a vlist, |goto next_p| */
            if (is_running(rule.wd))
                rule.wd = width(this_box);
            rule.ht = rule.ht + rule.dp;        /* this is the rule thickness */
            if ((rule.ht > 0) && (rule.wd > 0)) {       /* we don't output empty rules */
                (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                switch (box_direction(dvi_direction)) {
                case dir_TL_:
                    lpos_down(rule.ht);
                    pdf_place_rule(pdf, rule.wd, rule.ht);
                    break;
                case dir_BL_:
                    pdf_place_rule(pdf, rule.wd, rule.ht);
                    break;
                case dir_TR_:
                    lpos_left(rule.wd);
                    lpos_down(rule.ht);
                    pdf_place_rule(pdf, rule.wd, rule.ht);
                    break;
                case dir_BR_:
                    lpos_left(rule.wd);
                    pdf_place_rule(pdf, rule.wd, rule.ht);
                    break;
                case dir_LT_:
                    lpos_down(rule.wd);
                    pdf_place_rule(pdf, rule.ht, rule.wd);
                    break;
                case dir_RT_:
                    lpos_left(rule.ht);
                    lpos_down(rule.wd);
                    pdf_place_rule(pdf, rule.ht, rule.wd);
                    break;
                case dir_LB_:
                    pdf_place_rule(pdf, rule.ht, rule.wd);
                    break;
                case dir_RB_:
                    lpos_left(rule.ht);
                    pdf_place_rule(pdf, rule.ht, rule.wd);
                    break;
                }
                cur.v = cur.v + rule.ht;
            }
            goto NEXTP;
          MOVE_PAST:
            cur.v = cur.v + rule.ht;
          NEXTP:
            p = vlink(p);
        }
    }
    /* Finish vlist {\sl Sync\TeX} information record */
    synctex_tsilv(this_box);

    decr(cur_s);
    dvi_direction = save_direction;
    pdf->posstruct = refpos;
}
