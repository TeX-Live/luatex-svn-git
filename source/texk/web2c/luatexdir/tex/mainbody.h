/* mainbody.h
   
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

#ifndef MAINBODY_H
#  define MAINBODY_H


#define the_luatex_version 41 /* \.{\\luatexversion}  */
#define the_luatex_revision "0" /* \.{\\luatexrevision}  */
#define luatex_date_info 20000000 /* the compile date is negated */
#define luatex_version_string "beta-0.41.0"

#define engine_name "luatex" /* the name of this engine */

/*
The following parameters can be changed at compile time to extend or
reduce \TeX's capacity. They may have different values in \.{INITEX} and
in production versions of \TeX.
@.INITEX@>
@^system dependencies@>
*/

#define ssup_max_strings 262143

#define inf_main_memory   2000000
#define sup_main_memory   32000000

#define inf_max_strings   100000
#define sup_max_strings   ssup_max_strings
#define inf_strings_free   100
#define sup_strings_free   sup_max_strings

#define inf_buf_size   500
#define sup_buf_size   100000000

#define inf_nest_size   40
#define sup_nest_size   4000

#define inf_max_in_open   6
#define sup_max_in_open   127

#define inf_param_size   60
#define sup_param_size   6000

#define inf_save_size   600
#define sup_save_size   80000

#define inf_stack_size   200
#define sup_stack_size   30000

#define inf_dvi_buf_size   800
#define sup_dvi_buf_size   65536

#define inf_pool_size   128000
#define sup_pool_size   40000000
#define inf_pool_free   1000
#define sup_pool_free   sup_pool_size

#define inf_string_vacancies   8000
#define sup_string_vacancies   (sup_pool_size - 23000)

#define sup_hash_extra   sup_max_strings
#define inf_hash_extra   0

#define sup_ocp_list_size    1000000
#define inf_ocp_list_size    1000
#define sup_ocp_buf_size     100000000
#define inf_ocp_buf_size     1000
#define sup_ocp_stack_size   1000000
#define inf_ocp_stack_size   1000

#define inf_expand_depth   100
#define sup_expand_depth   1000000


#include <stdio.h>
#include "cpascal.h"

/* Types in the outer block */
typedef int ASCII_code; /* characters */
typedef unsigned char eight_bits; /* unsigned one-byte quantity */
typedef FILE *alpha_file; /* files that contain textual data */
typedef FILE *byte_file; /* files that contain binary data */

typedef integer str_number; 
typedef integer pool_pointer; 
typedef unsigned char packed_ASCII_code;

typedef integer scaled; /* this type is used for scaled integers */
typedef char small_number; /* this type is self-explanatory */

typedef float glue_ratio; /* one-word representation of a glue expansion factor */

typedef unsigned short quarterword;
typedef int halfword;
#include "tex/memoryword.h"

typedef unsigned char glue_ord; /* infinity to the 0, 1, 2, 3, or 4 power */

typedef unsigned char group_code; /* |save_level| for a level boundary */

typedef int internal_font_number; /* |font| in a |char_node| */
typedef integer font_index; /* index into |font_info| */

typedef integer save_pointer;

#define _text_char ASCII_code /* the data type of characters in text files */
#define _first_text_char 0 /* ordinal number of the smallest element of |text_char| */
#define _last_text_char 255 /*ordinal number of the largest element of |text_char| */

#define null_code '\0' /* ASCII code that might disappear */
#define carriage_return '\r' /* ASCII code used at end of line */
#define _invalid_code 0177 /* ASCII code that many systems prohibit in text files */

/* Global variables */
extern integer bad; /* is some ``constant'' wrong? */
extern boolean luainit; /* are we using lua for initializations  */
extern boolean tracefilenames; /* print file open-close  info? */


extern boolean ini_version; /* are we \.{INITEX}? */
extern boolean dump_option;
extern boolean dump_line;
extern integer bound_default;
extern char *bound_name;
extern integer error_line;
extern integer half_error_line;
extern integer max_print_line;
extern integer ocp_list_size;
extern integer ocp_buf_size;
extern integer ocp_stack_size;
extern integer max_strings;
extern integer strings_free;
extern integer string_vacancies;
extern integer pool_size;
extern integer pool_free;
extern integer font_k;
extern integer buf_size; 
extern integer stack_size;
extern integer max_in_open;
extern integer param_size;
extern integer nest_size;
extern integer save_size; 
extern integer expand_depth;
extern int parsefirstlinep;
extern int filelineerrorstylep;
extern int haltonerrorp;
extern boolean quoted_filename;

#define min_quarterword 0 /*smallest allowable value in a |quarterword| */
#define max_quarterword 65535 /* largest allowable value in a |quarterword| */
#define min_halfword  -0x3FFFFFFF /* smallest allowable value in a |halfword| */
#define max_halfword 0x3FFFFFFF /* largest allowable value in a |halfword| */

#define fix_date_and_time() dateandtime(int_par(time_code),int_par(day_code),int_par(month_code),int_par(year_code))

/* next(#) == hash[#].lh {link for coalesced lists} */
/* text(#) == hash[#].rh {string number for control sequence name} */
/* hash_is_full == (hash_used=hash_base) {test if all positions are occupied} */
/* font_id_text(#) == text(font_id_base+#) {a frozen font identifier's name} */

extern two_halves *hash; /* the hash table */
extern halfword hash_used; /* allocation pointer for |hash| */
extern integer hash_extra; /* |hash_extra=hash| above |eqtb_size| */
extern halfword hash_top; /* maximum of the hash array */
extern halfword hash_high; /* pointer to next high hash location */
extern boolean no_new_control_sequence; /* are new identifiers legal? */
extern integer cs_count; /* total number of known identifiers */

extern str_number get_cs_text(integer cs);

extern integer mag_set; /* if nonzero, this magnification should be used henceforth */
extern void prepare_mag (void);

extern integer cur_cmd; /* current command set by |get_next| */
extern halfword cur_chr; /* operand of current command */
extern halfword cur_cs; /* control sequence found here, zero if none found */
extern halfword cur_tok; /* packed representative of |cur_cmd| and |cur_chr| */

extern void show_cur_cmd_chr (void);

/* #define max_dimen 07777777777 *//* $2^{30}-1$ */

extern integer font_bytes;

extern void set_cur_font (internal_font_number f);

extern integer get_luatexversion (void);
extern str_number get_luatexrevision (void);
extern integer  get_luatex_date_info (void);

extern void ship_out(halfword p);

extern int ready_already;

extern void main_body (void);

extern void close_files_and_terminate (void);
extern void final_cleanup (void);
extern void init_prim (void);
extern void debug_help (void); /* routine to display various things */

/* lazy me */
#define zget_cs_text get_cs_text
#define get_cur_font() equiv(cur_font_loc)
#define zset_cur_font set_cur_font


#endif

