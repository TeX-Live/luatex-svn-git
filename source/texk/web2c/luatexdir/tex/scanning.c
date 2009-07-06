/* scanning.c
   
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

#include "luatex-api.h"
#include <ptexlib.h>

static const char _svn_version[] =
    "$Id$"
    "$URL$";

#include "commands.h"

#define prev_depth cur_list.aux_field.cint
#define space_factor cur_list.aux_field.hh.lhfield
#define par_shape_ptr  loc_par(param_par_shape_code)
#define text(A) hash[(A)].rh
#define font_id_text(A) text(get_font_id_base()+(A))

#define frozen_control_sequence (get_nullcs() + 1 + get_hash_size())
#define frozen_relax (frozen_control_sequence+7)
#define frozen_primitive (frozen_control_sequence+11)

/* base for lp/rp/ef codes starts from 2:  0 for |hyphen_char|, 1 for |skew_char| */
#define lp_code_base 2
#define rp_code_base 3
#define ef_code_base 4
#define tag_code 5
#define no_lig_code 6

#define cat_code_base (static_int_base-5)       /* current category code data index  */
#define lc_code_base (static_int_base-4)        /* table of |number_chars| lowercase mappings */
#define uc_code_base (static_int_base-3)        /* table of |number_chars| uppercase mappings */
#define sf_code_base (static_int_base-2)        /* table of |number_chars| spacefactor mappings */
#define math_code_base (static_int_base-1)      /* table of |number_chars| math mode mappings */

#define level_one 1

#define attribute(A) zeqtb[get_attribute_base()+(A)].hh.rh
#define dimen(A) zeqtb[get_dimen_base()+(A)].hh.rh
#undef skip
#define skip(A) zeqtb[get_skip_base()+(A)].hh.rh
#define mu_skip(A) zeqtb[get_mu_skip_base()+(A)].hh.rh
#define count(A) zeqtb[get_count_base()+(A)].hh.rh
#define box(A) zeqtb[get_box_base()+(A)].hh.rh

/*
Let's turn now to some procedures that \TeX\ calls upon frequently to digest
certain kinds of patterns in the input. Most of these are quite simple;
some are quite elaborate. Almost all of the routines call |get_x_token|,
which can cause them to be invoked recursively.

The |scan_left_brace| routine is called when a left brace is supposed to be
the next non-blank token. (The term ``left brace'' means, more precisely,
a character whose catcode is |left_brace|.) \TeX\ allows \.{\\relax} to
appear before the |left_brace|.
*/

void scan_left_brace(void)
{                               /* reads a mandatory |left_brace| */
    /* Get the next non-blank non-relax non-call token */
    do {
        get_x_token();
    } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

    if (cur_cmd != left_brace_cmd) {
        print_err("Missing { inserted");
        help4("A left brace was mandatory here, so I've put one in.",
              "You might want to delete and/or insert some corrections",
              "so that I will find a matching right brace soon.",
              "If you're confused by all this, try typing `I}' now.");
        back_error();
        cur_tok = left_brace_token + '{';
        cur_cmd = left_brace_cmd;
        cur_chr = '{';
        incr(align_state);
    }
}

/*
The |scan_optional_equals| routine looks for an optional `\.=' sign preceded
by optional spaces; `\.{\\relax}' is not ignored here.
*/

void scan_optional_equals(void)
{
    /* Get the next non-blank non-call token */
    do {
        get_x_token();
    } while (cur_cmd == spacer_cmd);
    if (cur_tok != other_token + '=')
        back_input();
}

/*
Here is a procedure that sounds an alarm when mu and non-mu units
are being switched.
*/

static void mu_error(void)
{
    print_err("Incompatible glue units");
    help1("I'm going to assume that 1mu=1pt when they're mixed.");
    error();
}

/*
The next routine `|scan_something_internal|' is used to fetch internal
numeric quantities like `\.{\\hsize}', and also to handle the `\.{\\the}'
when expanding constructions like `\.{\\the\\toks0}' and
`\.{\\the\\baselineskip}'. Soon we will be considering the |scan_int|
procedure, which calls |scan_something_internal|; on the other hand,
|scan_something_internal| also calls |scan_int|, for constructions like
`\.{\\catcode\`\\\$}' or `\.{\\fontdimen} \.3 \.{\\ff}'. So we
have to declare |scan_int| as a |forward| procedure. A few other
procedures are also declared at this point.

\TeX\ doesn't know exactly what to expect when
|scan_something_internal| begins.  For example, an integer or
dimension or glue value could occur immediately after `\.{\\hskip}';
and one can even say \.{\\the} with respect to token lists in
constructions like `\.{\\xdef\\o\{\\the\\output\}}'.  On the other
hand, only integers are allowed after a construction like
`\.{\\count}'. To handle the various possibilities,
|scan_something_internal| has a |level| parameter, which tells the
``highest'' kind of quantity that |scan_something_internal| is allowed
to produce. Eight levels are distinguished, namely |int_val|,
|attr_val|, |dimen_val|, |glue_val|, |mu_val|, |dir_val|, |ident_val|,
and |tok_val|.

The output of |scan_something_internal| (and of the other routines
|scan_int|, |scan_dimen|, and |scan_glue| below) is put into the global
variable |cur_val|, and its level is put into |cur_val_level|. The highest
values of |cur_val_level| are special: |mu_val| is used only when
|cur_val| points to something in a ``muskip'' register, or to one of the
three parameters \.{\\thinmuskip}, \.{\\medmuskip}, \.{\\thickmuskip};
|ident_val| is used only when |cur_val| points to a font identifier;
|tok_val| is used only when |cur_val| points to |null| or to the reference
count of a token list. The last two cases are allowed only when
|scan_something_internal| is called with |level=tok_val|.

If the output is glue, |cur_val| will point to a glue specification, and
the reference count of that glue will have been updated to reflect this
reference; if the output is a nonempty token list, |cur_val| will point to
its reference count, but in this case the count will not have been updated.
Otherwise |cur_val| will contain the integer or scaled value in question.
*/

integer cur_val;                /* value returned by numeric scanners */
integer cur_val1;               /* delcodes are sometimes 51 digits */
int cur_val_level;              /* the ``level'' of this value */

#define scanned_result(A,B) do {		\
	cur_val=A;				\
	cur_val_level=B;			\
    } while (0)

/*
When a |glue_val| changes to a |dimen_val|, we use the width component
of the glue; there is no need to decrease the reference count, since it
has not yet been increased.  When a |dimen_val| changes to an |int_val|,
we use scaled points so that the value doesn't actually change. And when a
|mu_val| changes to a |glue_val|, the value doesn't change either.
*/


static void downgrade_cur_val(boolean delete_glue)
{
    halfword m;
    if (cur_val_level == glue_val_level) {
        m = cur_val;
        cur_val = width(m);
        if (delete_glue)
            delete_glue_ref(m);
    } else if (cur_val_level == mu_val_level) {
        mu_error();
    }
    decr(cur_val_level);
}

static void negate_cur_val(boolean delete_glue)
{
    halfword m;
    if (cur_val_level >= glue_val_level) {
        m = cur_val;
        cur_val = new_spec(m);
        if (delete_glue)
            delete_glue_ref(m);
        /* Negate all three glue components of |cur_val| */
        negate(width(cur_val));
        negate(stretch(cur_val));
        negate(shrink(cur_val));

    } else {
        negate(cur_val);
    }
}


/*  Some of the internal items can be fetched both routines,
    and these have been split off into the next routine, that
    returns true if the command code was understood 
*/


static boolean short_scan_something_internal(int cmd, int chr, int level,
                                             boolean negative)
{
    halfword m;                 /* |chr_code| part of the operand token */
    halfword q;                 /* general purpose index */
    int p;                      /* index into |nest| */
    integer save_cur_chr;
    boolean succeeded = true;
    m = chr;
    switch (cmd) {
    case assign_toks_cmd:
        scanned_result(equiv(m), tok_val_level);
        break;
    case assign_int_cmd:
        scanned_result(zeqtb[m].cint, int_val_level);
        break;
    case assign_attr_cmd:
        scanned_result(zeqtb[m].cint, int_val_level);
        break;
    case assign_dir_cmd:
        scanned_result(zeqtb[m].cint, dir_val_level);
        break;
    case assign_dimen_cmd:
        scanned_result(zeqtb[m].cint, dimen_val_level);
        break;
    case assign_glue_cmd:
        scanned_result(equiv(m), glue_val_level);
        break;
    case assign_mu_glue_cmd:
        scanned_result(equiv(m), mu_val_level);
        break;
    case math_style_cmd:
        scanned_result(m, int_val_level);
        break;
    case set_aux_cmd:
        /* Fetch the |space_factor| or the |prev_depth| */
        if (abs(cur_list.mode_field) != m) {
            print_err("Improper ");
            print_cmd_chr(set_aux_cmd, m);
            help4("You can refer to \\spacefactor only in horizontal mode;",
                  "you can refer to \\prevdepth only in vertical mode; and",
                  "neither of these is meaningful inside \\write. So",
                  "I'm forgetting what you said and using zero instead.");
            error();
            if (level != tok_val_level)
                scanned_result(0, dimen_val_level);
            else
                scanned_result(0, int_val_level);
        } else if (m == vmode) {
            scanned_result(prev_depth, dimen_val_level);
        } else {
            scanned_result(space_factor, int_val_level);
        }
        break;
    case set_prev_graf_cmd:
        /* Fetch the |prev_graf| */
        if (cur_list.mode_field == 0) {
            scanned_result(0, int_val_level);   /* |prev_graf=0| within \.{\\write} */
        } else {
            nest[nest_ptr] = cur_list;
            p = nest_ptr;
            while (abs(nest[p].mode_field) != vmode)
                decr(p);
            scanned_result(nest[p].pg_field, int_val_level);
        }
        break;
    case set_page_int_cmd:
        /* Fetch the |dead_cycles| or the |insert_penalties| */
        if (m == 0)
            cur_val = dead_cycles;
        else if (m == 2)
            cur_val = interaction;      /* interactionmode */
        else
            cur_val = insert_penalties;
        cur_val_level = int_val_level;
        break;
    case set_page_dimen_cmd:
        /* Fetch something on the |page_so_far| */
        if ((page_contents == empty) && (!output_active)) {
            if (m == 0)
                cur_val = max_dimen;
            else
                cur_val = 0;
        } else {
            cur_val = page_so_far[m];
        }
        cur_val_level = dimen_val_level;
        break;
    case set_tex_shape_cmd:
        /* Fetch the |par_shape| size */
        if (par_shape_ptr == null)
            cur_val = 0;
        else
            cur_val = vinfo(par_shape_ptr + 1);
        cur_val_level = int_val_level;
        break;
    case set_etex_shape_cmd:
        /* Fetch a penalties array element */
        scan_int();
        if ((equiv(m) == null) || (cur_val < 0)) {
            cur_val = 0;
        } else {
            if (cur_val > penalty(equiv(m)))
                cur_val = penalty(equiv(m));
            cur_val = penalty(equiv(m) + cur_val);
        }
        cur_val_level = int_val_level;
        break;
    case char_given_cmd:
    case math_given_cmd:
    case omath_given_cmd:
    case xmath_given_cmd:
        scanned_result(cur_chr, int_val_level);
        break;
    case last_item_cmd:
        /* Because the items in this case directly refer to |cur_chr|,
           it needs to be saved and restored */
        save_cur_chr = cur_chr;
        cur_chr = chr;
        /* Fetch an item in the current node, if appropriate */
        /* Here is where \.{\\lastpenalty}, \.{\\lastkern}, and \.{\\lastskip} are
           implemented. The reference count for \.{\\lastskip} will be updated later. 

           We also handle \.{\\inputlineno} and \.{\\badness} here, because they are
           legal in similar contexts. */

        if (m >= last_item_input_line_no_code) {
            if (m >= last_item_eTeX_glue) {
                /* Process an expression and |return| */
                if (m < last_item_eTeX_mu) {
                    switch (m) {
                    case last_item_mu_to_glue_code:
                        scan_mu_glue();
                        break;
                    };          /* there are no other cases */
                    cur_val_level = glue_val_level;
                } else if (m < last_item_eTeX_expr) {
                    switch (m) {
                    case last_item_glue_to_mu_code:
                        scan_normal_glue();
                        break;
                    }           /* there are no other cases */
                    cur_val_level = mu_val_level;
                } else {
                    cur_val_level = m - last_item_eTeX_expr + int_val_level;
                    scan_expr();
                }
                /* This code for reducing |cur_val_level| and\slash or negating the
                   result is similar to the one for all the other cases of
                   |scan_something_internal|, with the difference that |scan_expr| has
                   already increased the reference count of a glue specification. 
                 */
                while (cur_val_level > level) {
                    downgrade_cur_val(true);
                }
                if (negative) {
                    negate_cur_val(true);
                }
                return succeeded;

            } else if (m >= last_item_eTeX_dim) {
                switch (m) {
                case last_item_font_char_wd_code:
                case last_item_font_char_ht_code:
                case last_item_font_char_dp_code:
                case last_item_font_char_ic_code:
                    scan_font_ident();
                    q = cur_val;
                    scan_char_num();
                    if (char_exists(q, cur_val)) {
                        switch (m) {
                        case last_item_font_char_wd_code:
                            cur_val = char_width(q, cur_val);
                            break;
                        case last_item_font_char_ht_code:
                            cur_val = char_height(q, cur_val);
                            break;
                        case last_item_font_char_dp_code:
                            cur_val = char_depth(q, cur_val);
                            break;
                        case last_item_font_char_ic_code:
                            cur_val = char_italic(q, cur_val);
                            break;
                        }       /* there are no other cases */
                    } else {
                        cur_val = 0;
                    }
                    break;
                case last_item_par_shape_length_code:
                case last_item_par_shape_indent_code:
                case last_item_par_shape_dimen_code:
                    q = cur_chr - last_item_par_shape_length_code;
                    scan_int();
                    if ((par_shape_ptr == null) || (cur_val <= 0)) {
                        cur_val = 0;
                    } else {
                        if (q == 2) {
                            q = cur_val % 2;
                            cur_val = (cur_val + q) / 2;
                        }
                        if (cur_val > vinfo(par_shape_ptr + 1))
                            cur_val = vinfo(par_shape_ptr + 1);
                        cur_val =
                            varmem[par_shape_ptr + 2 * cur_val - q + 1].cint;
                    }
                    cur_val_level = dimen_val_level;
                    break;
                case last_item_glue_stretch_code:
                case last_item_glue_shrink_code:
                    scan_normal_glue();
                    q = cur_val;
                    if (m == last_item_glue_stretch_code)
                        cur_val = stretch(q);
                    else
                        cur_val = shrink(q);
                    delete_glue_ref(q);
                    break;
                }               /* there are no other cases */
                cur_val_level = dimen_val_level;
            } else {
                switch (m) {
                case last_item_input_line_no_code:
                    cur_val = line;
                    break;
                case last_item_badness_code:
                    cur_val = last_badness;
                    break;
                case last_item_pdftex_version_code:
                    cur_val = get_pdftexversion();
                    break;
                case last_item_luatex_version_code:
                    cur_val = get_luatexversion();
                    break;
                case last_item_pdf_last_obj_code:
                    cur_val = pdf_last_obj;
                    break;
                case last_item_pdf_last_xform_code:
                    cur_val = pdf_last_xform;
                    break;
                case last_item_pdf_last_ximage_code:
                    cur_val = pdf_last_ximage;
                    break;
                case last_item_pdf_last_ximage_pages_code:
                    cur_val = pdf_last_ximage_pages;
                    break;
                case last_item_pdf_last_annot_code:
                    cur_val = pdf_last_annot;
                    break;
                case last_item_pdf_last_x_pos_code:
                    cur_val = pdf_last_x_pos;
                    break;
                case last_item_pdf_last_y_pos_code:
                    cur_val = pdf_last_y_pos;
                    break;
                case last_item_pdf_retval_code:
                    cur_val = pdf_retval;
                    break;
                case last_item_pdf_last_ximage_colordepth_code:
                    cur_val = pdf_last_ximage_colordepth;
                    break;
                case last_item_random_seed_code:
                    cur_val = random_seed;
                    break;
                case last_item_pdf_last_link_code:
                    cur_val = pdf_last_link;
                    break;
                case last_item_Aleph_version_code:
                    cur_val = get_Aleph_version();
                    break;
                case last_item_Omega_version_code:
                    cur_val = get_Omega_version();
                    break;
                case last_item_eTeX_version_code:
                    cur_val = get_eTeX_version();
                    break;
                case last_item_Aleph_minor_version_code:
                    cur_val = get_Aleph_minor_version();
                    break;
                case last_item_Omega_minor_version_code:
                    cur_val = get_Omega_minor_version();
                    break;
                case last_item_eTeX_minor_version_code:
                    cur_val = get_eTeX_minor_version();
                    break;
                case last_item_current_group_level_code:
                    cur_val = cur_level - level_one;
                    break;
                case last_item_current_group_type_code:
                    cur_val = cur_group;
                    break;
                case last_item_current_if_level_code:
                    q = cond_ptr;
                    cur_val = 0;
                    while (q != null) {
                        incr(cur_val);
                        q = vlink(q);
                    }
                    break;
                case last_item_current_if_type_code:
                    if (cond_ptr == null)
                        cur_val = 0;
                    else if (cur_if < unless_code)
                        cur_val = cur_if + 1;
                    else
                        cur_val = -(cur_if - unless_code + 1);
                    break;
                case last_item_current_if_branch_code:
                    if ((if_limit == or_code) || (if_limit == else_code))
                        cur_val = 1;
                    else if (if_limit == fi_code)
                        cur_val = -1;
                    else
                        cur_val = 0;
                    break;
                case last_item_glue_stretch_order_code:
                case last_item_glue_shrink_order_code:
                    scan_normal_glue();
                    q = cur_val;
                    if (m == last_item_glue_stretch_order_code)
                        cur_val = stretch_order(q);
                    else
                        cur_val = shrink_order(q);
                    delete_glue_ref(q);
                    break;

                }               /* there are no other cases */
                cur_val_level = int_val_level;
            }
        } else {
            if (cur_chr == glue_val_level)
                cur_val = zero_glue;
            else
                cur_val = 0;
            if (cur_chr == last_item_last_node_type_code) {
                cur_val_level = int_val_level;
                if ((cur_list.tail_field == cur_list.head_field)
                    || (cur_list.mode_field == 0))
                    cur_val = -1;
            } else {
                cur_val_level = cur_chr;        /* assumes identical values */
            }
            if ((cur_list.tail_field != contrib_head) &&
                !is_char_node(cur_list.tail_field) &&
                (cur_list.mode_field != 0)) {
                switch (cur_chr) {
                case last_item_lastpenalty_code:
                    if (type(cur_list.tail_field) == penalty_node)
                        cur_val = penalty(cur_list.tail_field);
                    break;
                case last_item_lastkern_code:
                    if (type(cur_list.tail_field) == kern_node)
                        cur_val = width(cur_list.tail_field);
                    break;
                case last_item_lastskip_code:
                    if (type(cur_list.tail_field) == glue_node)
                        cur_val = glue_ptr(cur_list.tail_field);
                    if (subtype(cur_list.tail_field) == mu_glue)
                        cur_val_level = mu_val_level;
                    break;
                case last_item_last_node_type_code:
                    cur_val = visible_last_node_type(cur_list.tail_field);
                    break;
                }               /* there are no other cases */
            } else if ((cur_list.mode_field == vmode)
                       && (cur_list.tail_field == cur_list.head_field)) {
                switch (cur_chr) {
                case last_item_lastpenalty_code:
                    cur_val = last_penalty;
                    break;
                case last_item_lastkern_code:
                    cur_val = last_kern;
                    break;
                case last_item_lastskip_code:
                    if (last_glue != max_halfword)
                        cur_val = last_glue;
                    break;
                case last_item_last_node_type_code:
                    cur_val = last_node_type;
                    break;
                }               /* there are no other cases */
            }
        }
        cur_chr = save_cur_chr;
        break;
    default:
        succeeded = false;
    }
    if (succeeded) {
        while (cur_val_level > level) {
            /* Convert |cur_val| to a lower level */
            downgrade_cur_val(false);
        }
        /* Fix the reference count, if any, and negate |cur_val| if |negative| */
        /* If |cur_val| points to a glue specification at this point, the reference
           count for the glue does not yet include the reference by |cur_val|.
           If |negative| is |true|, |cur_val_level| is known to be |<=mu_val|.
         */
        if (negative) {
            negate_cur_val(false);
        } else if ((cur_val_level >= glue_val_level)
                   && (cur_val_level <= mu_val_level)) {
            add_glue_ref(cur_val);
        }
    }
    return succeeded;
}

/* First, here is a short routine that is called from lua code. All
the  real work is delegated to |short_scan_something_internal| that
is shared between this routine and |scan_something_internal|. 
*/

void scan_something_simple(halfword cmd, halfword subitem)
{
    /* negative is never true */
    if (!short_scan_something_internal(cmd, subitem, tok_val_level, false)) {
        /* Complain that |texlib| can not do this; give zero result */
        print_err("You can't use `");
        print_cmd_chr(cmd, subitem);
        tprint("' as tex library index");
        help1("I'm forgetting what you said and using zero instead.");
        error();
        scanned_result(0, int_val_level);
    }
}


/*
OK, we're ready for |scan_something_internal| itself. A second parameter,
|negative|, is set |true| if the value that is found should be negated.
It is assumed that |cur_cmd| and |cur_chr| represent the first token of
the internal quantity to be scanned; an error will be signalled if
|cur_cmd<min_internal| or |cur_cmd>max_internal|.
*/


void scan_something_internal(int level, boolean negative)
{
    /* fetch an internal parameter */
    halfword m;                 /* |chr_code| part of the operand token */
    integer n, k;               /* accumulators */
  RESTART:
    m = cur_chr;
    if (!short_scan_something_internal(cur_cmd, cur_chr, level, negative)) {
        switch (cur_cmd) {
        case def_char_code_cmd:
            /* Fetch a character code from some table */
            scan_char_num();
            if (m == math_code_base) {
                cur_val1 = get_math_code_num(cur_val);
                scanned_result(cur_val1, int_val_level);
            } else if (m == lc_code_base) {
                cur_val1 = get_lc_code(cur_val);
                scanned_result(cur_val1, int_val_level);
            } else if (m == uc_code_base) {
                cur_val1 = get_uc_code(cur_val);
                scanned_result(cur_val1, int_val_level);
            } else if (m == sf_code_base) {
                cur_val1 = get_sf_code(cur_val);
                scanned_result(cur_val1, int_val_level);
            } else if (m == cat_code_base) {
                cur_val1 =
                    get_cat_code(int_par(param_cat_code_table_code), cur_val);
                scanned_result(cur_val1, int_val_level);
            } else {
                confusion("def_char");
            }
            break;
        case def_del_code_cmd:
            /* Fetch a character code from some table */
            scan_char_num();
            cur_val1 = get_del_code_num(cur_val);
            scanned_result(cur_val1, int_val_level);                
            break;
        case extdef_math_code_cmd:
            /* Fetch an extended math code table value */
            scan_char_num();
            cur_val1 = get_math_code_num(cur_val);
            scanned_result(cur_val1, int_val_level);
            break;
        case toks_register_cmd:
        case set_font_cmd:
        case def_font_cmd:
        case letterspace_font_cmd:
        case pdf_copy_font_cmd:
            /* Fetch a token list or font identifier, provided that |level=tok_val| */
            if (level != tok_val_level) {
                print_err("Missing number, treated as zero");
                help3("A number should have been here; I inserted `0'.",
                      "(If you can't figure out why I needed to see a number,",
                      "look up `weird error' in the index to The TeXbook.)");
                back_error();
                scanned_result(0, dimen_val_level);
            } else if (cur_cmd == toks_register_cmd) {
                scan_register_num();
                m = get_toks_base() + cur_val;
                scanned_result(equiv(m), tok_val_level);
            } else {
                back_input();
                scan_font_ident();
                scanned_result(get_font_id_base() + cur_val, ident_val_level);
            }
            break;
        case def_family_cmd:
            /* Fetch a math font identifier */
            scan_char_num();
            cur_val1 = fam_fnt(cur_val, m);
            scanned_result(get_font_id_base() + cur_val1, ident_val_level);
            break;
        case set_math_param_cmd:
            /* Fetch a math param */
            cur_val1 = cur_chr;
            get_token();
            if (cur_cmd != math_style_cmd) {
                print_err("Missing math style, treated as \\displaystyle");
                help1
                    ("A style should have been here; I inserted `\\displaystyle'.");
                cur_val = display_style;
                back_error();
            } else {
                cur_val = cur_chr;
            }
            if (cur_val1 < math_param_first_mu_glue) {
                if (cur_val1 == math_param_radical_degree_raise) {
                    cur_val1 = get_math_param(cur_val1, cur_chr);
                    scanned_result(cur_val1, int_val_level);
                } else {
                    cur_val1 = get_math_param(cur_val1, cur_chr);
                    scanned_result(cur_val1, dimen_val_level);
                }
            } else {
                cur_val1 = get_math_param(cur_val1, cur_chr);
                if (cur_val1 == param_thin_mu_skip_code)
                    cur_val1 = glue_par(param_thin_mu_skip_code);
                else if (cur_val1 == param_med_mu_skip_code)
                    cur_val1 = glue_par(param_med_mu_skip_code);
                else if (cur_val1 == param_thick_mu_skip_code)
                    cur_val1 = glue_par(param_thick_mu_skip_code);
                scanned_result(cur_val1, mu_val_level);
            }
            break;
        case assign_box_dir_cmd:
            scan_register_num();
            m = cur_val;
            if (box(m) != null)
                cur_val = box_dir(box(m));
            else
                cur_val = 0;
            cur_val_level = dir_val_level;
            break;
        case set_box_dimen_cmd:
            /* Fetch a box dimension */
            scan_register_num();
            if (box(cur_val) == null)
                cur_val = 0;
            else
                cur_val = varmem[box(cur_val) + m].cint;
            cur_val_level = dimen_val_level;
            break;
        case assign_font_dimen_cmd:
            /* Fetch a font dimension */
            get_font_dimen();
            break;
        case assign_font_int_cmd:
            /* Fetch a font integer */
            scan_font_ident();
            if (m == 0) {
                scanned_result(hyphen_char(cur_val), int_val_level);
            } else if (m == 1) {
                scanned_result(skew_char(cur_val), int_val_level);
            } else if (m == no_lig_code) {
                scanned_result(test_no_ligatures(cur_val), int_val_level);
            } else {
                n = cur_val;
                scan_char_num();
                k = cur_val;
                switch (m) {
                case lp_code_base:
                    scanned_result(get_lp_code(n, k), int_val_level);
                    break;
                case rp_code_base:
                    scanned_result(get_rp_code(n, k), int_val_level);
                    break;
                case ef_code_base:
                    scanned_result(get_ef_code(n, k), int_val_level);
                    break;
                case tag_code:
                    scanned_result(get_tag_code(n, k), int_val_level);
                    break;
                }
            }
            break;
        case register_cmd:
            /* Fetch a register */
            scan_register_num();
            switch (m) {
            case int_val_level:
                cur_val = count(cur_val);
                break;
            case attr_val_level:
                cur_val = attribute(cur_val);
                break;
            case dimen_val_level:
                cur_val = dimen(cur_val);
                break;
            case glue_val_level:
                cur_val = skip(cur_val);
                break;
            case mu_val_level:
                cur_val = mu_skip(cur_val);
                break;
            }                   /* there are no other cases */
            cur_val_level = m;
            break;
        case ignore_spaces_cmd:        /* trap unexpandable primitives */
            if (cur_chr == 1) {
                /* Reset |cur_tok| for unexpandable primitives, goto restart */
                /* This block deals with unexpandable \.{\\primitive} appearing at a spot where
                   an integer or an internal values should have been found. It fetches the
                   next token then resets |cur_cmd|, |cur_cs|, and |cur_tok|, based on the
                   primitive value of that token. No expansion takes place, because the
                   next token may be all sorts of things. This could trigger further
                   expansion creating new errors.
                 */
                get_token();
                cur_cs = prim_lookup(text(cur_cs));
                if (cur_cs != undefined_primitive) {
                    cur_cmd = get_prim_eq_type(cur_cs);
                    cur_chr = get_prim_equiv(cur_cs);
                    cur_tok = (cur_cmd * STRING_OFFSET) + cur_chr;
                } else {
                    cur_cmd = relax_cmd;
                    cur_chr = 0;
                    cur_tok = cs_token_flag + frozen_relax;
                    cur_cs = frozen_relax;
                }
                goto RESTART;
            }
            break;
        default:
            /* Complain that \.{\\the} can not do this; give zero result */
            print_err("You can't use `");
            print_cmd_chr(cur_cmd, cur_chr);
            tprint("' after \\the");
            help1("I'm forgetting what you said and using zero instead.");
            error();
            if (level != tok_val_level)
                scanned_result(0, dimen_val_level);
            else
                scanned_result(0, int_val_level);
            break;
        }
        while (cur_val_level > level) {
            /* Convert |cur_val| to a lower level */
            downgrade_cur_val(false);
        }
        /* Fix the reference count, if any, and negate |cur_val| if |negative| */
        /* If |cur_val| points to a glue specification at this point, the reference
           count for the glue does not yet include the reference by |cur_val|.
           If |negative| is |true|, |cur_val_level| is known to be |<=mu_val|.
         */
        if (negative) {
            negate_cur_val(false);
        } else if ((cur_val_level >= glue_val_level) &&
                   (cur_val_level <= mu_val_level)) {
            add_glue_ref(cur_val);
        }
    }
}


/*
It is nice to have routines that say what they do, so the original
|scan_eight_bit_int| is superceded by |scan_register_num| and
|scan_mark_num|. It may become split up even further in the future.
*/

/* Many of the |restricted classes| routines are the essentially 
the same except for the upper limit and the error message, so it makes
sense to combine these all into one function. 
*/

void scan_limited_int(int max, char *name)
{
    char hlp[80];
    scan_int();
    if ((cur_val < 0) || (cur_val > max)) {
        if (name == NULL) {
            snprintf(hlp, 80,
                     "Since I expected to read a number between 0 and %d,",
                     max);
            print_err("Bad number");
        } else {
            char msg[80];
            snprintf(hlp, 80, "A %s must be between 0 and %d.", name, max);
            snprintf(msg, 80, "Bad %s", name);
            print_err(msg);
        }
        help2(hlp, "I changed this one to zero.");
        int_error(cur_val);
        cur_val = 0;
    }
}

void scan_fifteen_bit_int(void)
{
    scan_real_fifteen_bit_int();
    cur_val = ((cur_val / 0x1000) * 0x1000000) +
        (((cur_val % 0x1000) / 0x100) * 0x10000) + (cur_val % 0x100);
}

void scan_fifty_one_bit_int(void)
{
    integer iiii;
    scan_int();
    if ((cur_val < 0) || (cur_val > 0777777777)) {
        print_err("Bad delimiter code");
        help2
            ("A numeric delimiter (first part) must be between 0 and 2^{27}-1.",
             "I changed this one to zero.");
        int_error(cur_val);
        cur_val = 0;
    }
    iiii = cur_val;
    scan_int();
    if ((cur_val < 0) || (cur_val > 0xFFFFFF)) {
        print_err("Bad delimiter code");
        help2
            ("A numeric delimiter (second part) must be between 0 and 2^{24}-1.",
             "I changed this one to zero.");
        int_error(cur_val);
        cur_val = 0;
    }
    cur_val1 = cur_val;
    cur_val = iiii;
}

/*
To be able to determine whether \.{\\write18} is enabled from within
\TeX\ we also implement \.{\\eof18}.  We sort of cheat by having an
additional route |scan_four_bit_int_or_18| which is the same as
|scan_four_bit_int| except it also accepts the value 18.
*/

void scan_four_bit_int_or_18(void)
{
    scan_int();
    if ((cur_val < 0) || ((cur_val > 15) && (cur_val != 18))) {
        print_err("Bad number");
        help2("Since I expected to read a number between 0 and 15,",
              "I changed this one to zero.");
        int_error(cur_val);
        cur_val = 0;
    }
}



void scan_string_argument(void)
{
    integer s;
    scan_left_brace();
    get_x_token();
    while ((cur_cmd != right_brace_cmd)) {
        if ((cur_cmd == letter_cmd) || (cur_cmd == other_char_cmd)) {
            str_room(1);
            append_char(cur_chr);
        } else if (cur_cmd == spacer_cmd) {
            str_room(1);
            append_char(' ');
        } else {
            tprint("Bad token appearing in string argument");
        }
        get_x_token();
    }
    s = make_string();
    /* todo: this was just conserving the string pool: */
    /* 
       if (str_eq_str("mi",s)) s="mi";
       if (str_eq_str("mo",s)) s="mo";
       if (str_eq_str("mn",s)) s="mn";
     */
    cur_val = s;
}

/*
An integer number can be preceded by any number of spaces and `\.+' or
`\.-' signs. Then comes either a decimal constant (i.e., radix 10), an
octal constant (i.e., radix 8, preceded by~\.\'), a hexadecimal constant
(radix 16, preceded by~\."), an alphabetic constant (preceded by~\.\`), or
an internal variable. After scanning is complete,
|cur_val| will contain the answer, which must be at most
$2^{31}-1=2147483647$ in absolute value. The value of |radix| is set to
10, 8, or 16 in the cases of decimal, octal, or hexadecimal constants,
otherwise |radix| is set to zero. An optional space follows a constant.
*/


int radix;                      /* |scan_int| sets this to 8, 10, 16, or zero */

/*
  The |scan_int| routine is used also to scan the integer part of a
  fraction; for example, the `\.3' in `\.{3.14159}' will be found by
  |scan_int|. The |scan_dimen| routine assumes that |cur_tok=point_token|
  after the integer part of such a fraction has been scanned by |scan_int|,
  and that the decimal point has been backed up to be scanned again.
*/

void scan_int(void)
{                               /* sets |cur_val| to an integer */
    boolean negative;           /* should the answer be negated? */
    integer m;                  /* |$2^{31}$ / radix|, the threshold of danger */
    int d;                      /* the digit just scanned */
    boolean vacuous;            /* have no digits appeared? */
    boolean OK_so_far;          /* has an error message been issued? */
    radix = 0;
    OK_so_far = true;
    /* Get the next non-blank non-sign token; set |negative| appropriately */
    negative = false;
    do {
        /* Get the next non-blank non-call token */
        do {
            get_x_token();
        } while (cur_cmd == spacer_cmd);
        if (cur_tok == other_token + '-') {
            negative = !negative;
            cur_tok = other_token + '+';
        }
    } while (cur_tok == other_token + '+');

  RESTART:
    if (cur_tok == alpha_token) {
        /* Scan an alphabetic character code into |cur_val| */
        /* A space is ignored after an alphabetic character constant, so that
           such constants behave like numeric ones. */
        get_token();            /* suppress macro expansion */
        if (cur_tok < cs_token_flag) {
            cur_val = cur_chr;
            if (cur_cmd <= right_brace_cmd) {
                if (cur_cmd == right_brace_cmd)
                    incr(align_state);
                else
                    decr(align_state);
            }
        } else {                /* the value of a csname in this context is its name */
            if (is_active_cs(text(cur_tok - cs_token_flag)))
                cur_val = active_cs_value(text(cur_tok - cs_token_flag));
            else if (single_letter(text(cur_tok - cs_token_flag)))
                cur_val =
                    pool_to_unichar(str_start_macro
                                    (text(cur_tok - cs_token_flag)));
            else
                cur_val = (biggest_char + 1);
        }
        if (cur_val > biggest_char) {
            print_err("Improper alphabetic constant");
            help2("A one-character control sequence belongs after a ` mark.",
                  "So I'm essentially inserting \\0 here.");
            cur_val = '0';
            back_error();
        } else {
            /* Scan an optional space */
            get_x_token();
            if (cur_cmd != spacer_cmd)
                back_input();
        }

    } else if (cur_tok == cs_token_flag + frozen_primitive) {
        /* Reset |cur_tok| for unexpandable primitives, goto restart */
        /* This block deals with unexpandable \.{\\primitive} appearing at a spot where
           an integer or an internal values should have been found. It fetches the
           next token then resets |cur_cmd|, |cur_cs|, and |cur_tok|, based on the
           primitive value of that token. No expansion takes place, because the
           next token may be all sorts of things. This could trigger further
           expansion creating new errors.
         */
        get_token();
        cur_cs = prim_lookup(text(cur_cs));
        if (cur_cs != undefined_primitive) {
            cur_cmd = get_prim_eq_type(cur_cs);
            cur_chr = get_prim_equiv(cur_cs);
            cur_tok = (cur_cmd * STRING_OFFSET) + cur_chr;
        } else {
            cur_cmd = relax_cmd;
            cur_chr = 0;
            cur_tok = cs_token_flag + frozen_relax;
            cur_cs = frozen_relax;
        }
        goto RESTART;
    } else if (cur_cmd == math_style_cmd) {
        cur_val = cur_chr;
    } else if ((cur_cmd >= min_internal_cmd) && (cur_cmd <= max_internal_cmd)) {
        scan_something_internal(int_val_level, false);
    } else {
        /* Scan a numeric constant */
        radix = 10;
        m = 214748364;
        if (cur_tok == octal_token) {
            radix = 8;
            m = 02000000000;
            get_x_token();
        } else if (cur_tok == hex_token) {
            radix = 16;
            m = 01000000000;
            get_x_token();
        }
        vacuous = true;
        cur_val = 0;
        /* Accumulate the constant until |cur_tok| is not a suitable digit */
        while (1) {
            if ((cur_tok < zero_token + radix) && (cur_tok >= zero_token)
                && (cur_tok <= zero_token + 9)) {
                d = cur_tok - zero_token;
            } else if (radix == 16) {
                if ((cur_tok <= A_token + 5) && (cur_tok >= A_token)) {
                    d = cur_tok - A_token + 10;
                } else if ((cur_tok <= other_A_token + 5)
                           && (cur_tok >= other_A_token)) {
                    d = cur_tok - other_A_token + 10;
                } else {
                    break;
                }
            } else {
                break;
            }
            vacuous = false;
            if ((cur_val >= m) && ((cur_val > m) || (d > 7) || (radix != 10))) {
                if (OK_so_far) {
                    print_err("Number too big");
                    help2
                        ("I can only go up to 2147483647='17777777777=\"7FFFFFFF,",
                         "so I'm using that number instead of yours.");
                    error();
                    cur_val = infinity;
                    OK_so_far = false;
                }
            } else {
                cur_val = cur_val * radix + d;
            }
            get_x_token();
        }
        if (vacuous) {
            /* Express astonishment that no number was here */
            print_err("Missing number, treated as zero");
            help3("A number should have been here; I inserted `0'.",
                  "(If you can't figure out why I needed to see a number,",
                  "look up `weird error' in the index to The TeXbook.)");
            back_error();
        } else if (cur_cmd != spacer_cmd) {
            back_input();
        }
    }
    if (negative)
        negate(cur_val);
}

/*
The following code is executed when |scan_something_internal| was
called asking for |mu_val|, when we really wanted a ``mudimen'' instead
of ``muglue.''
*/

static void coerce_glue(void)
{
    integer v;
    if (cur_val_level >= glue_val_level) {
        v = width(cur_val);
        delete_glue_ref(cur_val);
        cur_val = v;
    }
}


/*
The |scan_dimen| routine is similar to |scan_int|, but it sets |cur_val| to
a |scaled| value, i.e., an integral number of sp. One of its main tasks
is therefore to interpret the abbreviations for various kinds of units and
to convert measurements to scaled points.

There are three parameters: |mu| is |true| if the finite units must be
`\.{mu}', while |mu| is |false| if `\.{mu}' units are disallowed;
|inf| is |true| if the infinite units `\.{fil}', `\.{fill}', `\.{filll}'
are permitted; and |shortcut| is |true| if |cur_val| already contains
an integer and only the units need to be considered.

The order of infinity that was found in the case of infinite glue is returned
in the global variable |cur_order|.
*/

int cur_order;                  /* order of infinity found by |scan_dimen| */

/*
Constructions like `\.{-\'77 pt}' are legal dimensions, so |scan_dimen|
may begin with |scan_int|. This explains why it is convenient to use
|scan_int| also for the integer part of a decimal fraction.

Several branches of |scan_dimen| work with |cur_val| as an integer and
with an auxiliary fraction |f|, so that the actual quantity of interest is
$|cur_val|+|f|/2^{16}$. At the end of the routine, this ``unpacked''
representation is put into the single word |cur_val|, which suddenly
switches significance from |integer| to |scaled|.
*/

void scan_dimen(boolean mu, boolean inf, boolean shortcut)
/* sets |cur_val| to a dimension */
{
    boolean negative;           /* should the answer be negated? */
    integer f;                  /* numerator of a fraction whose denominator is $2^{16}$ */
    /* Local variables for dimension calculations */
    int num, denom;             /* conversion ratio for the scanned units */
    int k, kk;                  /* number of digits in a decimal fraction */
    halfword p, q;              /* top of decimal digit stack */
    scaled v;                   /* an internal dimension */
    integer save_cur_val;       /* temporary storage of |cur_val| */

    f = 0;
    arith_error = false;
    cur_order = normal;
    negative = false;
    if (!shortcut) {
        /* Get the next non-blank non-sign... */
        negative = false;
        do {
            /* Get the next non-blank non-call token */
            do {
                get_x_token();
            } while (cur_cmd == spacer_cmd);
            if (cur_tok == other_token + '-') {
                negative = !negative;
                cur_tok = other_token + '+';
            }
        } while (cur_tok == other_token + '+');

        if ((cur_cmd >= min_internal_cmd) && (cur_cmd <= max_internal_cmd)) {
            /* Fetch an internal dimension and |goto attach_sign|,
               or fetch an internal integer */
            if (mu) {
                scan_something_internal(mu_val_level, false);
                coerce_glue();
                if (cur_val_level == mu_val_level)
                    goto ATTACH_SIGN;
                if (cur_val_level != int_val_level)
                    mu_error();
            } else {
                scan_something_internal(dimen_val_level, false);
                if (cur_val_level == dimen_val_level)
                    goto ATTACH_SIGN;
            }

        } else {
            back_input();
            if (cur_tok == continental_point_token) {
                cur_tok = point_token;
            }
            if (cur_tok != point_token) {
                scan_int();
            } else {
                radix = 10;
                cur_val = 0;
            }
            if (cur_tok == continental_point_token)
                cur_tok = point_token;
            if ((radix == 10) && (cur_tok == point_token)) {
                /* Scan decimal fraction */
                /* When the following code is executed, we have |cur_tok=point_token|, but this
                   token has been backed up using |back_input|; we must first discard it.

                   It turns out that a decimal point all by itself is equivalent to `\.{0.0}'.
                   Let's hope people don't use that fact. */

                k = 0;
                p = null;
                get_token();    /* |point_token| is being re-scanned */
                while (1) {
                    get_x_token();
                    if ((cur_tok > zero_token + 9) || (cur_tok < zero_token))
                        break;
                    if (k < 17) {       /* digits for |k>=17| cannot affect the result */
                        q = get_avail();
                        set_token_link(q, p);
                        set_token_info(q, cur_tok - zero_token);
                        p = q;
                        incr(k);
                    }
                }
                for (kk = k; kk >= 1; kk--) {
                    dig[kk - 1] = token_info(p);
                    q = p;
                    p = token_link(p);
                    free_avail(q);
                }
                f = round_decimals(k);
                if (cur_cmd != spacer_cmd)
                    back_input();
            }
        }
    }
    if (cur_val < 0) {          /* in this case |f=0| */
        negative = !negative;
        negate(cur_val);
    }
    /* Scan units and set |cur_val| to $x\cdot(|cur_val|+f/2^{16})$, where there
       are |x| sp per unit; |goto attach_sign| if the units are internal */
    /* Now comes the harder part: At this point in the program, |cur_val| is a
       nonnegative integer and $f/2^{16}$ is a nonnegative fraction less than 1;
       we want to multiply the sum of these two quantities by the appropriate
       factor, based on the specified units, in order to produce a |scaled|
       result, and we want to do the calculation with fixed point arithmetic that
       does not overflow.
     */

    if (inf) {
        /* Scan for (f)\.{fil} units; |goto attach_fraction| if found */
        /* In traditional TeX, a specification like `\.{filllll}' or `\.{fill L L
           L}' will lead to two error messages (one for each additional keyword
           \.{"l"}). 
           Not so for luatex, it just parses the construct in reverse. */
        if (scan_keyword("filll")) {
            cur_order = filll;
            goto ATTACH_FRACTION;
        } else if (scan_keyword("fill")) {
            cur_order = fill;
            goto ATTACH_FRACTION;
        } else if (scan_keyword("fil")) {
            cur_order = fil;
            goto ATTACH_FRACTION;
        } else if (scan_keyword("fi")) {
            cur_order = sfi;
            goto ATTACH_FRACTION;
        }

    }
    /* Scan for (u)units that are internal dimensions;
       |goto attach_sign| with |cur_val| set if found */
    save_cur_val = cur_val;
    /* Get the next non-blank non-call... */
    do {
        get_x_token();
    } while (cur_cmd == spacer_cmd);

    if ((cur_cmd < min_internal_cmd) || (cur_cmd > max_internal_cmd)) {
        back_input();
    } else {
        if (mu) {
            scan_something_internal(mu_val_level, false);
            coerce_glue();
            if (cur_val_level != mu_val_level)
                mu_error();
        } else {
            scan_something_internal(dimen_val_level, false);
        }
        v = cur_val;
        goto FOUND;
    }
    if (mu)
        goto NOT_FOUND;
    if (scan_keyword("em")) {
        v = (quad(get_cur_font()));
    } else if (scan_keyword("ex")) {
        v = (x_height(get_cur_font()));
    } else if (scan_keyword("px")) {
        v = int_par(param_pdf_px_dimen_code);
    } else {
        goto NOT_FOUND;
    }
    /* Scan an optional space */
    get_x_token();
    if (cur_cmd != spacer_cmd)
        back_input();

  FOUND:
    cur_val = nx_plus_y(save_cur_val, v, xn_over_d(v, f, 0200000));
    goto ATTACH_SIGN;
  NOT_FOUND:

    if (mu) {
        /* Scan for (m)\.{mu} units and |goto attach_fraction| */
        if (scan_keyword("mu")) {
            goto ATTACH_FRACTION;
        } else {
            print_err("Illegal unit of measure (mu inserted)");
            help4("The unit of measurement in math glue must be mu.",
                  "To recover gracefully from this error, it's best to",
                  "delete the erroneous units; e.g., type `2' to delete",
                  "two letters. (See Chapter 27 of The TeXbook.)");
            error();
            goto ATTACH_FRACTION;
        }
    }
    if (scan_keyword("true")) {
        /* Adjust (f)for the magnification ratio */
        prepare_mag();
        if (int_par(param_mag_code) != 1000) {
            cur_val = xn_over_d(cur_val, 1000, int_par(param_mag_code));
            f = (1000 * f + 0200000 * tex_remainder) / int_par(param_mag_code);
            cur_val = cur_val + (f / 0200000);
            f = f % 0200000;
        }
    }
    if (scan_keyword("pt"))
        goto ATTACH_FRACTION;   /* the easy case */
    /* Scan for (a)all other units and adjust |cur_val| and |f| accordingly;
       |goto done| in the case of scaled points */

    /* The necessary conversion factors can all be specified exactly as
       fractions whose numerator and denominator add to 32768 or less.
       According to the definitions here, $\rm2660\,dd\approx1000.33297\,mm$;
       this agrees well with the value $\rm1000.333\,mm$ cited by Bosshard
       @^Bosshard, Hans Rudolf@>
       in {\sl Technische Grundlagen zur Satzherstellung\/} (Bern, 1980).
       The Didot point has been newly standardized in 1978;
       it's now exactly $\rm 1\,nd=0.375\,mm$.
       Conversion uses the equation $0.375=21681/20320/72.27\cdot25.4$.
       The new Cicero follows the new Didot point; $\rm 1\,nc=12\,nd$.
       These would lead to the ratios $21681/20320$ and $65043/5080$,
       respectively.
       The closest approximations supported by the algorithm would be
       $11183/10481$ and $1370/107$.  In order to maintain the
       relation $\rm 1\,nc=12\,nd$, we pick the ratio $685/642$ for
       $\rm nd$, however.
     */

#define set_conversion(A,B) do { num=(A); denom=(B); } while(0)

    if (scan_keyword("in")) {
        set_conversion(7227, 100);
    } else if (scan_keyword("pc")) {
        set_conversion(12, 1);
    } else if (scan_keyword("cm")) {
        set_conversion(7227, 254);
    } else if (scan_keyword("mm")) {
        set_conversion(7227, 2540);
    } else if (scan_keyword("bp")) {
        set_conversion(7227, 7200);
    } else if (scan_keyword("dd")) {
        set_conversion(1238, 1157);
    } else if (scan_keyword("cc")) {
        set_conversion(14856, 1157);
    } else if (scan_keyword("nd")) {
        set_conversion(685, 642);
    } else if (scan_keyword("nc")) {
        set_conversion(1370, 107);
    } else if (scan_keyword("sp")) {
        goto DONE;
    } else {
        /* Complain about unknown unit and |goto done2| */
        print_err("Illegal unit of measure (pt inserted)");
        help6("Dimensions can be in units of em, ex, in, pt, pc,",
              "cm, mm, dd, cc, nd, nc, bp, or sp; but yours is a new one!",
              "I'll assume that you meant to say pt, for printer's points.",
              "To recover gracefully from this error, it's best to",
              "delete the erroneous units; e.g., type `2' to delete",
              "two letters. (See Chapter 27 of The TeXbook.)");
        error();
        goto DONE2;
    }
    cur_val = xn_over_d(cur_val, num, denom);
    f = (num * f + 0200000 * tex_remainder) / denom;
    cur_val = cur_val + (f / 0200000);
    f = f % 0200000;
  DONE2:
  ATTACH_FRACTION:
    if (cur_val >= 040000)
        arith_error = true;
    else
        cur_val = cur_val * unity + f;
  DONE:
    /* Scan an optional space */
    get_x_token();
    if (cur_cmd != spacer_cmd)
        back_input();
  ATTACH_SIGN:
    if (arith_error || (abs(cur_val) >= 010000000000)) {
        /* Report that this dimension is out of range */
        print_err("Dimension too large");
        help2("I can't work with sizes bigger than about 19 feet.",
              "Continue and I'll use the largest value I can.");
        error();
        cur_val = max_dimen;
        arith_error = false;
    }
    if (negative)
        negate(cur_val);
}


/*
The final member of \TeX's value-scanning trio is |scan_glue|, which
makes |cur_val| point to a glue specification. The reference count of that
glue spec will take account of the fact that |cur_val| is pointing to~it.

The |level| parameter should be either |glue_val| or |mu_val|.

Since |scan_dimen| was so much more complex than |scan_int|, we might expect
|scan_glue| to be even worse. But fortunately, it is very simple, since
most of the work has already been done.
*/

void scan_glue(int level)
{                               /* sets |cur_val| to a glue spec pointer */
    boolean negative;           /* should the answer be negated? */
    halfword q;                 /* new glue specification */
    boolean mu;                 /* does |level=mu_val|? */
    mu = (level == mu_val_level);
    /* Get the next non-blank non-sign... */
    negative = false;
    do {
        /* Get the next non-blank non-call token */
        do {
            get_x_token();
        } while (cur_cmd == spacer_cmd);
        if (cur_tok == other_token + '-') {
            negative = !negative;
            cur_tok = other_token + '+';
        }
    } while (cur_tok == other_token + '+');

    if ((cur_cmd >= min_internal_cmd) && (cur_cmd <= max_internal_cmd)) {
        scan_something_internal(level, negative);
        if (cur_val_level >= glue_val_level) {
            if (cur_val_level != level)
                mu_error();
            return;
        }
        if (cur_val_level == int_val_level)
            scan_dimen(mu, false, true);
        else if (level == mu_val_level)
            mu_error();
    } else {
        back_input();
        scan_dimen(mu, false, false);
        if (negative)
            negate(cur_val);
    }
    /* Create a new glue specification whose width is |cur_val|; scan for its
       stretch and shrink components  */
    q = new_spec(zero_glue);
    width(q) = cur_val;
    if (scan_keyword("plus")) {
        scan_dimen(mu, true, false);
        stretch(q) = cur_val;
        stretch_order(q) = cur_order;
    }
    if (scan_keyword("minus")) {
        scan_dimen(mu, true, false);
        shrink(q) = cur_val;
        shrink_order(q) = cur_order;
    }
    cur_val = q;
}

/* This is an omega routine */

void scan_scaled(void)
{                               /* sets |cur_val| to a scaled value */
    boolean negative;           /* should the answer be negated? */
    integer f;                  /* numerator of a fraction whose denominator is $2^{16}$ */
    int k, kk;                  /* number of digits in a decimal fraction */
    halfword p, q;              /* top of decimal digit stack */
    f = 0;
    arith_error = false;
    negative = false;
    /* Get the next non-blank non-sign... */
    negative = false;
    do {
        /* Get the next non-blank non-call token */
        do {
            get_x_token();
        } while (cur_cmd == spacer_cmd);
        if (cur_tok == other_token + '-') {
            negative = !negative;
            cur_tok = other_token + '+';
        }
    } while (cur_tok == other_token + '+');

    back_input();
    if (cur_tok == continental_point_token)
        cur_tok = point_token;
    if (cur_tok != point_token) {
        scan_int();
    } else {
        radix = 10;
        cur_val = 0;
    }
    if (cur_tok == continental_point_token)
        cur_tok = point_token;
    if ((radix == 10) && (cur_tok == point_token)) {
        /* Scan decimal fraction */
        /* TODO: merge this with the same block in scan_dimen */
        /* When the following code is executed, we have |cur_tok=point_token|, but this
           token has been backed up using |back_input|; we must first discard it.

           It turns out that a decimal point all by itself is equivalent to `\.{0.0}'.
           Let's hope people don't use that fact. */

        k = 0;
        p = null;
        get_token();            /* |point_token| is being re-scanned */
        while (1) {
            get_x_token();
            if ((cur_tok > zero_token + 9) || (cur_tok < zero_token))
                break;
            if (k < 17) {       /* digits for |k>=17| cannot affect the result */
                q = get_avail();
                set_token_link(q, p);
                set_token_info(q, cur_tok - zero_token);
                p = q;
                incr(k);
            }
        }
        for (kk = k; kk >= 1; kk--) {
            dig[kk - 1] = token_info(p);
            q = p;
            p = token_link(p);
            free_avail(q);
        }
        f = round_decimals(k);
        if (cur_cmd != spacer_cmd)
            back_input();

    }
    if (cur_val < 0) {          /* in this case |f=0| */
        negative = !negative;
        negate(cur_val);
    }
    if (cur_val > 040000)
        arith_error = true;
    else
        cur_val = cur_val * unity + f;
    if (arith_error || (abs(cur_val) >= 010000000000)) {
        print_err("Stack number too large");
        error();
    }
    if (negative)
        negate(cur_val);
}

/*
This procedure is supposed to scan something like `\.{\\skip\\count12}',
i.e., whatever can follow `\.{\\the}', and it constructs a token list
containing something like `\.{-3.0pt minus 0.5fill}'.
*/

halfword the_toks(void)
{
    int old_setting;            /* holds |selector| setting */
    halfword p, q, r;           /* used for copying a token list */
    pool_pointer b;             /* base of temporary string */
    int c;                      /* value of |cur_chr| */
    /* Handle \.{\\unexpanded} or \.{\\detokenize} and |return| */
    if (odd(cur_chr)) {
        c = cur_chr;
        scan_general_text();
        if (c == 1) {
            return cur_val;
        } else {
            old_setting = selector;
            selector = new_string;
            b = pool_ptr;
            p = get_avail();
            set_token_link(p, token_link(temp_token_head));
            token_show(p);
            flush_list(p);
            selector = old_setting;
            return str_toks(b);
        }
    }
    get_x_token();
    scan_something_internal(tok_val_level, false);
    if (cur_val_level >= ident_val_level) {
        /* Copy the token list */
        p = temp_token_head;
        set_token_link(p, null);
        if (cur_val_level == ident_val_level) {
            store_new_token(cs_token_flag + cur_val);
        } else if (cur_val != null) {
            r = token_link(cur_val);    /* do not copy the reference count */
            while (r != null) {
                fast_store_new_token(token_info(r));
                r = token_link(r);
            }
        }
        return p;
    } else {
        old_setting = selector;
        selector = new_string;
        b = pool_ptr;
        switch (cur_val_level) {
        case int_val_level:
            print_int(cur_val);
            break;
        case attr_val_level:
            print_int(cur_val);
            break;
        case dir_val_level:
            print_dir(cur_val);
            break;
        case dimen_val_level:
            print_scaled(cur_val);
            tprint("pt");
            break;
        case glue_val_level:
            print_spec(cur_val, "pt");
            delete_glue_ref(cur_val);
            break;
        case mu_val_level:
            print_spec(cur_val, "mu");
            delete_glue_ref(cur_val);
            break;
        }                       /* there are no other cases */
        selector = old_setting;
        return str_toks(b);
    }
}

str_number the_scanned_result(void)
{
    int old_setting;            /* holds |selector| setting */
    str_number r;               /* return value * */
    old_setting = selector;
    selector = new_string;
    if (cur_val_level >= ident_val_level) {
        if (cur_val != null) {
            show_token_list(token_link(cur_val), null, pool_size - pool_ptr);
            r = make_string();
        } else {
            r = get_nullstr();
        }
    } else {
        switch (cur_val_level) {
        case int_val_level:
            print_int(cur_val);
            break;
        case attr_val_level:
            print_int(cur_val);
            break;
        case dir_val_level:
            print_dir(cur_val);
            break;
        case dimen_val_level:
            print_scaled(cur_val);
            tprint("pt");
            break;
        case glue_val_level:
            print_spec(cur_val, "pt");
            delete_glue_ref(cur_val);
            break;
        case mu_val_level:
            print_spec(cur_val, "mu");
            delete_glue_ref(cur_val);
            break;
        }                       /* there are no other cases */
        r = make_string();
    }
    selector = old_setting;
    return r;
}


/*
The following routine is used to implement `\.{\\fontdimen} |n| |f|'.
The boolean parameter |writing| is set |true| if the calling program
intends to change the parameter value.
*/

static void font_param_error(integer f)
{
    print_err("Font ");
    print_esc(font_id_text(f));
    tprint(" has only ");
    print_int(font_params(f));
    tprint(" fontdimen parameters");
    help2("To increase the number of font parameters, you must",
          "use \\fontdimen immediately after the \\font is loaded.");
    error();
}

void set_font_dimen(void)
{
    internal_font_number f;
    integer n;                  /* the parameter number */
    scan_int();
    n = cur_val;
    scan_font_ident();
    f = cur_val;
    if (n <= 0) {
        font_param_error(f);
    } else {
        if (n > font_params(f)) {
            if (font_touched(f)) {
                font_param_error(f);
            } else {
                /* Increase the number of parameters in the font */
                do {
                    set_font_param(f, (font_params(f) + 1), 0);
                } while (n != font_params(f));
            }
        }
    }
    scan_optional_equals();
    scan_normal_dimen();
    set_font_param(f, n, cur_val);
}

void get_font_dimen(void)
{
    internal_font_number f;
    integer n;                  /* the parameter number */
    scan_int();
    n = cur_val;
    scan_font_ident();
    f = cur_val;
    cur_val = 0;                /* initialize return value */
    if (n <= 0) {
        font_param_error(f);
        goto EXIT;
    } else {
        if (n > font_params(f)) {
            if (font_touched(f)) {
                font_param_error(f);
                goto EXIT;
            } else {
                /* Increase the number of parameters in the font */
                do {
                    set_font_param(f, (font_params(f) + 1), 0);
                } while (n != font_params(f));

            }
        }
    }
    cur_val = font_param(f, n);
  EXIT:
    scanned_result(cur_val, dimen_val_level);
}

/*
Here we declare two trivial procedures in order to avoid mutually
recursive procedures with parameters.
*/

void scan_normal_glue(void)
{
    scan_glue(glue_val_level);
}

void scan_mu_glue(void)
{
    scan_glue(mu_val_level);
}
