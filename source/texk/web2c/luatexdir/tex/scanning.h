/* scanning.h
   
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

/* $Id$ */

#ifndef SCANNING_H
#  define SCANNING_H

typedef enum {
    int_val_level = 0,          /* integer values */
    attr_val_level,             /* integer values */
    dimen_val_level,            /* dimension values */
    glue_val_level,             /* glue specifications */
    mu_val_level,               /* math glue specifications */
    dir_val_level,              /* directions */
    ident_val_level,            /* font identifier */
    tok_val_level,              /* token lists */
} value_level_code;

typedef union {
    int int_val; /* int, attr or dir */
    int dimen_val; /* dimension */
    halfword glu_val; /* glue or muglue spec */
    halfword token_val; /* font identifier or token list */
} scan_value;
typedef struct {
    scan_value value;
    union {
        int radix; /* from scan_int, used by scan_dimen */
        int order; /* from scan_dimen, used by scan_glue */
    } info;
    value_level_code level;
} scan_result;

extern void scan_left_brace(int status);
extern void scan_optional_equals(int status);

extern scan_result *scan_something_simple(halfword cmd, halfword subitem);
extern void scan_something_internal(scan_result*val, int level, boolean negative, int status);


extern void scan_limited_int(scan_result *val, int max, const char *name, int status);

#  define scan_register_num(v,x) scan_limited_int(v,65535,"register code",x)
#  define scan_mark_num(v,x) scan_limited_int(v,65535,"marks code",x)
#  define scan_char_num(v,x) scan_limited_int(v,biggest_char,"character code",x)
#  define scan_four_bit_int(v,x) scan_limited_int(v,15,NULL,x)
#  define scan_math_family_int(v,x) scan_limited_int(v,255,"math family",x)
#  define scan_real_fifteen_bit_int(v,x) scan_limited_int(v,32767,"mathchar",x)
#  define scan_big_fifteen_bit_int(v,x) scan_limited_int(v,0x7FFFFFF,"extended mathchar",x)
#  define scan_twenty_seven_bit_int(v,x) scan_limited_int(v,0777777777,"delimiter code",x)

extern void scan_fifteen_bit_int(scan_result *val, int status);
extern void scan_four_bit_int_or_18(scan_result *val, int status);

#  define octal_token (other_token+'\'')        /* apostrophe, indicates an octal constant */
#  define hex_token (other_token+'"')   /* double quote, indicates a hex constant */
#  define alpha_token (other_token+'`') /* reverse apostrophe, precedes alpha constants */
#  define point_token (other_token+'.') /* decimal point */
#  define continental_point_token (other_token+',')     /* decimal point, Eurostyle */
#  define infinity 017777777777 /* the largest positive value that \TeX\ knows */
#  define zero_token (other_token+'0')  /* zero, the smallest digit */
#  define A_token (letter_token+'A')    /* the smallest special hex digit */
#  define other_A_token (other_token+'A') /* special hex digit of type |other_char| */

extern void scan_int(scan_result *val, int status);

#  define scan_normal_dimen(v,x) scan_dimen(v,false,false,false,x)

extern void scan_dimen(scan_result *val, boolean mu, boolean inf, boolean shortcut, int status);
extern void scan_glue(scan_result *val, int level, int status);

extern halfword the_toks(int status);
extern str_number the_scanned_result(scan_result *val);
extern void set_font_dimen(int status);
extern void get_font_dimen(scan_result *val, int status);

#  define default_rule 26214    /* 0.4\thinspace pt */

extern halfword scan_rule_spec(int status);

extern void scan_font_ident(scan_result *val, int status);
extern void scan_general_text(scan_result *val);
extern void get_x_or_protected(int status);
extern halfword scan_toks(boolean macrodef, boolean xpand);


extern void scan_normal_glue(scan_result *val, int status);
extern void scan_mu_glue(scan_result *val, int status);

extern int add_or_sub(int x, int y, int max_answer, boolean negative);
extern int quotient(int n, int d);
extern int fract(int x, int n, int d, int max_answer);
extern void scan_expr(scan_result *val, int level, int status);



#endif
