/* texnodes.c
   
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#include "luatex-api.h"
#include <ptexlib.h>
#include "commands.h"

#undef name

#define noDEBUG

static const char _svn_version[] =
    "$Id$ $URL$";

#define adjust_pre subtype
#define attribute(A) eqtb[attribute_base+(A)].cint

#define dir_par(A) eqtb[dir_base+(A)].cint
#define text_direction dir_par(text_direction_code)
#define uc_hyph int_par(uc_hyph_code)
#define cur_lang int_par(cur_lang_code)
#define left_hyphen_min int_par(left_hyphen_min_code)
#define right_hyphen_min int_par(right_hyphen_min_code)

#define MAX_CHAIN_SIZE 13

volatile memory_word *varmem = NULL;

#ifndef NDEBUG
char *varmem_sizes = NULL;
#endif

halfword var_mem_max = 0;
halfword rover = 0;

halfword free_chain[MAX_CHAIN_SIZE] = { null };

static integer my_prealloc = 0;

int fix_node_lists = 1;

int free_error_seen = 0;
int copy_error_seen = 0;

halfword slow_get_node(integer s);      /* defined below */
int copy_error(halfword p);     /* define below */

#define fake_node 100
#define fake_node_size 2
#define fake_node_name "fake"

#define variable_node_size 2


char *node_fields_list[] = { "attr", "width", "depth", "height", "dir", "shift",
    "glue_order", "glue_sign", "glue_set", "list", NULL
};
char *node_fields_rule[] = { "attr", "width", "depth", "height", "dir", NULL };
char *node_fields_insert[] =
    { "attr", "cost", "depth", "height", "spec", "list", NULL };
char *node_fields_mark[] = { "attr", "class", "mark", NULL };
char *node_fields_adjust[] = { "attr", "list", NULL };
char *node_fields_disc[] = { "attr", "pre", "post", "replace", NULL };
char *node_fields_math[] = { "attr", "surround", NULL };
char *node_fields_glue[] = { "attr", "spec", "leader", NULL };
char *node_fields_kern[] = { "attr", "kern", NULL };
char *node_fields_penalty[] = { "attr", "penalty", NULL };

char *node_fields_unset[] =
    { "attr", "width", "depth", "height", "dir", "shrink",
    "glue_order", "glue_sign", "stretch", "span", "list", NULL
};
char *node_fields_margin_kern[] = { "attr", "width", "glyph", NULL };

char *node_fields_glyph[] =
    { "attr", "char", "font", "lang", "left", "right", "uchyph",
    "components", "xoffset", "yoffset", NULL
};
char *node_fields_style[] = { "attr", "style", NULL };
char *node_fields_choice[] =
    { "attr", "display", "text", "script", "scriptscript", NULL };
char *node_fields_ord[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_op[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_bin[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_rel[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_open[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_close[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_punct[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_inner[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_under[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_over[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_vcenter[] = { "attr", "nucleus", "sub", "sup", NULL };
char *node_fields_radical[] =
    { "attr", "nucleus", "sub", "sup", "left", "degree", NULL };
char *node_fields_fraction[] =
    { "attr", "width", "num", "denom", "left", "right", NULL };
char *node_fields_accent[] =
    { "attr", "nucleus", "sub", "sup", "accent", "bot_accent", NULL };
char *node_fields_fence[] = { "attr", "delim", NULL };
char *node_fields_math_char[] = { "attr", "fam", "char", NULL };
char *node_fields_sub_box[] = { "attr", "list", NULL };
char *node_fields_sub_mlist[] = { "attr", "list", NULL };
char *node_fields_math_text_char[] = { "attr", "fam", "char", NULL };
char *node_fields_delim[] =
    { "attr", "small_fam", "small_char", "large_fam", "large_char", NULL };

char *node_fields_inserting[] =
    { "height", "last_ins_ptr", "best_ins_ptr", NULL };

char *node_fields_splitup[] = { "height", "last_ins_ptr", "best_ins_ptr",
    "broken_ptr", "broken_ins", NULL
};

char *node_fields_action[] = { "action_type", "named_id", "action_id",
    "file", "new_window", "data", "ref_count", NULL
};
char *node_fields_attribute[] = { "number", "value", NULL };

char *node_fields_glue_spec[] = { "width", "stretch", "shrink",
    "stretch_order", "shrink_order", "ref_count", NULL
};
char *node_fields_attribute_list[] = { NULL };

char *node_fields_whatsit_open[] =
    { "attr", "stream", "name", "area", "ext", NULL };
char *node_fields_whatsit_write[] = { "attr", "stream", "data", NULL };
char *node_fields_whatsit_close[] = { "attr", "stream", NULL };
char *node_fields_whatsit_special[] = { "attr", "data", NULL };

char *node_fields_whatsit_local_par[] =
    { "attr", "pen_inter", "pen_broken", "dir",
    "box_left", "box_left_width", "box_right", "box_right_width", NULL
};
char *node_fields_whatsit_dir[] =
    { "attr", "dir", "level", "dvi_ptr", "dvi_h", NULL };

char *node_fields_whatsit_pdf_literal[] = { "attr", "mode", "data", NULL };
char *node_fields_whatsit_pdf_refobj[] = { "attr", "objnum", NULL };
char *node_fields_whatsit_pdf_refxform[] =
    { "attr", "width", "height", "depth", "objnum", NULL };
char *node_fields_whatsit_pdf_refximage[] =
    { "attr", "width", "height", "depth", "objnum", NULL };
char *node_fields_whatsit_pdf_annot[] =
    { "attr", "width", "height", "depth", "objnum", "data", NULL };
char *node_fields_whatsit_pdf_start_link[] =
    { "attr", "width", "height", "depth",
    "objnum", "link_attr", "action", NULL
};
char *node_fields_whatsit_pdf_end_link[] = { "attr", NULL };

char *node_fields_whatsit_pdf_dest[] = { "attr", "width", "height", "depth",
    "named_id", "dest_id", "dest_type", "xyz_zoom", "objnum", NULL
};

char *node_fields_whatsit_pdf_thread[] = { "attr", "width", "height", "depth",
    "named_id", "thread_id", "thread_attr", NULL
};

char *node_fields_whatsit_pdf_start_thread[] =
    { "attr", "width", "height", "depth",
    "named_id", "thread_id", "thread_attr", NULL
};
char *node_fields_whatsit_pdf_end_thread[] = { "attr", NULL };
char *node_fields_whatsit_pdf_save_pos[] = { "attr", NULL };
char *node_fields_whatsit_late_lua[] = { "attr", "reg", "data", "name", NULL };
char *node_fields_whatsit_close_lua[] = { "attr", "reg", NULL };
char *node_fields_whatsit_pdf_colorstack[] =
    { "attr", "stack", "cmd", "data", NULL };
char *node_fields_whatsit_pdf_setmatrix[] = { "attr", "data", NULL };
char *node_fields_whatsit_pdf_save[] = { "attr", NULL };
char *node_fields_whatsit_pdf_restore[] = { "attr", NULL };
char *node_fields_whatsit_cancel_boundary[] = { "attr", NULL };
char *node_fields_whatsit_user_defined[] =
    { "attr", "user_id", "type", "value", NULL };

node_info node_data[] = {
    {hlist_node, box_node_size, node_fields_list, "hlist"},
    {vlist_node, box_node_size, node_fields_list, "vlist"},
    {rule_node, rule_node_size, node_fields_rule, "rule"},
    {ins_node, ins_node_size, node_fields_insert, "ins"},
    {mark_node, mark_node_size, node_fields_mark, "mark"},
    {adjust_node, adjust_node_size, node_fields_adjust, "adjust"},
    {fake_node, fake_node_size, NULL, fake_node_name},  /* don't touch this! */
    {disc_node, disc_node_size, node_fields_disc, "disc"},
    {whatsit_node, -1, NULL, "whatsit"},
    {math_node, math_node_size, node_fields_math, "math"},
    {glue_node, glue_node_size, node_fields_glue, "glue"},
    {kern_node, kern_node_size, node_fields_kern, "kern"},
    {penalty_node, penalty_node_size, node_fields_penalty, "penalty"},
    {unset_node, box_node_size, node_fields_unset, "unset"},
    {style_node, style_node_size, node_fields_style, "style"},
    {choice_node, style_node_size, node_fields_choice, "choice"},
    {simple_noad, noad_size, node_fields_ord, "noad"},
    {old_op_noad, noad_size, node_fields_op, "op"},
    {old_bin_noad, noad_size, node_fields_bin, "bin"},
    {old_rel_noad, noad_size, node_fields_rel, "rel"},
    {old_open_noad, noad_size, node_fields_open, "open"},
    {old_close_noad, noad_size, node_fields_close, "close"},
    {old_punct_noad, noad_size, node_fields_punct, "punct"},
    {old_inner_noad, noad_size, node_fields_inner, "inner"},
    {radical_noad, radical_noad_size, node_fields_radical, "radical"},
    {fraction_noad, fraction_noad_size, node_fields_fraction, "fraction"},
    {old_under_noad, noad_size, node_fields_under, "under"},
    {old_over_noad, noad_size, node_fields_over, "over"},
    {accent_noad, accent_noad_size, node_fields_accent, "accent"},
    {old_vcenter_noad, noad_size, node_fields_vcenter, "vcenter"},
    {fence_noad, fence_noad_size, node_fields_fence, "fence"},
    {math_char_node, math_kernel_node_size, node_fields_math_char, "math_char"},
    {sub_box_node, math_kernel_node_size, node_fields_sub_box, "sub_box"},
    {sub_mlist_node, math_kernel_node_size, node_fields_sub_mlist, "sub_mlist"},
    {math_text_char_node, math_kernel_node_size, node_fields_math_text_char,
     "math_text_char"},
    {delim_node, math_shield_node_size, node_fields_delim, "delim"},
    {margin_kern_node, margin_kern_node_size, node_fields_margin_kern,
     "margin_kern"},
    {glyph_node, glyph_node_size, node_fields_glyph, "glyph"},
    {align_record_node, box_node_size, NULL, "align_record"},
    {pseudo_file_node, pseudo_file_node_size, NULL, "pseudo_file"},
    {pseudo_line_node, variable_node_size, NULL, "pseudo_line"},
    {inserting_node, page_ins_node_size, node_fields_inserting, "page_insert"},
    {split_up_node, page_ins_node_size, node_fields_splitup, "split_insert"},
    {expr_node, expr_node_size, NULL, "expr_stack"},
    {nesting_node, nesting_node_size, NULL, "nested_list"},
    {span_node, span_node_size, NULL, "span"},
    {attribute_node, attribute_node_size, node_fields_attribute, "attribute"},
    {glue_spec_node, glue_spec_size, node_fields_glue_spec, "glue_spec"},
    {attribute_list_node, attribute_node_size, node_fields_attribute_list,
     "attribute_list"},
    {action_node, pdf_action_size, node_fields_action, "action"},
    {temp_node, temp_node_size, NULL, "temp"},
    {align_stack_node, align_stack_node_size, NULL, "align_stack"},
    {movement_node, movement_node_size, NULL, "movement_stack"},
    {if_node, if_node_size, NULL, "if_stack"},
    {unhyphenated_node, active_node_size, NULL, "unhyphenated"},
    {hyphenated_node, active_node_size, NULL, "hyphenated"},
    {delta_node, delta_node_size, NULL, "delta"},
    {passive_node, passive_node_size, NULL, "passive"},
    {shape_node, variable_node_size, NULL, "shape"},
    {-1, -1, NULL, NULL}
};

#define last_normal_node shape_node

node_info whatsit_node_data[] = {
    {open_node, open_node_size, node_fields_whatsit_open, "open"},
    {write_node, write_node_size, node_fields_whatsit_write, "write"},
    {close_node, close_node_size, node_fields_whatsit_close, "close"},
    {special_node, special_node_size, node_fields_whatsit_special, "special"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {local_par_node, local_par_size, node_fields_whatsit_local_par,
     "local_par"},
    {dir_node, dir_node_size, node_fields_whatsit_dir, "dir"},
    {pdf_literal_node, write_node_size, node_fields_whatsit_pdf_literal,
     "pdf_literal"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {pdf_refobj_node, pdf_refobj_node_size, node_fields_whatsit_pdf_refobj,
     "pdf_refobj"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {pdf_refxform_node, pdf_refxform_node_size,
     node_fields_whatsit_pdf_refxform, "pdf_refxform"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {pdf_refximage_node, pdf_refximage_node_size,
     node_fields_whatsit_pdf_refximage, "pdf_refximage"},
    {pdf_annot_node, pdf_annot_node_size, node_fields_whatsit_pdf_annot,
     "pdf_annot"},
    {pdf_start_link_node, pdf_annot_node_size,
     node_fields_whatsit_pdf_start_link, "pdf_start_link"},
    {pdf_end_link_node, pdf_end_link_node_size,
     node_fields_whatsit_pdf_end_link, "pdf_end_link"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {pdf_dest_node, pdf_dest_node_size, node_fields_whatsit_pdf_dest,
     "pdf_dest"},
    {pdf_thread_node, pdf_thread_node_size, node_fields_whatsit_pdf_thread,
     "pdf_thread"},
    {pdf_start_thread_node, pdf_thread_node_size,
     node_fields_whatsit_pdf_start_thread, "pdf_start_thread"},
    {pdf_end_thread_node, pdf_end_thread_node_size,
     node_fields_whatsit_pdf_end_thread, "pdf_end_thread"},
    {pdf_save_pos_node, pdf_save_pos_node_size,
     node_fields_whatsit_pdf_save_pos, "pdf_save_pos"},
    {pdf_thread_data_node, pdf_thread_node_size, NULL, "pdf_thread_data"},
    {pdf_link_data_node, pdf_annot_node_size, NULL, "pdf_link_data"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {late_lua_node, late_lua_node_size, node_fields_whatsit_late_lua,
     "late_lua"},
    {close_lua_node, write_node_size, node_fields_whatsit_close_lua,
     "close_lua"},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {fake_node, fake_node_size, NULL, fake_node_name},
    {pdf_colorstack_node, pdf_colorstack_node_size,
     node_fields_whatsit_pdf_colorstack, "pdf_colorstack"},
    {pdf_setmatrix_node, pdf_setmatrix_node_size,
     node_fields_whatsit_pdf_setmatrix, "pdf_setmatrix"},
    {pdf_save_node, pdf_save_node_size, node_fields_whatsit_pdf_save,
     "pdf_save"},
    {pdf_restore_node, pdf_restore_node_size, node_fields_whatsit_pdf_restore,
     "pdf_restore"},
    {cancel_boundary_node, cancel_boundary_size,
     node_fields_whatsit_cancel_boundary, "cancel_boundary"},
    {user_defined_node, user_defined_node_size,
     node_fields_whatsit_user_defined, "user_defined"},
    {-1, -1, NULL, NULL}
};

#define last_whatsit_node user_defined_node

halfword new_node(int i, int j)
{
    register int s;
    register halfword n;
    s = get_node_size(i, j);
    n = get_node(s);
    /* it should be possible to do this memset at free_node()  */
    /* type() and subtype() will be set below, and vlink() is 
       set to null by get_node(), so we can do we clearing one 
       word less than |s| */
    (void) memset((void *) (varmem + n + 1), 0,
                  (sizeof(memory_word) * (s - 1)));
    switch (i) {
    case glyph_node:
        init_lang_data(n);
        break;
    case hlist_node:
    case vlist_node:
        box_dir(n) = -1;
        break;
    case whatsit_node:
        if (j == open_node) {
            open_name(n) = get_nullstr();
            open_area(n) = open_name(n);
            open_ext(n) = open_name(n);
        }
        break;
    case disc_node:
        pre_break(n) = pre_break_head(n);
        type(pre_break(n)) = nesting_node;
        subtype(pre_break(n)) = pre_break_head(0);
        post_break(n) = post_break_head(n);
        type(post_break(n)) = nesting_node;
        subtype(post_break(n)) = post_break_head(0);
        no_break(n) = no_break_head(n);
        type(no_break(n)) = nesting_node;
        subtype(no_break(n)) = no_break_head(0);
        break;
    case rule_node:
        depth(n) = null_flag;
        height(n) = null_flag;
        rule_dir(n) = -1;
        /* fall through */
    case unset_node:
        width(n) = null_flag;
        break;
    case pseudo_line_node:
    case shape_node:
        /* this is a trick that makes pseudo_files slightly slower,
         * but the overall allocation faster then an explicit test
         * at the top of new_node().
         */
        free_node(n, variable_node_size);
        n = slow_get_node(j);
        (void) memset((void *) (varmem + n + 1), 0,
                      (sizeof(memory_word) * (j - 1)));
        break;
    default:
        break;
    }
    /* handle synctex extension */
    switch (i) {
    case math_node:
        synctex_tag_math(n) = cur_input.synctex_tag_field;
        synctex_line_math(n) = line;
        break;
    case glue_node:
        synctex_tag_glue(n) = cur_input.synctex_tag_field;
        synctex_line_glue(n) = line;
        break;
    case kern_node:
        /* synctex ignores implicit kerns */
        if (j != 0) {
            synctex_tag_kern(n) = cur_input.synctex_tag_field;
            synctex_line_kern(n) = line;
        }
        break;
    case hlist_node:
    case vlist_node:
    case unset_node:
        synctex_tag_box(n) = cur_input.synctex_tag_field;
        synctex_line_box(n) = line;
        break;
    case rule_node:
        synctex_tag_rule(n) = cur_input.synctex_tag_field;
        synctex_line_rule(n) = line;
        break;
    }
    /* take care of attributes */
    if (nodetype_has_attributes(i)) {
        build_attribute_list(n);
    }
    type(n) = i;
    subtype(n) = j;
#ifdef DEBUG
    fprintf(stderr, "Alloc-ing %s node %d\n",
            get_node_name(type(n), subtype(n)), (int) n);
#endif
    return n;
}

halfword raw_glyph_node(void)
{
    register halfword n;
    n = get_node(glyph_node_size);
    (void) memset((void *) (varmem + n + 1), 0,
                  (sizeof(memory_word) * (glyph_node_size - 1)));
    type(n) = glyph_node;
    subtype(n) = 0;
    return n;
}

halfword new_glyph_node(void)
{
    register halfword n;
    n = get_node(glyph_node_size);
    (void) memset((void *) (varmem + n + 1), 0,
                  (sizeof(memory_word) * (glyph_node_size - 1)));
    type(n) = glyph_node;
    subtype(n) = 0;
    build_attribute_list(n);
    return n;
}


/* makes a duplicate of the node list that starts at |p| and returns a
   pointer to the new list */

halfword copy_node_list(halfword p)
{
    halfword q = null;          /* previous position in new list */
    halfword h = null;          /* head of the list */
    copy_error_seen = 0;
    while (p != null) {
        register halfword s = copy_node(p);
        if (h == null) {
            h = s;
        } else {
            couple_nodes(q, s);
        }
        q = s;
        p = vlink(p);
    }
    return h;
}

 /* make a dupe of a single node */
halfword copy_node(const halfword p)
{
    halfword r;                 /* current node being fabricated for new list */
    register halfword s;        /* a helper variable for copying into variable mem  */
    register int i;
    if (copy_error(p)) {
        r = new_node(temp_node, 0);
        return r;
    }
    i = get_node_size(type(p), subtype(p));
    r = get_node(i);

    (void) memcpy((void *) (varmem + r), (void *) (varmem + p),
                  (sizeof(memory_word) * i));

    /* handle synctex extension */
    switch (type(p)) {
    case math_node:
        synctex_tag_math(r) = cur_input.synctex_tag_field;
        synctex_line_math(r) = line;
        break;
    case kern_node:
        synctex_tag_kern(r) = cur_input.synctex_tag_field;
        synctex_line_kern(r) = line;
        break;
    }

    if (nodetype_has_attributes(type(p))) {
        add_node_attr_ref(node_attr(p));
        alink(r) = null;        /* needs checking */
    }
    vlink(r) = null;


    switch (type(p)) {
    case glyph_node:
        s = copy_node_list(lig_ptr(p));
        lig_ptr(r) = s;
        break;
    case glue_node:
        add_glue_ref(glue_ptr(p));
        s = copy_node_list(leader_ptr(p));
        leader_ptr(r) = s;
        break;
    case hlist_node:
    case vlist_node:
    case unset_node:
        s = copy_node_list(list_ptr(p));
        list_ptr(r) = s;
        break;
    case ins_node:
        add_glue_ref(split_top_ptr(p));
        s = copy_node_list(ins_ptr(p));
        ins_ptr(r) = s;
        break;
    case margin_kern_node:
        s = copy_node(margin_char(p));
        margin_char(r) = s;
        break;
    case disc_node:
        pre_break(r) = pre_break_head(r);
        if (vlink_pre_break(p) != null) {
            s = copy_node_list(vlink_pre_break(p));
            alink(s) = pre_break(r);
            tlink_pre_break(r) = tail_of_list(s);
            vlink_pre_break(r) = s;
        } else {
            assert(tlink(pre_break(r)) == null);
        }
        post_break(r) = post_break_head(r);
        if (vlink_post_break(p) != null) {
            s = copy_node_list(vlink_post_break(p));
            alink(s) = post_break(r);
            tlink_post_break(r) = tail_of_list(s);
            vlink_post_break(r) = s;
        } else {
            assert(tlink_post_break(r) == null);
        }
        no_break(r) = no_break_head(r);
        if (vlink(no_break(p)) != null) {
            s = copy_node_list(vlink_no_break(p));
            alink(s) = no_break(r);
            tlink_no_break(r) = tail_of_list(s);
            vlink_no_break(r) = s;
        } else {
            assert(tlink_no_break(r) == null);
        }
        break;
    case mark_node:
        add_token_ref(mark_ptr(p));
        break;
    case adjust_node:
        s = copy_node_list(adjust_ptr(p));
        adjust_ptr(r) = s;
        break;

    case choice_node:
        s = copy_node_list(display_mlist(p));
        display_mlist(r) = s;
        s = copy_node_list(text_mlist(p));
        text_mlist(r) = s;
        s = copy_node_list(script_mlist(p));
        script_mlist(r) = s;
        s = copy_node_list(script_script_mlist(p));
        script_script_mlist(r) = s;
        break;
    case simple_noad:
    case radical_noad:
    case accent_noad:
        s = copy_node_list(nucleus(p));
        nucleus(r) = s;
        s = copy_node_list(subscr(p));
        subscr(r) = s;
        s = copy_node_list(supscr(p));
        supscr(r) = s;
        if (type(p) == accent_noad) {
            s = copy_node_list(accent_chr(p));
            accent_chr(r) = s;
            s = copy_node_list(bot_accent_chr(p));
            bot_accent_chr(r) = s;
        } else if (type(p) == radical_noad) {
            s = copy_node(left_delimiter(p));
            left_delimiter(r) = s;
            s = copy_node_list(degree(p));
            degree(r) = s;
        }
        break;
    case fence_noad:
        s = copy_node(delimiter(p));
        delimiter(r) = s;
        break;
        /* 
           case style_node:
           case delim_node:
           case math_char_node:
           case math_text_char_node:
           break;
         */
    case sub_box_node:
    case sub_mlist_node:
        s = copy_node_list(math_list(p));
        math_list(r) = s;
        break;
    case fraction_noad:
        s = copy_node_list(numerator(p));
        numerator(r) = s;
        s = copy_node_list(denominator(p));
        denominator(r) = s;
        s = copy_node(left_delimiter(p));
        left_delimiter(r) = s;
        s = copy_node(right_delimiter(p));
        right_delimiter(r) = s;
        break;
    case glue_spec_node:
        glue_ref_count(r) = null;
        break;
    case whatsit_node:
        switch (subtype(p)) {
        case dir_node:
        case local_par_node:
            break;
        case write_node:
        case special_node:
            add_token_ref(write_tokens(p));
            break;
        case pdf_literal_node:
            copy_pdf_literal(r, p);
            break;
        case pdf_colorstack_node:
            if (pdf_colorstack_cmd(p) <= colorstack_data)
                add_token_ref(pdf_colorstack_data(p));
            break;
        case pdf_setmatrix_node:
            add_token_ref(pdf_setmatrix_data(p));
            break;
        case late_lua_node:
            if (late_lua_name(p) > 0)
                add_token_ref(late_lua_name(p));
            add_token_ref(late_lua_data(p));
            break;
        case pdf_annot_node:
            add_token_ref(pdf_annot_data(p));
            break;
        case pdf_start_link_node:
            if (pdf_link_attr(r) != null)
                add_token_ref(pdf_link_attr(r));
            add_action_ref(pdf_link_action(r));
            break;
        case pdf_dest_node:
            if (pdf_dest_named_id(p) > 0)
                add_token_ref(pdf_dest_id(p));
            break;
        case pdf_thread_node:
        case pdf_start_thread_node:
            if (pdf_thread_named_id(p) > 0)
                add_token_ref(pdf_thread_id(p));
            if (pdf_thread_attr(p) != null)
                add_token_ref(pdf_thread_attr(p));
            break;
        case user_defined_node:
            switch (user_node_type(p)) {
            case 'a':
                add_node_attr_ref(user_node_value(p));
                break;
            case 't':
                add_token_ref(user_node_value(p));
                break;
            case 's':
                /* |add_string_ref(user_node_value(p));| *//* if this was mpost .. */
                break;
            case 'n':
                s = copy_node_list(user_node_value(p));
                user_node_value(r) = s;
                break;
            }
            break;
        default:
            break;
        }
        break;
    }
#ifdef DEBUG
    fprintf(stderr, "Alloc-ing %s node %d (copy of %d)\n",
            get_node_name(type(r), subtype(r)), (int) r, (int) p);
#endif
    return r;
}

int valid_node(halfword p)
{
    if (p > my_prealloc) {
        if (p < var_mem_max) {
#ifndef NDEBUG
            if (varmem_sizes[p] > 0)
#endif
                return 1;
        }
    } else {
        return 1;
    }
    return 0;
}

static void do_free_error(halfword p)
{
    halfword r;
    char errstr[255] = { 0 };
    char *errhlp[] = {
        "When I tried to free the node mentioned in the error message, it turned",
        "out it was not (or no longer) actually in use.",
        "Errors such as these are often caused by Lua node list alteration,",
        "but could also point to a bug in the executable. It should be safe to continue.",
        NULL
    };

    check_node_mem();
    if (free_error_seen)
        return;

    r = null;
    free_error_seen = 1;
    if (type(p) == glyph_node) {
        snprintf(errstr, 255,
                 "Attempt to double-free glyph (%c) node %d, ignored",
                 (int) character(p), (int) p);
    } else {
        snprintf(errstr, 255, "Attempt to double-free %s node %d, ignored",
                 get_node_name(type(p), subtype(p)), (int) p);
    }
    tex_error(errstr, errhlp);
#ifndef NDEBUG
    for (r = my_prealloc + 1; r < var_mem_max; r++) {
        if (vlink(r) == p) {
            halfword s = r;
            while (s > my_prealloc && varmem_sizes[s] == 0)
                s--;
            if (s != null
                && s != my_prealloc
                && s != var_mem_max
                && (r - s) < get_node_size(type(s), subtype(s))
                && alink(s) != p) {

                if (type(s) == disc_node) {
                    fprintf(stdout,
                            "  pointed to from %s node %d (vlink %d, alink %d): ",
                            get_node_name(type(s), subtype(s)), (int) s,
                            (int) vlink(s), (int) alink(s));
                    fprintf(stdout, "pre_break(%d,%d,%d), ",
                            (int) vlink_pre_break(s), (int) tlink(pre_break(s)),
                            (int) alink(pre_break(s)));
                    fprintf(stdout, "post_break(%d,%d,%d), ",
                            (int) vlink_post_break(s),
                            (int) tlink(post_break(s)),
                            (int) alink(post_break(s)));
                    fprintf(stdout, "no_break(%d,%d,%d)",
                            (int) vlink_no_break(s), (int) tlink(no_break(s)),
                            (int) alink(no_break(s)));
                    fprintf(stdout, "\n");
                } else {
                    if (vlink(s) == p
                        || (type(s) == glyph_node && lig_ptr(s) == p)
                        || (type(s) == vlist_node && list_ptr(s) == p)
                        || (type(s) == hlist_node && list_ptr(s) == p)
                        || (type(s) == unset_node && list_ptr(s) == p)
                        || (type(s) == ins_node && ins_ptr(s) == p)
                        ) {
                        fprintf(stdout,
                                "  pointed to from %s node %d (vlink %d, alink %d): ",
                                get_node_name(type(s), subtype(s)), (int) s,
                                (int) vlink(s), (int) alink(s));
                        if (type(s) == glyph_node) {
                            fprintf(stdout, "lig_ptr(%d)", (int) lig_ptr(s));
                        } else if (type(s) == vlist_node
                                   || type(s) == hlist_node) {
                            fprintf(stdout, "list_ptr(%d)", (int) list_ptr(s));
                        }
                        fprintf(stdout, "\n");
                    } else {
                        if ((type(s) != penalty_node)
                            && (type(s) != math_node)
                            && (type(s) != kern_node)
                            ) {
                            fprintf(stdout, "  pointed to from %s node %d\n",
                                    get_node_name(type(s), subtype(s)),
                                    (int) s);
                        }
                    }
                }
            }
        }
    }
#endif
}

int free_error(halfword p)
{
    assert(p > my_prealloc);
    assert(p < var_mem_max);
#ifndef NDEBUG
    if (varmem_sizes[p] == 0) {
        do_free_error(p);
        return 1;               /* double free */
    }
#endif
    return 0;
}


static void do_copy_error(halfword p)
{
    char errstr[255] = { 0 };
    char *errhlp[] = {
        "When I tried to copy the node mentioned in the error message, it turned",
        "out it was not (or no longer) actually in use.",
        "Errors such as these are often caused by Lua node list alteration,",
        "but could also point to a bug in the executable. It should be safe to continue.",
        NULL
    };

    if (copy_error_seen)
        return;

    copy_error_seen = 1;
    if (type(p) == glyph_node) {
        snprintf(errstr, 255,
                 "Attempt to copy free glyph (%c) node %d, ignored",
                 (int) character(p), (int) p);
    } else {
        snprintf(errstr, 255, "Attempt to copy free %s node %d, ignored",
                 get_node_name(type(p), subtype(p)), (int) p);
    }
    tex_error(errstr, errhlp);
}


int copy_error(halfword p)
{
    assert(p >= 0);
    assert(p < var_mem_max);
#ifndef NDEBUG
    if (p > my_prealloc && varmem_sizes[p] == 0) {
        do_copy_error(p);
        return 1;               /* copy free node */
    }
#endif
    return 0;
}



void flush_node(halfword p)
{

    if (p == null)              /* legal, but no-op */
        return;

#ifdef DEBUG
    fprintf(stderr, "Free-ing %s node %d\n", get_node_name(type(p), subtype(p)),
            (int) p);
#endif
    if (free_error(p))
        return;

    switch (type(p)) {
    case glyph_node:
        flush_node_list(lig_ptr(p));
        break;
    case glue_node:
        delete_glue_ref(glue_ptr(p));
        flush_node_list(leader_ptr(p));
        break;

    case attribute_node:
    case attribute_list_node:
    case temp_node:
    case glue_spec_node:
    case rule_node:
    case kern_node:
    case math_node:
    case penalty_node:
        break;

    case hlist_node:
    case vlist_node:
    case unset_node:
        flush_node_list(list_ptr(p));
        break;
    case whatsit_node:
        switch (subtype(p)) {

        case dir_node:
            break;
        case open_node:
        case write_node:
        case close_node:
        case pdf_save_node:
        case pdf_restore_node:
        case cancel_boundary_node:
        case close_lua_node:
        case pdf_refobj_node:
        case pdf_refxform_node:
        case pdf_refximage_node:
        case pdf_end_link_node:
        case pdf_end_thread_node:
        case pdf_save_pos_node:
        case local_par_node:
            break;

        case special_node:
            delete_token_ref(write_tokens(p));
            break;
        case pdf_literal_node:
            free_pdf_literal(p);
            break;
        case pdf_colorstack_node:
            if (pdf_colorstack_cmd(p) <= colorstack_data)
                delete_token_ref(pdf_colorstack_data(p));
            break;
        case pdf_setmatrix_node:
            delete_token_ref(pdf_setmatrix_data(p));
            break;
        case late_lua_node:
            if (late_lua_name(p) > 0)
                delete_token_ref(late_lua_name(p));
            delete_token_ref(late_lua_data(p));
            break;
        case pdf_annot_node:
            delete_token_ref(pdf_annot_data(p));
            break;

        case pdf_link_data_node:
            break;

        case pdf_start_link_node:
            if (pdf_link_attr(p) != null)
                delete_token_ref(pdf_link_attr(p));
            delete_action_ref(pdf_link_action(p));
            break;
        case pdf_dest_node:
            if (pdf_dest_named_id(p) > 0)
                delete_token_ref(pdf_dest_id(p));
            break;

        case pdf_thread_data_node:
            break;

        case pdf_thread_node:
        case pdf_start_thread_node:
            if (pdf_thread_named_id(p) > 0)
                delete_token_ref(pdf_thread_id(p));
            if (pdf_thread_attr(p) != null)
                delete_token_ref(pdf_thread_attr(p));
            break;
        case user_defined_node:
            switch (user_node_type(p)) {
            case 'a':
                delete_attribute_ref(user_node_value(p));
                break;
            case 't':
                delete_token_ref(user_node_value(p));
                break;
            case 'n':
                flush_node_list(user_node_value(p));
                break;
            case 's':
                /* |delete_string_ref(user_node_value(p));| *//* if this was mpost .. */
                break;
            case 'd':
                break;
            default:
                {
                    char *hlp[] = {
                        "The type of the value in a user defined whatsit node should be one",
                        "of 'a' (attribute list), 'd' (number), 'n' (node list), 's' (string),",
                        "or 't' (tokenlist). Yours has an unknown type, and therefore I don't",
                        "know how to free the node's value. A memory leak may result.",
                        NULL
                    };
                    tex_error("Unidentified user defined whatsit", hlp);
                }
                break;
            }
            break;

        default:
            confusion("ext3");
            return;

        }
        break;
    case ins_node:
        flush_node_list(ins_ptr(p));
        delete_glue_ref(split_top_ptr(p));
        break;
    case margin_kern_node:
        flush_node(margin_char(p));
        break;
    case mark_node:
        delete_token_ref(mark_ptr(p));
        break;
    case disc_node:
        flush_node_list(vlink(pre_break(p)));
        flush_node_list(vlink(post_break(p)));
        flush_node_list(vlink(no_break(p)));
        break;
    case adjust_node:
        flush_node_list(adjust_ptr(p));
        break;
    case style_node:           /* nothing to do */
        break;
    case choice_node:
        flush_node_list(display_mlist(p));
        flush_node_list(text_mlist(p));
        flush_node_list(script_mlist(p));
        flush_node_list(script_script_mlist(p));
        break;
    case simple_noad:
    case radical_noad:
    case accent_noad:
        flush_node_list(nucleus(p));
        flush_node_list(subscr(p));
        flush_node_list(supscr(p));
        if (type(p) == accent_noad) {
            flush_node_list(accent_chr(p));
            flush_node_list(bot_accent_chr(p));
        } else if (type(p) == radical_noad) {
            flush_node(left_delimiter(p));
            flush_node_list(degree(p));
        }
        break;
    case fence_noad:
        flush_node(delimiter(p));
        break;
    case delim_node:           /* nothing to do */
    case math_char_node:
    case math_text_char_node:
        break;
    case sub_box_node:
    case sub_mlist_node:
        flush_node_list(math_list(p));
        break;
    case fraction_noad:
        flush_node_list(numerator(p));
        flush_node_list(denominator(p));
        flush_node(left_delimiter(p));
        flush_node(right_delimiter(p));
        break;
    case pseudo_file_node:
        flush_node_list(pseudo_lines(p));
        break;
    case pseudo_line_node:
    case shape_node:
        free_node(p, subtype(p));
        return;
        break;
    case align_stack_node:
    case span_node:
    case movement_node:
    case if_node:
    case nesting_node:
    case unhyphenated_node:
    case hyphenated_node:
    case delta_node:
    case passive_node:
    case action_node:
    case inserting_node:
    case split_up_node:
    case expr_node:
        break;
    default:
        fprintf(stdout, "flush_node: type is %d\n", type(p));
        return;

    }
    if (nodetype_has_attributes(type(p)))
        delete_attribute_ref(node_attr(p));
    free_node(p, get_node_size(type(p), subtype(p)));
    return;
}

void flush_node_list(halfword pp)
{                               /* erase list of nodes starting at |p| */
    register halfword p = pp;
    free_error_seen = 0;
    if (p == null)              /* legal, but no-op */
        return;
    if (free_error(p))
        return;

    while (p != null) {
        register halfword q = vlink(p);
        flush_node(p);
        p = q;
    }
}

static int test_count = 1;

#define dorangetest(a,b,c)  do {                                        \
    if (!(b>=0 && b<c)) {                                               \
      fprintf(stdout,"For node p:=%d, 0<=%d<%d (l.%d,r.%d)\n",          \
              (int)a, (int)b, (int)c, __LINE__,test_count);             \
      confusion("dorangetest");                                        \
    } } while (0)

#define dotest(a,b,c) do {                                              \
    if (b!=c) {                                                         \
      fprintf(stdout,"For node p:=%d, %d==%d (l.%d,r.%d)\n",            \
              (int)a, (int)b, (int)c, __LINE__,test_count);             \
      confusion("dotest");                                             \
    } } while (0)

#define check_action_ref(a)     { dorangetest(p,a,var_mem_max); }
#define check_glue_ref(a)       { dorangetest(p,a,var_mem_max); }
#define check_attribute_ref(a)  { dorangetest(p,a,var_mem_max); }
#define check_token_ref(a)      assert(1)

void check_node(halfword p)
{

    switch (type(p)) {
    case glyph_node:
        dorangetest(p, lig_ptr(p), var_mem_max);
        break;
    case glue_node:
        check_glue_ref(glue_ptr(p));
        dorangetest(p, leader_ptr(p), var_mem_max);
        break;
    case hlist_node:
    case vlist_node:
    case unset_node:
    case align_record_node:
        dorangetest(p, list_ptr(p), var_mem_max);
        break;
    case ins_node:
        dorangetest(p, ins_ptr(p), var_mem_max);
        check_glue_ref(split_top_ptr(p));
        break;
    case whatsit_node:
        switch (subtype(p)) {
        case special_node:
            check_token_ref(write_tokens(p));
            break;
        case pdf_literal_node:
            if (pdf_literal_type(p) == normal)
                check_token_ref(pdf_literal_data(p));
            break;
        case pdf_colorstack_node:
            if (pdf_colorstack_cmd(p) <= colorstack_data)
                check_token_ref(pdf_colorstack_data(p));
            break;
        case pdf_setmatrix_node:
            check_token_ref(pdf_setmatrix_data(p));
            break;
        case late_lua_node:
            if (late_lua_name(p) > 0)
                check_token_ref(late_lua_name(p));
            check_token_ref(late_lua_data(p));
            break;
        case pdf_annot_node:
            check_token_ref(pdf_annot_data(p));
            break;
        case pdf_start_link_node:
            if (pdf_link_attr(p) != null)
                check_token_ref(pdf_link_attr(p));
            check_action_ref(pdf_link_action(p));
            break;
        case pdf_dest_node:
            if (pdf_dest_named_id(p) > 0)
                check_token_ref(pdf_dest_id(p));
            break;
        case pdf_thread_node:
        case pdf_start_thread_node:
            if (pdf_thread_named_id(p) > 0)
                check_token_ref(pdf_thread_id(p));
            if (pdf_thread_attr(p) != null)
                check_token_ref(pdf_thread_attr(p));
            break;
        case user_defined_node:
            switch (user_node_type(p)) {
            case 'a':
                check_attribute_ref(user_node_value(p));
                break;
            case 't':
                check_token_ref(user_node_value(p));
                break;
            case 'n':
                dorangetest(p, user_node_value(p), var_mem_max);
                break;
            case 's':
            case 'd':
                break;
            default:
                confusion("extuser");
                break;
            }
            break;
        case dir_node:
        case open_node:
        case write_node:
        case close_node:
        case pdf_save_node:
        case pdf_restore_node:
        case cancel_boundary_node:
        case close_lua_node:
        case pdf_refobj_node:
        case pdf_refxform_node:
        case pdf_refximage_node:
        case pdf_end_link_node:
        case pdf_end_thread_node:
        case pdf_save_pos_node:
        case local_par_node:
            break;
        default:
            confusion("ext3");
        }
        break;
    case margin_kern_node:
        check_node(margin_char(p));
        break;
    case disc_node:
        dorangetest(p, vlink(pre_break(p)), var_mem_max);
        dorangetest(p, vlink(post_break(p)), var_mem_max);
        dorangetest(p, vlink(no_break(p)), var_mem_max);
        break;
    case adjust_node:
        dorangetest(p, adjust_ptr(p), var_mem_max);
        break;
    case pseudo_file_node:
        dorangetest(p, pseudo_lines(p), var_mem_max);
        break;
    case pseudo_line_node:
    case shape_node:
        break;
    case choice_node:
        dorangetest(p, display_mlist(p), var_mem_max);
        dorangetest(p, text_mlist(p), var_mem_max);
        dorangetest(p, script_mlist(p), var_mem_max);
        dorangetest(p, script_script_mlist(p), var_mem_max);
        break;
    case fraction_noad:
        dorangetest(p, numerator(p), var_mem_max);
        dorangetest(p, denominator(p), var_mem_max);
        dorangetest(p, left_delimiter(p), var_mem_max);
        dorangetest(p, right_delimiter(p), var_mem_max);
        break;
    case simple_noad:
        dorangetest(p, nucleus(p), var_mem_max);
        dorangetest(p, subscr(p), var_mem_max);
        dorangetest(p, supscr(p), var_mem_max);
        break;
    case radical_noad:
        dorangetest(p, nucleus(p), var_mem_max);
        dorangetest(p, subscr(p), var_mem_max);
        dorangetest(p, supscr(p), var_mem_max);
        dorangetest(p, degree(p), var_mem_max);
        dorangetest(p, left_delimiter(p), var_mem_max);
        break;
    case accent_noad:
        dorangetest(p, nucleus(p), var_mem_max);
        dorangetest(p, subscr(p), var_mem_max);
        dorangetest(p, supscr(p), var_mem_max);
        dorangetest(p, accent_chr(p), var_mem_max);
        dorangetest(p, bot_accent_chr(p), var_mem_max);
        break;
    case fence_noad:
        dorangetest(p, delimiter(p), var_mem_max);
        break;
    case rule_node:
    case kern_node:
    case math_node:
    case penalty_node:
    case mark_node:
    case style_node:
    case attribute_list_node:
    case attribute_node:
    case glue_spec_node:
    case temp_node:
    case align_stack_node:
    case movement_node:
    case if_node:
    case nesting_node:
    case span_node:
    case unhyphenated_node:
    case hyphenated_node:
    case delta_node:
    case passive_node:
    case expr_node:
        break;
    default:
        fprintf(stdout, "check_node: type is %d\n", type(p));
    }
}

void check_static_node_mem(void)
{
    dotest(zero_glue, width(zero_glue), 0);
    dotest(zero_glue, type(zero_glue), glue_spec_node);
    dotest(zero_glue, vlink(zero_glue), null);
    dotest(zero_glue, stretch(zero_glue), 0);
    dotest(zero_glue, stretch_order(zero_glue), normal);
    dotest(zero_glue, shrink(zero_glue), 0);
    dotest(zero_glue, shrink_order(zero_glue), normal);

    dotest(sfi_glue, width(sfi_glue), 0);
    dotest(sfi_glue, type(sfi_glue), glue_spec_node);
    dotest(sfi_glue, vlink(sfi_glue), null);
    dotest(sfi_glue, stretch(sfi_glue), 0);
    dotest(sfi_glue, stretch_order(sfi_glue), sfi);
    dotest(sfi_glue, shrink(sfi_glue), 0);
    dotest(sfi_glue, shrink_order(sfi_glue), normal);

    dotest(fil_glue, width(fil_glue), 0);
    dotest(fil_glue, type(fil_glue), glue_spec_node);
    dotest(fil_glue, vlink(fil_glue), null);
    dotest(fil_glue, stretch(fil_glue), unity);
    dotest(fil_glue, stretch_order(fil_glue), fil);
    dotest(fil_glue, shrink(fil_glue), 0);
    dotest(fil_glue, shrink_order(fil_glue), normal);

    dotest(fill_glue, width(fill_glue), 0);
    dotest(fill_glue, type(fill_glue), glue_spec_node);
    dotest(fill_glue, vlink(fill_glue), null);
    dotest(fill_glue, stretch(fill_glue), unity);
    dotest(fill_glue, stretch_order(fill_glue), fill);
    dotest(fill_glue, shrink(fill_glue), 0);
    dotest(fill_glue, shrink_order(fill_glue), normal);

    dotest(ss_glue, width(ss_glue), 0);
    dotest(ss_glue, type(ss_glue), glue_spec_node);
    dotest(ss_glue, vlink(ss_glue), null);
    dotest(ss_glue, stretch(ss_glue), unity);
    dotest(ss_glue, stretch_order(ss_glue), fil);
    dotest(ss_glue, shrink(ss_glue), unity);
    dotest(ss_glue, shrink_order(ss_glue), fil);

    dotest(fil_neg_glue, width(fil_neg_glue), 0);
    dotest(fil_neg_glue, type(fil_neg_glue), glue_spec_node);
    dotest(fil_neg_glue, vlink(fil_neg_glue), null);
    dotest(fil_neg_glue, stretch(fil_neg_glue), -unity);
    dotest(fil_neg_glue, stretch_order(fil_neg_glue), fil);
    dotest(fil_neg_glue, shrink(fil_neg_glue), 0);
    dotest(fil_neg_glue, shrink_order(fil_neg_glue), normal);
}

void check_node_mem(void)
{
    int i;
    check_static_node_mem();
#ifndef NDEBUG
    for (i = (my_prealloc + 1); i < var_mem_max; i++) {
        if (varmem_sizes[i] > 0) {
            check_node(i);
        }
    }
#endif
    test_count++;
}

void fix_node_list(halfword head)
{
    halfword p, q;
    if (head == null)
        return;
    p = head;
    q = vlink(p);
    while (q != null) {
        alink(q) = p;
        p = q;
        q = vlink(p);
    }
}

halfword get_node(integer s)
{
    register halfword r;

    /*check_static_node_mem(); */
    assert(s < MAX_CHAIN_SIZE);

    r = free_chain[s];
    if (r != null) {
        free_chain[s] = vlink(r);
#ifndef NDEBUG
        varmem_sizes[r] = s;
#endif
        vlink(r) = null;
        var_used += s;          /* maintain usage statistics */
        return r;
    }
    /* this is the end of the 'inner loop' */
    return slow_get_node(s);
}

void print_free_chain(int c)
{
    halfword p = free_chain[c];
    fprintf(stdout, "\nfree chain[%d] =\n  ", c);
    while (p != null) {
        fprintf(stdout, "%d,", (int) p);
        p = vlink(p);
    }
    fprintf(stdout, "null;\n");
}

void free_node(halfword p, integer s)
{

    if (p <= my_prealloc) {
        fprintf(stdout, "node %d (type %d) should not be freed!\n", (int) p,
                type(p));
        return;
    }
#ifndef NDEBUG
    varmem_sizes[p] = 0;
#endif
    if (s < MAX_CHAIN_SIZE) {
        vlink(p) = free_chain[s];
        free_chain[s] = p;
    } else {
        /* todo ? it is perhaps possible to merge this node with an existing rover */
        node_size(p) = s;
        vlink(p) = rover;
        while (vlink(rover) != vlink(p)) {
            rover = vlink(rover);
        }
        vlink(rover) = p;
    }
    var_used -= s;              /* maintain statistics */
}

void free_node_chain(halfword q, integer s)
{
    register halfword p = q;
    while (vlink(p) != null) {
#ifndef NDEBUG
        varmem_sizes[p] = 0;
#endif
        var_used -= s;
        p = vlink(p);
    }
    var_used -= s;
#ifndef NDEBUG
    varmem_sizes[p] = 0;
#endif
    vlink(p) = free_chain[s];
    free_chain[s] = q;
}


void init_node_mem(halfword t)
{
    my_prealloc = var_mem_stat_max;
    assert(whatsit_node_data[user_defined_node].id == user_defined_node);
    assert(node_data[passive_node].id == passive_node);

    varmem = (memory_word *) realloc((void *) varmem, sizeof(memory_word) * t);
    if (varmem == NULL) {
        overflow("node memory size", var_mem_max);
    }
    memset((void *) (varmem), 0, t * sizeof(memory_word));

#ifndef NDEBUG
    varmem_sizes = (char *) realloc(varmem_sizes, sizeof(char) * t);
    if (varmem_sizes == NULL) {
        overflow("node memory size", var_mem_max);
    }
    memset((void *) varmem_sizes, 0, sizeof(char) * t);
#endif
    var_mem_max = t;
    rover = var_mem_stat_max + 1;
    vlink(rover) = rover;
    node_size(rover) = (t - rover);
    var_used = 0;
    /* initialize static glue specs */
    glue_ref_count(zero_glue) = null + 1;
    width(zero_glue) = 0;
    type(zero_glue) = glue_spec_node;
    vlink(zero_glue) = null;
    stretch(zero_glue) = 0;
    stretch_order(zero_glue) = normal;
    shrink(zero_glue) = 0;
    shrink_order(zero_glue) = normal;
    glue_ref_count(sfi_glue) = null + 1;
    width(sfi_glue) = 0;
    type(sfi_glue) = glue_spec_node;
    vlink(sfi_glue) = null;
    stretch(sfi_glue) = 0;
    stretch_order(sfi_glue) = sfi;
    shrink(sfi_glue) = 0;
    shrink_order(sfi_glue) = normal;
    glue_ref_count(fil_glue) = null + 1;
    width(fil_glue) = 0;
    type(fil_glue) = glue_spec_node;
    vlink(fil_glue) = null;
    stretch(fil_glue) = unity;
    stretch_order(fil_glue) = fil;
    shrink(fil_glue) = 0;
    shrink_order(fil_glue) = normal;
    glue_ref_count(fill_glue) = null + 1;
    width(fill_glue) = 0;
    type(fill_glue) = glue_spec_node;
    vlink(fill_glue) = null;
    stretch(fill_glue) = unity;
    stretch_order(fill_glue) = fill;
    shrink(fill_glue) = 0;
    shrink_order(fill_glue) = normal;
    glue_ref_count(ss_glue) = null + 1;
    width(ss_glue) = 0;
    type(ss_glue) = glue_spec_node;
    vlink(ss_glue) = null;
    stretch(ss_glue) = unity;
    stretch_order(ss_glue) = fil;
    shrink(ss_glue) = unity;
    shrink_order(ss_glue) = fil;
    glue_ref_count(fil_neg_glue) = null + 1;
    width(fil_neg_glue) = 0;
    type(fil_neg_glue) = glue_spec_node;
    vlink(fil_neg_glue) = null;
    stretch(fil_neg_glue) = -unity;
    stretch_order(fil_neg_glue) = fil;
    shrink(fil_neg_glue) = 0;
    shrink_order(fil_neg_glue) = normal;
    /* initialize node list heads */
    vinfo(page_ins_head) = 0;
    vlink(page_ins_head) = null;
    alink(page_ins_head) = null;
    vinfo(contrib_head) = 0;
    vlink(contrib_head) = null;
    alink(contrib_head) = null;
    vinfo(page_head) = 0;
    vlink(page_head) = null;
    alink(page_head) = null;
    vinfo(temp_head) = 0;
    vlink(temp_head) = null;
    alink(temp_head) = null;
    vinfo(hold_head) = 0;
    vlink(hold_head) = null;
    alink(hold_head) = null;
    vinfo(adjust_head) = 0;
    vlink(adjust_head) = null;
    alink(adjust_head) = null;
    vinfo(pre_adjust_head) = 0;
    vlink(pre_adjust_head) = null;
    alink(pre_adjust_head) = null;
    vinfo(active) = 0;
    vlink(active) = null;
    alink(active) = null;
    vinfo(align_head) = 0;
    vlink(align_head) = null;
    alink(align_head) = null;
    vinfo(end_span) = 0;
    vlink(end_span) = null;
    alink(end_span) = null;
    type(begin_point) = glyph_node;
    subtype(begin_point) = 0;
    vlink(begin_point) = null;
    vinfo(begin_point + 1) = null;
    alink(begin_point) = null;
    font(begin_point) = 0;
    character(begin_point) = '.';
    vinfo(begin_point + 3) = 0;
    vlink(begin_point + 3) = 0;
    vinfo(begin_point + 4) = 0;
    vlink(begin_point + 4) = 0;
    type(end_point) = glyph_node;
    subtype(end_point) = 0;
    vlink(end_point) = null;
    vinfo(end_point + 1) = null;
    alink(end_point) = null;
    font(end_point) = 0;
    character(end_point) = '.';
    vinfo(end_point + 3) = 0;
    vlink(end_point + 3) = 0;
    vinfo(end_point + 4) = 0;
    vlink(end_point + 4) = 0;
}

void dump_node_mem(void)
{
    dump_int(var_mem_max);
    dump_int(rover);
    dump_things(varmem[0], var_mem_max);
#ifndef NDEBUG
    dump_things(varmem_sizes[0], var_mem_max);
#endif
    dump_things(free_chain[0], MAX_CHAIN_SIZE);
    dump_int(var_used);
    dump_int(my_prealloc);
}

/* it makes sense to enlarge the varmem array immediately */

void undump_node_mem(void)
{
    integer x;
    undump_int(x);
    undump_int(rover);
    var_mem_max = (x < 100000 ? 100000 : x);
    varmem = xmallocarray(memory_word, var_mem_max);
    /*memset ((void *)varmem,0,x*sizeof(memory_word)); */
    undump_things(varmem[0], x);
#ifndef NDEBUG
    varmem_sizes = xmallocarray(char, var_mem_max);
    memset((void *) varmem_sizes, 0, var_mem_max * sizeof(char));
    undump_things(varmem_sizes[0], x);
#endif
    undump_things(free_chain[0], MAX_CHAIN_SIZE);
    undump_int(var_used);
    undump_int(my_prealloc);
    if (var_mem_max > x) {
        /* todo ? it is perhaps possible to merge the new node with an existing rover */
        vlink(x) = rover;
        node_size(x) = (var_mem_max - x);
        while (vlink(rover) != vlink(x)) {
            rover = vlink(rover);
        }
        vlink(rover) = x;
    }
}

#if 0
void test_rovers(char *s)
{
    int q = rover;
    int r = q;
    fprintf(stdout, "%s: {rover=%d,size=%d,link=%d}", s, r, node_size(r),
            vlink(r));
    while (vlink(r) != q) {
        r = vlink(r);
        fprintf(stdout, ",{rover=%d,size=%d,link=%d}", r, node_size(r),
                vlink(r));
    }
    fprintf(stdout, "\n");
}
#else
#  define test_rovers(a)
#endif

halfword slow_get_node(integer s)
{
    register int t;

  RETRY:
    t = node_size(rover);
    assert(vlink(rover) < var_mem_max);
    assert(vlink(rover) != 0);
    test_rovers("entry");
    if (t > s) {
        register halfword r;
        /* allocating from the bottom helps decrease page faults */
        r = rover;
        rover += s;
        vlink(rover) = vlink(r);
        node_size(rover) = node_size(r) - s;
        if (vlink(rover) != r) {        /* list is longer than one */
            halfword q = r;
            while (vlink(q) != r) {
                q = vlink(q);
            }
            vlink(q) += s;
        } else {
            vlink(rover) += s;
        }
        test_rovers("taken");
        assert(vlink(rover) < var_mem_max);
#ifndef NDEBUG
        varmem_sizes[r] = (s > 127 ? 127 : s);
#endif
        vlink(r) = null;
        var_used += s;          /* maintain usage statistics */
        return r;               /* this is the only exit */
    } else {
        int x;
        /* attempt to keep the free list small */
        if (vlink(rover) != rover) {
            if (t < MAX_CHAIN_SIZE) {
                halfword l = vlink(rover);
                vlink(rover) = free_chain[t];
                free_chain[t] = rover;
                rover = l;
                while (vlink(l) != free_chain[t]) {
                    l = vlink(l);
                }
                vlink(l) = rover;
                test_rovers("outtake");
                goto RETRY;
            } else {
                halfword l = rover;
                while (vlink(rover) != l) {
                    if (node_size(rover) > s) {
                        goto RETRY;
                    }
                    rover = vlink(rover);
                }
            }
        }
        /* if we are still here, it was apparently impossible to get a match */
        x = (var_mem_max >> 2) + s;
        varmem =
            (memory_word *) realloc((void *) varmem,
                                    sizeof(memory_word) * (var_mem_max + x));
        if (varmem == NULL) {
            overflow("node memory size", var_mem_max);
        }
        memset((void *) (varmem + var_mem_max), 0, x * sizeof(memory_word));

#ifndef NDEBUG
        varmem_sizes =
            (char *) realloc(varmem_sizes, sizeof(char) * (var_mem_max + x));
        if (varmem_sizes == NULL) {
            overflow("node memory size", var_mem_max);
        }
        memset((void *) (varmem_sizes + var_mem_max), 0, x * sizeof(char));
#endif

        /* todo ? it is perhaps possible to merge the new memory with an existing rover */
        vlink(var_mem_max) = rover;
        node_size(var_mem_max) = x;
        while (vlink(rover) != vlink(var_mem_max)) {
            rover = vlink(rover);
        }
        vlink(rover) = var_mem_max;
        rover = var_mem_max;
        test_rovers("realloc");
        var_mem_max += x;
        goto RETRY;
    }
}

char *sprint_node_mem_usage(void)
{
    int i, b;

    char *s, *ss;
#ifndef NDEBUG
    char msg[256];
    int node_counts[last_normal_node + last_whatsit_node + 2] = { 0 };

    for (i = (var_mem_max - 1); i > my_prealloc; i--) {
        if (varmem_sizes[i] > 0) {
            if (type(i) > last_normal_node + last_whatsit_node) {
                node_counts[last_normal_node + last_whatsit_node + 1]++;
            } else if (type(i) == whatsit_node) {
                node_counts[(subtype(i) + last_normal_node + 1)]++;
            } else {
                node_counts[type(i)]++;
            }
        }
    }
    s = strdup("");
    b = 0;
    for (i = 0; i < last_normal_node + last_whatsit_node + 2; i++) {
        if (node_counts[i] > 0) {
            int j =
                (i > (last_normal_node + 1) ? (i - last_normal_node - 1) : 0);
            snprintf(msg, 255, "%s%d %s", (b ? ", " : ""), (int) node_counts[i],
                     get_node_name((i > last_normal_node ? whatsit_node : i),
                                   j));
            ss = xmalloc(strlen(s) + strlen(msg) + 1);
            strcpy(ss, s);
            strcat(ss, msg);
            free(s);
            s = ss;
            b = 1;
        }
    }
#else
    s = strdup("");
#endif
    return s;
}

halfword list_node_mem_usage(void)
{
    halfword i, j;
    halfword p = null, q = null;
#ifndef NDEBUG
    char *saved_varmem_sizes = xmallocarray(char, var_mem_max);
    memcpy(saved_varmem_sizes, varmem_sizes, var_mem_max);
    for (i = my_prealloc + 1; i < (var_mem_max - 1); i++) {
        if (saved_varmem_sizes[i] > 0) {
            j = copy_node(i);
            if (p == null) {
                q = j;
            } else {
                vlink(p) = j;
            }
            p = j;
        }
    }
    free(saved_varmem_sizes);
#endif
    return q;
}

void print_node_mem_stats(void)
{
    int i, b;
    halfword j;
    char msg[256];
    char *s;
    integer free_chain_counts[MAX_CHAIN_SIZE] = { 0 };
    snprintf(msg, 255, "node memory in use: %d words out of %d",
             (int) (var_used + my_prealloc), (int) var_mem_max);
    tprint_nl(msg);
    tprint_nl("rapidly available: ");
    b = 0;
    for (i = 1; i < MAX_CHAIN_SIZE; i++) {
        for (j = free_chain[i]; j != null; j = vlink(j))
            free_chain_counts[i]++;
        if (free_chain_counts[i] > 0) {
            snprintf(msg, 255, "%s%d:%d", (b ? ", " : ""), i,
                     (int) free_chain_counts[i]);
            tprint(msg);
            b = 1;
        }
    }
    tprint(" nodes");
    s = sprint_node_mem_usage();
    tprint_nl("current usage: ");
    tprint(s);
    free(s);
    tprint(" nodes");
    print_nlp();                /* newline, if needed */
}

/* this belongs in the web but i couldn't find the correct syntactic place */

halfword new_span_node(halfword n, int s, scaled w)
{
    halfword p = new_node(span_node, 0);
    span_link(p) = n;
    span_span(p) = s;
    width(p) = w;
    return p;
}

halfword string_to_pseudo(integer l, integer pool_ptr, integer nl)
{
    halfword i, r, q = null;
    four_quarters w;
    int sz;
    halfword h = new_node(pseudo_file_node, 0);
    while (l < pool_ptr) {
        int m = l;
        while ((l < pool_ptr) && (str_pool[l] != nl))
            l++;
        sz = (l - m + 7) / 4;
        if (sz == 1)
            sz = 2;
        r = new_node(pseudo_line_node, sz);
        i = r;
        while (--sz > 1) {
            w.b0 = str_pool[m++];
            w.b1 = str_pool[m++];
            w.b2 = str_pool[m++];
            w.b3 = str_pool[m++];
            varmem[++i].qqqq = w;
        }
        w.b0 = (l > m ? str_pool[m++] : ' ');
        w.b1 = (l > m ? str_pool[m++] : ' ');
        w.b2 = (l > m ? str_pool[m++] : ' ');
        w.b3 = (l > m ? str_pool[m] : ' ');
        varmem[++i].qqqq = w;
        if (pseudo_lines(h) == null) {
            pseudo_lines(h) = r;
            q = r;
        } else {
            couple_nodes(q, r);
        }
        q = vlink(q);
        if (str_pool[l] == nl)
            l++;
    }
    return h;
}

/* attribute stuff */

static halfword new_attribute_node(unsigned int i, int v)
{
    register halfword r = get_node(attribute_node_size);
    type(r) = attribute_node;
    attribute_id(r) = i;
    attribute_value(r) = v;
    return r;
}

halfword copy_attribute_list(halfword n)
{
    halfword q = get_node(attribute_node_size);
    register halfword p = q;
    type(p) = attribute_list_node;
    attr_list_ref(p) = 0;
    n = vlink(n);
    while (n != null) {
        register halfword r = get_node(attribute_node_size);
        /* the link will be fixed automatically in the next loop */
        (void) memcpy((void *) (varmem + r), (void *) (varmem + n),
                      (sizeof(memory_word) * attribute_node_size));
        vlink(p) = r;
        p = r;
        n = vlink(n);
    }
    return q;
}

void update_attribute_cache(void)
{
    halfword p;
    register int i;
    attr_list_cache = get_node(attribute_node_size);
    type(attr_list_cache) = attribute_list_node;
    attr_list_ref(attr_list_cache) = 0;
    p = attr_list_cache;
    for (i = 0; i <= max_used_attr; i++) {
        register int v = attribute(i);
        if (v > UNUSED_ATTRIBUTE) {
            register halfword r = new_attribute_node(i, v);
            vlink(p) = r;
            p = r;
        }
    }
    if (vlink(attr_list_cache) == null) {
        free_node(attr_list_cache, attribute_node_size);
        attr_list_cache = null;
    }
    return;
}

void build_attribute_list(halfword b)
{
    if (max_used_attr >= 0) {
        if (attr_list_cache == cache_disabled) {
            update_attribute_cache();
            if (attr_list_cache == null)
                return;
        }
        attr_list_ref(attr_list_cache)++;
        node_attr(b) = attr_list_cache;
#ifdef DEBUG
        fprintf(stderr, "Added attrlist (%d) to node %d (count=%d)\n",
                node_attr(b), b, attr_list_ref(attr_list_cache));
#endif
    }
}

void delete_attribute_ref(halfword b)
{
    if (b != null) {
        assert(type(b) == attribute_list_node);
        attr_list_ref(b)--;
#ifdef DEBUG
        fprintf(stderr, "Removed attrlistref (%d) (count=%d)\n", b,
                attr_list_ref(b));
#endif
        if (attr_list_ref(b) == 0) {
            if (b == attr_list_cache)
                attr_list_cache = cache_disabled;
            free_node_chain(b, attribute_node_size);
        }
        /* maintain sanity */
        if (attr_list_ref(b) < 0)
            attr_list_ref(b) = 0;
    }
}

/* |p| is an attr list head, or zero */

halfword do_set_attribute(halfword p, int i, int val)
{
    register halfword q;
    register int j = 0;
    if (p == null) {            /* add a new head & node */
        q = get_node(attribute_node_size);
        type(q) = attribute_list_node;
        attr_list_ref(q) = 1;
        p = new_attribute_node(i, val);
        vlink(q) = p;
        return q;
    }
    q = p;
    assert(vlink(p) != null);
    while (vlink(p) != null) {
        int t = attribute_id(vlink(p));
        if (t == i && attribute_value(vlink(p)) == val)
            return q;           /* no need to do anything */
        if (t >= i)
            break;
        j++;
        p = vlink(p);
    }

    p = q;
    while (j-- > 0)
        p = vlink(p);
    if (attribute_id(vlink(p)) == i) {
        attribute_value(vlink(p)) = val;
    } else {                    /* add a new node */
        halfword r = new_attribute_node(i, val);
        vlink(r) = vlink(p);
        vlink(p) = r;
    }
    return q;
}

void set_attribute(halfword n, int i, int val)
{
    register halfword p;
    register int j = 0;
    if (!nodetype_has_attributes(type(n)))
        return;
    p = node_attr(n);
    if (p == null) {            /* add a new head & node */
        p = get_node(attribute_node_size);
        type(p) = attribute_list_node;
        attr_list_ref(p) = 1;
        node_attr(n) = p;
        p = new_attribute_node(i, val);
        vlink(node_attr(n)) = p;
        return;
    }
    assert(vlink(p) != null);
    while (vlink(p) != null) {
        int t = attribute_id(vlink(p));
        if (t == i && attribute_value(vlink(p)) == val)
            return;
        if (t >= i)
            break;
        j++;
        p = vlink(p);
    }
    p = node_attr(n);
    if (attr_list_ref(p) != 1) {
        if (attr_list_ref(p) > 1) {
            p = copy_attribute_list(p);
            delete_attribute_ref(node_attr(n));
            node_attr(n) = p;
        } else {
            fprintf(stdout,
                    "Node %d has an attribute list that is free already\n",
                    (int) n);
        }
        attr_list_ref(p) = 1;
    }
    while (j-- > 0)
        p = vlink(p);

    if (attribute_id(vlink(p)) == i) {
        attribute_value(vlink(p)) = val;
    } else {                    /* add a new node */
        halfword r = new_attribute_node(i, val);
        vlink(r) = vlink(p);
        vlink(p) = r;
    }
    return;
}


int unset_attribute(halfword n, int i, int val)
{
    register halfword p;
    register int t;
    register int j = 0;

    if (!nodetype_has_attributes(type(n)))
        return null;
    p = node_attr(n);
    if (p == null)
        return UNUSED_ATTRIBUTE;
    assert(vlink(p) != null);
    while (vlink(p) != null) {
        t = attribute_id(vlink(p));
        if (t > i)
            return UNUSED_ATTRIBUTE;
        if (t == i) {
            p = vlink(p);
            break;
        }
        j++;
        p = vlink(p);
    }
    if (attribute_id(p) != i)
        return UNUSED_ATTRIBUTE;
    /* if we are still here, the attribute exists */
    p = node_attr(n);
    if (attr_list_ref(p) != 1) {
        if (attr_list_ref(p) > 1) {
            p = copy_attribute_list(p);
            delete_attribute_ref(node_attr(n));
            node_attr(n) = p;
        } else {
            fprintf(stdout,
                    "Node %d has an attribute list that is free already\n",
                    (int) n);
        }
        attr_list_ref(p) = 1;
    }
    p = vlink(p);
    while (j-- > 0)
        p = vlink(p);
    t = attribute_value(p);
    if (val == UNUSED_ATTRIBUTE || t == val) {
        attribute_value(p) = UNUSED_ATTRIBUTE;
    }
    return t;
}

int has_attribute(halfword n, int i, int val)
{
    register halfword p;
    if (!nodetype_has_attributes(type(n)))
        return UNUSED_ATTRIBUTE;
    p = node_attr(n);
    if (p == null || vlink(p) == null)
        return UNUSED_ATTRIBUTE;
    p = vlink(p);
    while (p != null) {
        if (attribute_id(p) == i) {
            int ret = attribute_value(p);
            if (val == UNUSED_ATTRIBUTE || val == ret)
                return ret;
            return UNUSED_ATTRIBUTE;
        } else if (attribute_id(p) > i) {
            return UNUSED_ATTRIBUTE;
        }
        p = vlink(p);
    }
    return UNUSED_ATTRIBUTE;
}

void print_short_node_contents(halfword p)
{
    switch (type(p)) {
    case hlist_node:
    case vlist_node:
    case ins_node:
    case whatsit_node:
    case mark_node:
    case adjust_node:
    case unset_node:
        print_char('[');
        print_char(']');
        break;
    case rule_node:
        print_char('|');
        break;
    case glue_node:
        if (glue_ptr(p) != zero_glue)
            print_char(' ');
        break;
    case math_node:
        print_char('$');
        break;
    case disc_node:
        short_display(vlink(pre_break(p)));
        short_display(vlink(post_break(p)));
        break;
    default:
        assert(1);
        break;
    }
}


void show_pdftex_whatsit_rule_spec(integer p)
{
    tprint("(");
    print_rule_dimen(pdf_height(p));
    print_char('+');
    print_rule_dimen(pdf_depth(p));
    tprint(")x");
    print_rule_dimen(pdf_width(p));
}


/*
Each new type of node that appears in our data structure must be capable
of being displayed, copied, destroyed, and so on. The routines that we
need for write-oriented whatsits are somewhat like those for mark nodes;
other extensions might, of course, involve more subtlety here.
*/


void print_write_whatsit(char *s, pointer p)
{
    tprint_esc(s);
    if (write_stream(p) < 16)
        print_int(write_stream(p));
    else if (write_stream(p) == 16)
        print_char('*');
    else
        print_char('-');
}


void show_whatsit_node(integer p)
{
    switch (subtype(p)) {
    case open_node:
        print_write_whatsit("openout", p);
        print_char('=');
        print_file_name(open_name(p), open_area(p), open_ext(p));
        break;
    case write_node:
        print_write_whatsit("write", p);
        print_mark(write_tokens(p));
        break;
    case close_node:
        print_write_whatsit("closeout", p);
        break;
    case special_node:
        tprint_esc("special");
        print_mark(write_tokens(p));
        break;
    case dir_node:
        if (dir_dir(p) < 0) {
            tprint_esc("enddir");
            print_char(' ');
            print_dir(dir_dir(p) + 64);
        } else {
            tprint_esc("begindir");
            print_char(' ');
            print_dir(dir_dir(p));
        }
    case local_par_node:
        tprint_esc("whatsit");
        append_char('.');
        print_ln();
        print_current_string();
        tprint_esc("localinterlinepenalty");
        print_char('=');
        print_int(local_pen_inter(p));
        print_ln();
        print_current_string();
        tprint_esc("localbrokenpenalty");
        print_char('=');
        print_int(local_pen_broken(p));
        print_ln();
        print_current_string();
        tprint_esc("localleftbox");
        if (local_box_left(p) == null) {
            tprint("=null");
        } else {
            append_char('.');
            show_node_list(local_box_left(p));
            decr(pool_ptr);
        }
        print_ln();
        print_current_string();
        tprint_esc("localrightbox");
        if (local_box_right(p) == null) {
            tprint("=null");
        } else {
            append_char('.');
            show_node_list(local_box_right(p));
            decr(pool_ptr);
        }
        decr(pool_ptr);
        break;
    case pdf_literal_node:
        show_pdf_literal(p);
        break;
    case pdf_colorstack_node:
        tprint_esc("pdfcolorstack ");
        print_int(pdf_colorstack_stack(p));
        switch (pdf_colorstack_cmd(p)) {
        case colorstack_set:
            tprint(" set ");
            break;
        case colorstack_push:
            tprint(" push ");
            break;
        case colorstack_pop:
            tprint(" pop");
            break;
        case colorstack_current:
            tprint(" current");
            break;
        default:
            confusion("pdfcolorstack");
            break;
        }
        if (pdf_colorstack_cmd(p) <= colorstack_data)
            print_mark(pdf_colorstack_data(p));
        break;
    case pdf_setmatrix_node:
        tprint_esc("pdfsetmatrix");
        print_mark(pdf_setmatrix_data(p));
        break;
    case pdf_save_node:
        tprint_esc("pdfsave");
        break;
    case pdf_restore_node:
        tprint_esc("pdfrestore");
        break;
    case cancel_boundary_node:
        tprint_esc("noboundary");
        break;
    case late_lua_node:
        tprint_esc("latelua");
        print_int(late_lua_reg(p));
        print_mark(late_lua_data(p));
        break;
    case close_lua_node:
        tprint_esc("closelua");
        print_int(late_lua_reg(p));
        break;
    case pdf_refobj_node:
        tprint_esc("pdfrefobj");
        if (obj_obj_is_stream(static_pdf, pdf_obj_objnum(p)) > 0) {
            if (obj_obj_stream_attr(static_pdf, pdf_obj_objnum(p)) != null) {
                tprint(" attr");
                print_mark(obj_obj_stream_attr(static_pdf, pdf_obj_objnum(p)));
            }
            tprint(" stream");
        }
        if (obj_obj_is_file(static_pdf, pdf_obj_objnum(p)) > 0)
            tprint(" file");
        print_mark(obj_obj_data(static_pdf, pdf_obj_objnum(p)));
        break;
    case pdf_refxform_node:
        tprint_esc("pdfrefxform");
        print_char('(');
        print_scaled(obj_xform_height(static_pdf, pdf_xform_objnum(p)));
        print_char('+');
        print_scaled(obj_xform_depth(static_pdf, pdf_xform_objnum(p)));
        tprint(")x");
        print_scaled(obj_xform_width(static_pdf, pdf_xform_objnum(p)));
        break;
    case pdf_refximage_node:
        tprint_esc("pdfrefximage");
        tprint("(");
        print_scaled(pdf_height(p));
        print_char('+');
        print_scaled(pdf_depth(p));
        tprint(")x");
        print_scaled(pdf_width(p));
        break;
    case pdf_annot_node:
        tprint_esc("pdfannot");
        show_pdftex_whatsit_rule_spec(p);
        print_mark(pdf_annot_data(p));
        break;
    case pdf_start_link_node:
        tprint_esc("pdfstartlink");
        show_pdftex_whatsit_rule_spec(p);
        if (pdf_link_attr(p) != null) {
            tprint(" attr");
            print_mark(pdf_link_attr(p));
        }
        tprint(" action");
        if (pdf_action_type(pdf_link_action(p)) == pdf_action_user) {
            tprint(" user");
            print_mark(pdf_action_tokens(pdf_link_action(p)));
            return;
        }
        if (pdf_action_file(pdf_link_action(p)) != null) {
            tprint(" file");
            print_mark(pdf_action_file(pdf_link_action(p)));
        }
        switch (pdf_action_type(pdf_link_action(p))) {
        case pdf_action_goto:
            if (pdf_action_named_id(pdf_link_action(p)) > 0) {
                tprint(" goto name");
                print_mark(pdf_action_id(pdf_link_action(p)));
            } else {
                tprint(" goto num");
                print_int(pdf_action_id(pdf_link_action(p)));
            }
            break;
        case pdf_action_page:
            tprint(" page");
            print_int(pdf_action_id(pdf_link_action(p)));
            print_mark(pdf_action_tokens(pdf_link_action(p)));
            break;
        case pdf_action_thread:
            if (pdf_action_named_id(pdf_link_action(p)) > 0) {
                tprint(" thread name");
                print_mark(pdf_action_id(pdf_link_action(p)));
            } else {
                tprint(" thread num");
                print_int(pdf_action_id(pdf_link_action(p)));
            }
            break;
        default:
            pdf_error("displaying", "unknown action type");
            break;
        }
        break;
    case pdf_end_link_node:
        tprint_esc("pdfendlink");
        break;
    case pdf_dest_node:
        tprint_esc("pdfdest");
        if (pdf_dest_named_id(p) > 0) {
            tprint(" name");
            print_mark(pdf_dest_id(p));
        } else {
            tprint(" num");
            print_int(pdf_dest_id(p));
        }
        print_char(' ');
        switch (pdf_dest_type(p)) {
        case pdf_dest_xyz:
            tprint("xyz");
            if (pdf_dest_xyz_zoom(p) != null) {
                tprint(" zoom");
                print_int(pdf_dest_xyz_zoom(p));
            }
            break;
        case pdf_dest_fitbh:
            tprint("fitbh");
            break;
        case pdf_dest_fitbv:
            tprint("fitbv");
            break;
        case pdf_dest_fitb:
            tprint("fitb");
            break;
        case pdf_dest_fith:
            tprint("fith");
            break;
        case pdf_dest_fitv:
            tprint("fitv");
            break;
        case pdf_dest_fitr:
            tprint("fitr");
            show_pdftex_whatsit_rule_spec(p);
            break;
        case pdf_dest_fit:
            tprint("fit");
            break;
        default:
            tprint("unknown!");
            break;
        }
        break;
    case pdf_thread_node:
    case pdf_start_thread_node:
        if (subtype(p) == pdf_thread_node)
            tprint_esc("pdfthread");
        else
            tprint_esc("pdfstartthread");
        tprint("(");
        print_rule_dimen(pdf_height(p));
        print_char('+');
        print_rule_dimen(pdf_depth(p));
        tprint(")x");
        print_rule_dimen(pdf_width(p));
        if (pdf_thread_attr(p) != null) {
            tprint(" attr");
            print_mark(pdf_thread_attr(p));
        }
        if (pdf_thread_named_id(p) > 0) {
            tprint(" name");
            print_mark(pdf_thread_id(p));
        } else {
            tprint(" num");
            print_int(pdf_thread_id(p));
        }
        break;
    case pdf_end_thread_node:
        tprint_esc("pdfendthread");
        break;
    case pdf_save_pos_node:
        tprint_esc("pdfsavepos");
        break;
    case user_defined_node:
        tprint_esc("whatsit");
        print_int(user_node_id(p));
        print_char('=');
        switch (user_node_type(p)) {
        case 'a':
            tprint("<>");
            break;
        case 'n':
            tprint("[");
            show_node_list(user_node_value(p));
            tprint("]");
            break;
        case 's':
            print_char('"');
            print(user_node_value(p));
            print_char('"');
            break;
        case 't':
            print_mark(user_node_value(p));
            break;
        default:               /* only 'd' */
            print_int(user_node_value(p));
            break;
        }
        break;
    default:
        tprint("whatsit?");
        break;
    }
}


/*
  Now we are ready for |show_node_list| itself. This procedure has been
  written to be ``extra robust'' in the sense that it should not crash or get
  into a loop even if the data structures have been messed up by bugs in
  the rest of the program. You can safely call its parent routine
  |show_box(p)| for arbitrary values of |p| when you are debugging \TeX.
  However, in the presence of bad data, the procedure may
  fetch a |memory_word| whose variant is different from the way it was stored;
  for example, it might try to read |mem[p].hh| when |mem[p]|
  contains a scaled integer, if |p| is a pointer that has been
  clobbered or chosen at random.
*/

/* |str_room| need not be checked; see |show_box|  */

/* Recursive calls on |show_node_list| therefore use the following pattern: */

#define node_list_display(A) do {               \
    append_char('.');                           \
    show_node_list(A);                          \
    flush_char();                               \
} while (0)

void show_node_list(integer p)
{                               /* prints a node list symbolically */
    integer n;                  /* the number of items already printed at this level */
    real g;                     /* a glue ratio, as a floating point number */
    if (cur_length > depth_threshold) {
        if (p > null)
            tprint(" []");      /* indicate that there's been some truncation */
        return;
    }
    n = 0;
    while (p != null) {
        print_ln();
        print_current_string(); /* display the nesting history */
        if (int_par(tracing_online_code) < -2)
            print_int(p);
        incr(n);
        if (n > breadth_max) {  /* time to stop */
            tprint("etc.");
            return;
        }
        /* @<Display node |p|@> */
        if (is_char_node(p)) {
            print_font_and_char(p);
            if (is_ligature(p)) {
                /* @<Display ligature |p|@>; */
                tprint(" (ligature ");
                if (is_leftboundary(p))
                    print_char('|');
                font_in_short_display = font(p);
                short_display(lig_ptr(p));
                if (is_rightboundary(p))
                    print_char('|');
                print_char(')');
            }
        } else {
            switch (type(p)) {
            case hlist_node:
            case vlist_node:
            case unset_node:
                /* @<Display box |p|@>; */
                if (type(p) == hlist_node)
                    tprint_esc("h");
                else if (type(p) == vlist_node)
                    tprint_esc("v");
                else
                    tprint_esc("unset");
                tprint("box(");
                print_scaled(height(p));
                print_char('+');
                print_scaled(depth(p));
                tprint(")x");
                print_scaled(width(p));
                if (type(p) == unset_node) {
                    /* @<Display special fields of the unset node |p|@>; */
                    if (span_count(p) != min_quarterword) {
                        tprint(" (");
                        print_int(span_count(p) + 1);
                        tprint(" columns)");
                    }
                    if (glue_stretch(p) != 0) {
                        tprint(", stretch ");
                        print_glue(glue_stretch(p), glue_order(p), NULL);
                    }
                    if (glue_shrink(p) != 0) {
                        tprint(", shrink ");
                        print_glue(glue_shrink(p), glue_sign(p), NULL);
                    }
                } else {
                    /*<Display the value of |glue_set(p)|@> */
                    /* The code will have to change in this place if |glue_ratio| is
                       a structured type instead of an ordinary |real|. Note that this routine
                       should avoid arithmetic errors even if the |glue_set| field holds an
                       arbitrary random value. The following code assumes that a properly
                       formed nonzero |real| number has absolute value $2^{20}$ or more when
                       it is regarded as an integer; this precaution was adequate to prevent
                       floating point underflow on the author's computer.
                     */

                    g = (real) (glue_set(p));
                    if ((g != 0.0) && (glue_sign(p) != normal)) {
                        tprint(", glue set ");
                        if (glue_sign(p) == shrinking)
                            tprint("- ");
                        if (g > 20000.0 || g < -20000.0) {
                            if (g > 0.0)
                                print_char('>');
                            else
                                tprint("< -");
                            print_glue(20000 * unity, glue_order(p), NULL);
                        } else {
                            print_glue(round(unity * g), glue_order(p), NULL);
                        }
                    }

                    if (shift_amount(p) != 0) {
                        tprint(", shifted ");
                        print_scaled(shift_amount(p));
                    }
                    tprint(", direction ");
                    print_dir(box_dir(p));
                }
                node_list_display(list_ptr(p)); /* recursive call */
                break;
            case rule_node:
                /* @<Display rule |p|@>; */
                tprint_esc("rule(");
                print_rule_dimen(height(p));
                print_char('+');
                print_rule_dimen(depth(p));
                tprint(")x");
                print_rule_dimen(width(p));
                break;
            case ins_node:
                /* @<Display insertion |p|@>; */
                tprint_esc("insert");
                print_int(subtype(p));
                tprint(", natural size ");
                print_scaled(height(p));
                tprint("; split(");
                print_spec(split_top_ptr(p), NULL);
                print_char(',');
                print_scaled(depth(p));
                tprint("); float cost ");
                print_int(float_cost(p));
                node_list_display(ins_ptr(p));  /* recursive call */
                break;
            case whatsit_node:
                show_whatsit_node(p);
                break;
            case glue_node:
                /* @<Display glue |p|@>; */
                if (subtype(p) >= a_leaders) {
                    /* @<Display leaders |p|@>; */
                    tprint_esc("");
                    if (subtype(p) == c_leaders)
                        print_char('c');
                    else if (subtype(p) == x_leaders)
                        print_char('x');
                    tprint("leaders ");
                    print_spec(glue_ptr(p), NULL);
                    node_list_display(leader_ptr(p));   /* recursive call */
                } else {
                    tprint_esc("glue");
                    if (subtype(p) != normal) {
                        print_char('(');
                        if (subtype(p) < cond_math_glue)
                            print_skip_param(subtype(p) - 1);
                        else if (subtype(p) == cond_math_glue)
                            tprint_esc("nonscript");
                        else
                            tprint_esc("mskip");
                        print_char(')');
                    }
                    if (subtype(p) != cond_math_glue) {
                        print_char(' ');
                        if (subtype(p) < cond_math_glue)
                            print_spec(glue_ptr(p), NULL);
                        else
                            print_spec(glue_ptr(p), "mu");
                    }
                }
                break;
            case margin_kern_node:
                tprint_esc("kern");
                print_scaled(width(p));
                if (subtype(p) == left_side)
                    tprint(" (left margin)");
                else
                    tprint(" (right margin)");
                break;
            case kern_node:
                /* @<Display kern |p|@>; */
                /*  An ``explicit'' kern value is indicated implicitly by an explicit space. */
                if (subtype(p) != mu_glue) {
                    tprint_esc("kern");
                    if (subtype(p) != normal)
                        print_char(' ');
                    print_scaled(width(p));
                    if (subtype(p) == acc_kern)
                        tprint(" (for accent)");
                } else {
                    tprint_esc("mkern");
                    print_scaled(width(p));
                    tprint("mu");
                }
                break;
            case math_node:
                /* @<Display math node |p|@>; */
                tprint_esc("math");
                if (subtype(p) == before)
                    tprint("on");
                else
                    tprint("off");
                if (width(p) != 0) {
                    tprint(", surrounded ");
                    print_scaled(width(p));
                }
                break;
            case penalty_node:
                /* @<Display penalty |p|@>; */
                tprint_esc("penalty ");
                print_int(penalty(p));
                break;
            case disc_node:
                /* @<Display discretionary |p|@>; */
                /* The |post_break| list of a discretionary node is indicated by a prefixed
                   `\.{\char'174}' instead of the `\..' before the |pre_break| list. */
                tprint_esc("discretionary");
                if (vlink(no_break(p)) != null) {
                    tprint(" replacing ");
                    node_list_display(vlink(no_break(p)));
                }
                node_list_display(vlink(pre_break(p))); /* recursive call */
                append_char('|');
                show_node_list(vlink(post_break(p)));
                flush_char();   /* recursive call */
                break;
            case mark_node:
                /* @<Display mark |p|@>; */
                tprint_esc("mark");
                if (mark_class(p) != 0) {
                    print_char('s');
                    print_int(mark_class(p));
                }
                print_mark(mark_ptr(p));
                break;
            case adjust_node:
                /* @<Display adjustment |p|@>; */
                tprint_esc("vadjust");
                if (adjust_pre(p) != 0)
                    tprint(" pre ");
                node_list_display(adjust_ptr(p));       /* recursive call */
                break;
            default:
                show_math_node(p);
                break;
            }
        }
        p = vlink(p);
    }
}

/* This routine finds the 'base' width of a horizontal box, using the same logic
  that \TeX82 used for \.{\\predisplaywidth} */

pointer actual_box_width(pointer r, scaled base_width)
{
    scaled w;                   /* calculated |size| */
    pointer p;                  /* current node when calculating |pre_display_size| */
    pointer q;                  /* glue specification when calculating |pre_display_size| */
    internal_font_number f;     /* font in current |char_node| */
    scaled d;                   /* increment to |v| */
    scaled v;                   /* |w| plus possible glue amount */
    w = -max_dimen;
    v = shift_amount(r) + base_width;
    p = list_ptr(r);
    while (p != null) {
        if (is_char_node(p)) {
            f = font(p);
            d = glyph_width(p);
            goto found;
        }
        switch (type(p)) {
        case hlist_node:
        case vlist_node:
        case rule_node:
            d = width(p);
            goto found;
            break;
        case margin_kern_node:
            d = width(p);
            break;
        case kern_node:
            d = width(p);
            break;
        case math_node:
            d = surround(p);
            break;
        case glue_node:
            /* We need to be careful that |w|, |v|, and |d| do not depend on any |glue_set|
               values, since such values are subject to system-dependent rounding.
               System-dependent numbers are not allowed to infiltrate parameters like
               |pre_display_size|, since \TeX82 is supposed to make the same decisions on all
               machines.
             */
            q = glue_ptr(p);
            d = width(q);
            if (glue_sign(r) == stretching) {
                if ((glue_order(r) == stretch_order(q))
                    && (stretch(q) != 0))
                    v = max_dimen;
            } else if (glue_sign(r) == shrinking) {
                if ((glue_order(r) == shrink_order(q))
                    && (shrink(q) != 0))
                    v = max_dimen;
            }
            if (subtype(p) >= a_leaders)
                goto found;
            break;
        case whatsit_node:
            if ((subtype(p) == pdf_refxform_node)
                || (subtype(p) == pdf_refximage_node))
                d = pdf_width(p);
            else
                d = 0;
            break;
        default:
            d = 0;
            break;
        }
        if (v < max_dimen)
            v = v + d;
        goto not_found;
      found:
        if (v < max_dimen) {
            v = v + d;
            w = v;
        } else {
            w = max_dimen;
            break;
        }
      not_found:
        p = vlink(p);
    }
    return w;
}


halfword tail_of_list (halfword p)
{ 
    halfword q=p;
    while (vlink(q)!=null)  
        q=vlink(q);
    return q;
}


/* |delete_glue_ref| is called when a pointer to a glue
   specification is being withdrawn.
*/

#define fast_delete_glue_ref(A) do {		\
    if (glue_ref_count(A)==null) {		\
      flush_node(A);				\
    } else {					\
      decr(glue_ref_count(A));			\
    }						\
  } while (0)

void delete_glue_ref(halfword p) /* |p| points to a glue specification */
{
  assert(type(p)==glue_spec_node);
  fast_delete_glue_ref(p);
}

halfword var_used;
halfword temp_ptr; /* a pointer variable for occasional emergency use */

/*
Attribute lists need two extra globals to increase processing efficiency.
|max_used_attr| limits the test loop that checks for set attributes, and
|attr_list_cache| contains a pointer to an already created attribute list.  It is
set to the special value |cache_disabled| when the current value can no longer be
trusted: after an assignment to an attribute register, and after a group has
ended.
*/

integer max_used_attr; /* maximum assigned attribute id  */
halfword attr_list_cache;

/*
From the computer's standpoint, \TeX's chief mission is to create
horizontal and vertical lists. We shall now investigate how the elements
of these lists are represented internally as nodes in the dynamic memory.

A horizontal or vertical list is linked together by |link| fields in
the first word of each node. Individual nodes represent boxes, glue,
penalties, or special things like discretionary hyphens; because of this
variety, some nodes are longer than others, and we must distinguish different
kinds of nodes. We do this by putting a `|type|' field in the first word,
together with the link and an optional `|subtype|'.
*/

/*
Character nodes appear only in horizontal lists, never in vertical lists.

An |hlist_node| stands for a box that was made from a horizontal list.
Each |hlist_node| is seven words long, and contains the following fields
(in addition to the mandatory |type| and |link|, which we shall not
mention explicitly when discussing the other node types): The |height| and
|width| and |depth| are scaled integers denoting the dimensions of the
box.  There is also a |shift_amount| field, a scaled integer indicating
how much this box should be lowered (if it appears in a horizontal list),
or how much it should be moved to the right (if it appears in a vertical
list). There is a |list_ptr| field, which points to the beginning of the
list from which this box was fabricated; if |list_ptr| is |null|, the box
is empty. Finally, there are three fields that represent the setting of
the glue:  |glue_set(p)| is a word of type |glue_ratio| that represents
the proportionality constant for glue setting; |glue_sign(p)| is
|stretching| or |shrinking| or |normal| depending on whether or not the
glue should stretch or shrink or remain rigid; and |glue_order(p)|
specifies the order of infinity to which glue setting applies (|normal|,
|sfi|, |fil|, |fill|, or |filll|). The |subtype| field is not used.
*/

/*
@d width_offset=2 {position of |width| field in a box node}
@d depth_offset=3 {position of |depth| field in a box node}
@d height_offset=4 {position of |height| field in a box node}
@d width(#) == vmem(#+width_offset).sc {width of the box, in sp}
@d depth(#) == vmem(#+depth_offset).sc {depth of the box, in sp}
@d height(#) == vmem(#+height_offset).sc {height of the box, in sp}
@d shift_amount(#) == vlink(#+5) {repositioning distance, in sp}
@d box_dir(#) == vinfo(#+5) {position of |box_dir| in a box node}
@d list_offset=6 {position of |list_ptr| field in a box node}
@d list_ptr(#) == vlink(#+list_offset) {beginning of the list inside the box}
@d glue_order(#) == subtype(#+list_offset) {applicable order of infinity}
@d glue_sign(#) == type(#+list_offset) {stretching or shrinking}
@d normal=0 {the most common case when several cases are named}
@d stretching = 1 {glue setting applies to the stretch components}
@d shrinking = 2 {glue setting applies to the shrink components}
@d glue_set(#) == vmem(#+7).gr {a word of type |glue_ratio| for glue setting}
*/

/*
The |new_null_box| function returns a pointer to an |hlist_node| in
which all subfields have the values corresponding to `\.{\\hbox\{\}}'.
The |subtype| field is set to |min_quarterword|, since that's the desired
|span_count| value if this |hlist_node| is changed to an |unset_node|.
*/

halfword new_null_box (void) /* creates a new box node */
{
    halfword p; /* the new node */
    p=new_node(hlist_node,min_quarterword);
    box_dir(p)=text_direction;
    return p;
}

/*
A |vlist_node| is like an |hlist_node| in all respects except that it
contains a vertical list.
*/

/*
A |rule_node| stands for a solid black rectangle; it has |width|,
|depth|, and |height| fields just as in an |hlist_node|. However, if
any of these dimensions is $-2^{30}$, the actual value will be determined
by running the rule up to the boundary of the innermost enclosing box.
This is called a ``running dimension.'' The |width| is never running in
an hlist; the |height| and |depth| are never running in a~vlist.

@d null_flag==-@'10000000000 {$-2^{30}$, signifies a missing item}
@d is_running(#) == (#=null_flag) {tests for a running dimension}
@d rule_dir(#)==vlink(#+5)
*/

/*
A new rule node is delivered by the |new_rule| function. It
makes all the dimensions ``running,'' so you have to change the
ones that are not allowed to run.
*/

halfword new_rule (void)
{
    halfword p; /* the new node */
    p=new_node(rule_node,0); /* the |subtype| is not used */
    return p;
}

/*
@ Insertions are represented by |ins_node| records, where the |subtype|
indicates the corresponding box number. For example, `\.{\\insert 250}'
leads to an |ins_node| whose |subtype| is |250+min_quarterword|.
The |height| field of an |ins_node| is slightly misnamed; it actually holds
the natural height plus depth of the vertical list being inserted.
The |depth| field holds the |split_max_depth| to be used in case this
insertion is split, and the |split_top_ptr| points to the corresponding
|split_top_skip|. The |float_cost| field holds the |floating_penalty| that
will be used if this insertion floats to a subsequent page after a
split insertion of the same class.  There is one more field, the
|ins_ptr|, which points to the beginning of the vlist for the insertion.

@d float_cost(#)==vmem(#+2).int {the |floating_penalty| to be used}
@d ins_ptr(#)==vinfo(#+5) {the vertical list to be inserted}
@d split_top_ptr(#)==vlink(#+5) {the |split_top_skip| to be used}

@ A |mark_node| has a |mark_ptr| field that points to the reference count
of a token list that contains the user's \.{\\mark} text.
In addition there is a |mark_class| field that contains the mark class.

@d mark_ptr(#)==vlink(#+2) {head of the token list for a mark}
@d mark_class(#)==vinfo(#+2) {the mark class}

@ An |adjust_node|, which occurs only in horizontal lists,
specifies material that will be moved out into the surrounding
vertical list; i.e., it is used to implement \TeX's `\.{\\vadjust}'
operation.  The |adjust_ptr| field points to the vlist containing this
material.

@d adjust_pre == subtype  {pre-adjustment?}
@d adjust_ptr(#)==vlink(#+2) {vertical list to be moved out of horizontal list}
@#{|append_list| is used to append a list to |tail|}
@d append_list(#) == begin vlink(tail) := vlink(#); append_list_end
@d append_list_end(#) == tail := #; end

@ A |glyph_node|, which occurs only in horizontal lists, specifies a
glyph in a particular font, along with its attribute list. Older
versions of \TeX\ could use token memory for characters, because the
font,char combination would fit in a single word (both values were
required to be strictly less than $2^{16}$). In \LuaTeX, room is
needed for characters that are larger than that, as well as a pointer
to a potential attribute list, and the two displacement values.

In turn, that made the node so large that it made sense to merge
ligature glyphs as well, as that requires only one extra pointer.  A
few extra classes of glyph nodes will be introduced later.  The
unification of all those types makes it easier to manipulate lists of
glyphs. The subtype differentiates various glyph kinds.

First, here is a function that returns a pointer to a glyph node for a given
glyph in a given font. If that glyph doesn't exist, |null| is returned
instead.  Nodes of this subtype are directly created only for accents
and their base (through |make_accent|), and math nucleus items (in the
conversion from |mlist| to |hlist|).


@d is_char_node(#) == ((#<>null)and(type(#)=glyph_node))
@d font(#)==vlink(#+2) {the font code in a |glyph_node|}
@d character(#)==vinfo(#+2) {the character code in a |glyph_node|}
@d lig_ptr(#)==vlink(#+3) {the list of characters for ligature replacements }
@d lang_data(#)==vinfo(#+3) {language information }
@d x_displace(#)==vinfo(#+4) { horizontal displacement }
@d y_displace(#)==vlink(#+4) { vertical displacement  }
*/

halfword new_glyph(integer f, integer c)
{
    halfword p = null; /* the new node */
    if ((f==0) || (char_exists(f,c))) {
	p=new_glyph_node();
	set_to_glyph(p); 
	font(p)=f;  
	character(p)=c;
    }
    return p;
}

/*
A subset of the glyphs nodes represent ligatures: characters
fabricated from the interaction of two or more actual characters.  The
characters that generated the ligature have not been forgotten, since
they are needed for diagnostic messages; the |lig_ptr| field points to
a linked list of character nodes for all original characters that have
been deleted. (This list might be empty if the characters that
generated the ligature were retained in other nodes.)

The |subtype| field of these |glyph_node|s is 1, plus 2 and/or 1 if
the original source of the ligature included implicit left and/or
right boundaries. These nodes are created by the C function |new_ligkern|.

@ A third general type of glyphs could be called a character, as it
only appears in lists that are not yet processed by the ligaturing and
kerning steps of the program.

|main_control| inserts these, and they are later converted to
|subtype_normal| by |new_ligkern|.
*/

quarterword norm_min(integer h)
{
    if (h<=0) 
	return 1;
    else if (h>=255) 
	return 255;
    else 
	return h;
}

halfword new_char(integer f, integer c)
{
    halfword p; /* the new node */
    p=new_glyph_node();
    set_to_character(p); 
    font(p)=f;  
    character(p)=c;
    lang_data(p)=make_lang_data(uc_hyph,cur_lang,left_hyphen_min,right_hyphen_min);
    return p;
}

/*
Left and right ghost glyph nodes are the result of \.{\\leftghost}
and \.{\\rightghost}, respectively. They are going to be removed by
|new_ligkern|, at the end of which they are no longer needed.

@ Here are a few handy helpers used by the list output routines.
*/

scaled glyph_width(halfword p)
{
    scaled w;
    w = char_width(font(p),character(p));
    return w;
}

scaled glyph_height(halfword p)
{
    scaled w;
    w = char_height(font(p),character(p)) + y_displace(p);
    if (w<0) w=0;
    return w;
}

scaled glyph_depth(halfword p)
{
    scaled w;
    w = char_depth(font(p),character(p));
    if (y_displace(p)>0)  
	w = w - y_displace(p);
    if (w<0) w=0;
    return w;
}

/*
@ A |disc_node|, which occurs only in horizontal lists, specifies a
``dis\-cretion\-ary'' line break. If such a break occurs at node |p|, the text
that starts at |pre_break(p)| will precede the break, the text that starts at
|post_break(p)| will follow the break, and text that appears in
|no_break(p)| nodes will be ignored. For example, an ordinary
discretionary hyphen, indicated by `\.{\\-}', yields a |disc_node| with
|pre_break| pointing to a |char_node| containing a hyphen, |post_break=null|,
and |no_break=null|.

{TODO: Knuth said: All three of the discretionary texts must be lists
that consist entirely of character, kern, box and rule nodes.}

If |subtype(p)=automatic_disc|, the |ex_hyphen_penalty| will be charged for this
break.  Otherwise the |hyphen_penalty| will be charged.  The texts will
actually be substituted into the list by the line-breaking algorithm if it
decides to make the break, and the discretionary node will disappear at
that time; thus, the output routine sees only discretionaries that were
not chosen.

@d automatic_disc=2
@d syllable_disc=3
@d disc_type==subtype {the kind of discretionary}
@d pre_break(#)==vinfo(#+2) {text that precedes a discretionary break}
@d post_break(#)==vlink(#+2) {text that follows a discretionary break}
@d no_break(#)==vlink(#+3) {text this discretionary break replaces}
@d pre_break_head(#)==(#+4)
@d post_break_head(#)==(#+6)
@d no_break_head(#)==(#+8)
@d tlink(#)==vinfo(#+1)
*/

halfword new_disc(void) /* creates an empty |disc_node| */
{
    halfword p; /* the new node */
    p=new_node(disc_node,0);
    return p;
}

/*
A |whatsit_node| is a wild card reserved for extensions to \TeX. The
|subtype| field in its first word says what `\\{whatsit}' it is, and
implicitly determines the node size (which must be 2 or more) and the
format of the remaining words. When a |whatsit_node| is encountered
in a list, special actions are invoked; knowledgeable people who are
careful not to mess up the rest of \TeX\ are able to make \TeX\ do new
things by adding code at the end of the program. For example, there
might be a `\TeX nicolor' extension to specify different colors of ink,
@^extensions to \TeX@>
and the whatsit node might contain the desired parameters.

The present implementation of \TeX\ treats the features associated with
`\.{\\write}' and `\.{\\special}' as if they were extensions, in order to
illustrate how such routines might be coded. We shall defer further
discussion of extensions until the end of this program.
*/

/*
@ A |math_node|, which occurs only in horizontal lists, appears before and
after mathematical formulas. The |subtype| field is |before| before the
formula and |after| after it. There is a |surround| field, which represents
the amount of surrounding space inserted by \.{\\mathsurround}.

@d surround(#)==vlink(#+2)
@d before=0 {|subtype| for math node that introduces a formula}
@d after=1 {|subtype| for math node that winds up a formula}
*/

halfword new_math(scaled w, int s)
{
    halfword p; /* the new node */
     p=new_node(math_node,s);
     surround(p)=w; 
     return p;
}

/*
\TeX\ makes use of the fact that |hlist_node|, |vlist_node|,
|rule_node|, |ins_node|, |mark_node|, |adjust_node|,
|disc_node|, |whatsit_node|, and |math_node| are at the low end of the
type codes, by permitting a break at glue in a list if and only if the
|type| of the previous node is less than |math_node|. Furthermore, a
node is discarded after a break if its type is |math_node| or~more.

@d precedes_break(#)==(type(#)<math_node)
@d non_discardable(#)==(type(#)<math_node)

@ A |glue_node| represents glue in a list. However, it is really only
a pointer to a separate glue specification, since \TeX\ makes use of the
fact that many essentially identical nodes of glue are usually present.
If |p| points to a |glue_node|, |glue_ptr(p)| points to
another packet of words that specify the stretch and shrink components, etc.

Glue nodes also serve to represent leaders; the |subtype| is used to
distinguish between ordinary glue (which is called |normal|) and the three
kinds of leaders (which are called |a_leaders|, |c_leaders|, and |x_leaders|).
The |leader_ptr| field points to a rule node or to a box node containing the
leaders; it is set to |null| in ordinary glue nodes.

Many kinds of glue are computed from \TeX's ``skip'' parameters, and
it is helpful to know which parameter has led to a particular glue node.
Therefore the |subtype| is set to indicate the source of glue, whenever
it originated as a parameter. We will be defining symbolic names for the
parameter numbers later (e.g., |line_skip_code=0|, |baseline_skip_code=1|,
etc.); it suffices for now to say that the |subtype| of parametric glue
will be the same as the parameter number, plus~one.

In math formulas there are two more possibilities for the |subtype| in a
glue node: |mu_glue| denotes an \.{\\mskip} (where the units are scaled \.{mu}
instead of scaled \.{pt}); and |cond_math_glue| denotes the `\.{\\nonscript}'
feature that cancels the glue node immediately following if it appears
in a subscript.

@d cond_math_glue=98 {special |subtype| to suppress glue in the next node}
@d mu_glue=99 {|subtype| for math glue}
@d a_leaders=100 {|subtype| for aligned leaders}
@d c_leaders=101 {|subtype| for centered leaders}
@d x_leaders=102 {|subtype| for expanded leaders}
@d glue_ptr(#)==vinfo(#+2) {pointer to a glue specification}
@d leader_ptr(#)==vlink(#+2) {pointer to box or rule node for leaders}

@ A glue specification has a halfword reference count in its first word,
@^reference counts@>
representing |null| plus the number of glue nodes that point to it (less one).
Note that the reference count appears in the same position as
the |link| field in list nodes; this is the field that is initialized
to |null| when a node is allocated, and it is also the field that is flagged
by |empty_flag| in empty nodes.

Glue specifications also contain three |scaled| fields, for the |width|,
|stretch|, and |shrink| dimensions. Finally, there are two one-byte
fields called |stretch_order| and |shrink_order|; these contain the
orders of infinity (|normal|, |sfi|, |fil|, |fill|, or |filll|)
corresponding to the stretch and shrink values.

@d glue_ref_count(#) == vlink(#+3) {reference count of a glue specification}
@d stretch(#) == vlink(#+1) {the stretchability of this glob of glue}
@d shrink(#) == vinfo(#+1) {the shrinkability of this glob of glue}
@d stretch_order(#) == type(#+3) {order of infinity for stretching}
@d shrink_order(#) == subtype(#+3) {order of infinity for shrinking}
@d sfi=1 {first-order infinity}
@d fil=2 {second-order infinity}
@d fill=3 {third-order infinity}
@d filll=4 {fourth-order infinity}
*/

/*
@ Here is a function that returns a pointer to a copy of a glue spec.
The reference count in the copy is |null|, because there is assumed
to be exactly one reference to the new specification.
*/

halfword new_spec(halfword p) /* duplicates a glue specification */
{
    halfword q; /* the new spec */
    q=copy_node(p);
    glue_ref_count(q)=null;
    return q;
}

/*
@ And here's a function that creates a glue node for a given parameter
identified by its code number; for example,
|new_param_glue(line_skip_code)| returns a pointer to a glue node for the
current \.{\\lineskip}.
*/

halfword new_param_glue(int n)
{
    halfword p; /* the new node */
    halfword q; /* the glue specification */
    p=new_node(glue_node,n+1);
    q=glue_par(n);
    glue_ptr(p)=q; 
    incr(glue_ref_count(q));
    return p;
}

/*
Glue nodes that are more or less anonymous are created by |new_glue|,
whose argument points to a glue specification.
*/

halfword new_glue(halfword q)
{
    halfword p; /* the new node */
    p=new_node(glue_node,normal);
    glue_ptr(p)=q; 
    incr(glue_ref_count(q));
    return p;
}

/*
Still another subroutine is needed: This one is sort of a combination
of |new_param_glue| and |new_glue|. It creates a glue node for one of
the current glue parameters, but it makes a fresh copy of the glue
specification, since that specification will probably be subject to change,
while the parameter will stay put. The global variable |temp_ptr| is
set to the address of the new spec.
*/

halfword new_skip_param(int n) 
{
    halfword p; /* the new node */
    temp_ptr=new_spec(glue_par(n));
    p=new_glue(temp_ptr); 
    glue_ref_count(temp_ptr)=null; 
    subtype(p)=n+1;
    return p;
}

/*
@ A |kern_node| has a |width| field to specify a (normally negative)
amount of spacing. This spacing correction appears in horizontal lists
between letters like A and V when the font designer said that it looks
better to move them closer together or further apart. A kern node can
also appear in a vertical list, when its `|width|' denotes additional
spacing in the vertical direction. The |subtype| is either |normal| (for
kerns inserted from font information or math mode calculations) or |explicit|
(for kerns inserted from \.{\\kern} and \.{\\/} commands) or |acc_kern|
(for kerns inserted from non-math accents) or |mu_glue| (for kerns
inserted from \.{\\mkern} specifications in math formulas).

@d explicit=1 {|subtype| of kern nodes from \.{\\kern} and \.{\\/}}
@d acc_kern=2 {|subtype| of kern nodes from accents}

@# {memory structure for marginal kerns}
@d margin_char(#) == vlink(#+3)

@# {|subtype| of marginal kerns}
@d left_side == 0
@d right_side == 1

@# {base for lp/rp/ef codes starts from 2:
    0 for |hyphen_char|,
    1 for |skew_char|}
@d lp_code_base == 2
@d rp_code_base == 3
@d ef_code_base == 4
@d tag_code == 5

@d auto_kern == explicit
@d no_lig_code == 6

@ The |new_kern| function creates a kern node having a given width.
*/

halfword new_kern(scaled w) 
{
    halfword p; /* the new node */
    p=new_node(kern_node,normal);
    width(p)=w;
    return p;
}

/*
A |penalty_node| specifies the penalty associated with line or page
breaking, in its |penalty| field. This field is a fullword integer, but
the full range of integer values is not used: Any penalty |>=10000| is
treated as infinity, and no break will be allowed for such high values.
Similarly, any penalty |<=-10000| is treated as negative infinity, and a
break will be forced.

@d inf_penalty==inf_bad {``infinite'' penalty value}
@d eject_penalty==-inf_penalty {``negatively infinite'' penalty value}
@d penalty(#) == vlink(#+2) {the added cost of breaking a list here}

@ Anyone who has been reading the last few sections of the program will
be able to guess what comes next.
*/

halfword new_penalty(integer m)
{
    halfword p; /* the new node */
    p=new_node(penalty_node,0); /* the |subtype| is not used */
    penalty(p)=m; 
    return p;
}

/*
@ You might think that we have introduced enough node types by now. Well,
almost, but there is one more: An |unset_node| has nearly the same format
as an |hlist_node| or |vlist_node|; it is used for entries in \.{\\halign}
or \.{\\valign} that are not yet in their final form, since the box
dimensions are their ``natural'' sizes before any glue adjustment has been
made. The |glue_set| word is not present; instead, we have a |glue_stretch|
field, which contains the total stretch of order |glue_order| that is
present in the hlist or vlist being boxed.
Similarly, the |shift_amount| field is replaced by a |glue_shrink| field,
containing the total shrink of order |glue_sign| that is present.
The |subtype| field is called |span_count|; an unset box typically
contains the data for |qo(span_count)+1| columns.
Unset nodes will be changed to box nodes when alignment is completed.

@d glue_stretch(#)==vmem(#+7).sc {total stretch in an unset node}
@d glue_shrink==shift_amount {total shrink in an unset node}
@d span_count==subtype {indicates the number of spanned columns}

@ In fact, there are still more types coming. When we get to math formula
processing we will see that a |style_node| has |type=14|; and a number
of larger type codes will also be defined, for use in math mode only.

@ Warning: If any changes are made to these data structure layouts, such as
changing any of the node sizes or even reordering the words of nodes,
the |copy_node_list| procedure and the memory initialization code
below may have to be changed. Such potentially dangerous parts of the
program are listed in the index under `data structure assumptions'.
@!@^data structure assumptions@>
However, other references to the nodes are made symbolically in terms of
the \.{WEB} macro definitions above, so that format changes will leave
\TeX's other algorithms intact.
@^system dependencies@>
*/
