/* texmath.h
   
   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
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

/* $Id$ */

#ifndef TEXMATH_H
#  define TEXMATH_H 1

#  define empty 0

extern pointer new_noad(void);

extern void show_math_node(halfword);
extern void print_math_comp(halfword);
extern void print_limit_switch(halfword);
extern void print_style(halfword);
extern void print_size(halfword);
extern void flush_math(void);
extern void math_left_brace(void);
extern void finish_display_alignment(halfword, halfword, memory_word);
extern halfword new_sub_box(halfword);

#  define math_reset(p) do { if (p!=null) flush_node(p); p = null; } while (0)

#  define scripts_allowed(A) ((type((A))>=ord_noad)&&(type((A))<left_noad))

#  define default_code 010000000000     /* denotes |default_rule_thickness| */

#  define limits 1              /* |subtype| of |op_noad| whose scripts are to be above, below */
#  define no_limits 2           /* |subtype| of |op_noad| whose scripts are to be normal */

extern void initialize_math(void);
extern halfword math_vcenter_group(halfword);
extern void build_choices(void);
extern void close_math_group(halfword);
extern void init_math(void);
extern void start_eq_no(void);
extern void set_math_char(mathcodeval);
extern void math_math_comp(void);
extern void math_limit_switch(void);
extern void math_radical(void);
extern void math_ac(void);
extern pointer new_style(small_number);
extern void append_choices(void);
extern void sub_sup(void);
extern void math_fraction(void);
extern void math_left_right(void);
extern void after_math(void);

extern void scan_extdef_del_code (int level, int extcode);
extern void scan_extdef_math_code (int level, int extcode);

#  define total_mathsy_params 22
#  define total_mathex_params 13

extern pointer cur_mlist;
extern integer cur_style;
extern boolean mlist_penalties;
extern integer cur_size;

/*
@ Before an mlist is converted to an hlist, \TeX\ makes sure that
the fonts in family~2 have enough parameters to be math-symbol
fonts, and that the fonts in family~3 have enough parameters to be
math-extension fonts. The math-symbol parameters are referred to by using the
following macros, which take a size code as their parameter; for example,
|num1(cur_size)| gives the value of the |num1| parameter for the current size.
@^parameters for symbols@>
@^font parameters@>
*/


#  define mathsy_end(A)
#  define mathsy(A) font_param(fam_fnt(2+cur_size),A) mathsy_end
#  define math_x_height mathsy(5)
                                /* height of `\.x' */
#  define math_quad mathsy(6)   /* \.{18mu} */
#  define num1 mathsy(8)        /* numerator shift-up in display styles */
#  define num2 mathsy(9)        /* numerator shift-up in non-display, non-\.{\\atop} */
#  define num3 mathsy(10)       /* numerator shift-up in non-display \.{\\atop} */
#  define denom1 mathsy(11)     /* denominator shift-down in display styles */
#  define denom2 mathsy(12)     /* denominator shift-down in non-display styles */
#  define sup1 mathsy(13)       /* superscript shift-up in uncramped display style */
#  define sup2 mathsy(14)       /* superscript shift-up in uncramped non-display */
#  define sup3 mathsy(15)       /* superscript shift-up in cramped styles */
#  define sub1 mathsy(16)       /* subscript shift-down if superscript is absent */
#  define sub2 mathsy(17)       /* subscript shift-down if superscript is present */
#  define sup_drop mathsy(18)   /* superscript baseline below top of large box */
#  define sub_drop mathsy(19)   /* subscript baseline below bottom of large box */
#  define delim1 mathsy(20)     /* size of \.{\\atopwithdelims} delimiters in display styles */
#  define delim2 mathsy(21)     /* size of \.{\\atopwithdelims} delimiters in non-displays */
#  define axis_height mathsy(22)/* height of fraction lines above the baseline */

/*
The math-extension parameters have similar macros, but the size code is
omitted (since it is always |cur_size| when we refer to such parameters).
@^parameters for symbols@>
@^font parameters@>
*/

#  define mathex(A) font_param(fam_fnt(3+cur_size),A)
#  define default_rule_thickness mathex(8)      /* thickness of \.{\\over} bars */
#  define big_op_spacing1 mathex(9)     /* minimum clearance above a displayed op */
#  define big_op_spacing2 mathex(10)    /* minimum clearance below a displayed op */
#  define big_op_spacing3 mathex(11)    /* minimum baselineskip above displayed op */
#  define big_op_spacing4 mathex(12)    /* minimum baselineskip below displayed op */
#  define big_op_spacing5 mathex(13)    /* padding above and below displayed limits */


/*
  @ We also need to compute the change in style between mlists and their
  subsidiaries. The following macros define the subsidiary style for
  an overlined nucleus (|cramped_style|), for a subscript or a superscript
  (|sub_style| or |sup_style|), or for a numerator or denominator (|num_style|
  or |denom_style|).
*/

typedef enum {
  display_style = 0,       /* |subtype| for \.{\\displaystyle} */
  text_style = 2,          /* |subtype| for \.{\\textstyle} */
  script_style = 4,        /* |subtype| for \.{\\scriptstyle} */
  script_script_style = 6, /* |subtype| for \.{\\scriptscriptstyle} */
} math_style_subtypes;

extern const char *math_style_names[];

#  define cramped 1             /* add this to an uncramped style if you want to cramp it */


#  define cramped_style(A) 2*((A)/2)+cramped    /* cramp the style */
#  define sub_style(A) 2*((A)/4)+script_style+cramped   /* smaller and cramped */
#  define sup_style(A) 2*((A)/4)+script_style+((A)%2)   /* smaller */
#  define num_style(A) (A)+2-2*((A)/6)  /* smaller unless already script-script */
#  define denom_style(A) 2*((A)/2)+cramped+2-2*((A)/6)  /* smaller, cramped */

void mlist_to_hlist(void);

#  define text_size 0
#  define script_size 256
#  define script_script_size 512

#  define math_direction int_par(param_math_direction_code)

#  define dir_math_save cur_list.math_field

#  define null_font 0
#  define min_quarterword 0

#endif
