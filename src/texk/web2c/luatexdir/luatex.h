/* luatex.h
   
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

/* some code array functions */

extern void set_math_code(integer n, halfword v, quarterword grouplevel);
extern halfword get_math_code(integer n);

extern void set_del_code(integer n, halfword v, halfword w,
                         quarterword grouplevel);
extern halfword get_del_code_a(integer n);
extern halfword get_del_code_b(integer n);

extern void unsavemathcodes(quarterword grouplevel);
extern void initializemathcodes();
extern void dumpmathcodes();
extern void undumpmathcodes();

extern void set_lc_code(integer n, halfword v, quarterword grouplevel);
extern halfword get_lc_code(integer n);
extern void set_uc_code(integer n, halfword v, quarterword grouplevel);
extern halfword get_uc_code(integer n);
extern void set_sf_code(integer n, halfword v, quarterword grouplevel);
extern halfword get_sf_code(integer n);
extern void set_cat_code(integer h, integer n, halfword v,
                         quarterword grouplevel);
extern halfword get_cat_code(integer h, integer n);

extern void unsavetextcodes(quarterword grouplevel);
extern void unsavecatcodes(integer h, quarterword grouplevel);
extern void copy_cat_codes(int from, int to);
extern void initex_cat_codes(int h);
extern void clearcatcodestack(integer h);
extern boolean validcatcodetable(int h);

extern void initializetextcodes();
extern void dumptextcodes();
extern void undumptextcodes();

extern void do_vf(internal_font_number tmp_f);

extern int readbinfile(FILE * f, unsigned char **b, integer * s);

#define read_tfm_file  readbinfile
#define read_vf_file   readbinfile
#define read_ocp_file  readbinfile
#define read_data_file readbinfile

extern int **ocp_tables;

extern void allocate_ocp_table();
extern void dump_ocp_table();
extern void undump_ocp_table();

extern void run_external_ocp();
extern void b_test_in();

/* Additions to texmfmp.h for pdfTeX */

/* mark a char in font */
#define pdf_mark_char(f,c) set_char_used(f,c,true)

/* test whether a char in font is marked */
#define pdf_char_marked char_used

/* writepdf() always writes by fwrite() */
#define       write_pdf(a, b) \
  (void) fwrite ((char *) &pdf_buf[a], sizeof (pdf_buf[a]), \
                 (int) ((b) - (a) + 1), pdf_file)

#define tex_b_open_in(f) \
    open_input (&(f), kpse_tex_format, FOPEN_RBIN_MODE)
#define ovf_b_open_in(f) \
    open_input (&(f), kpse_ovf_format, FOPEN_RBIN_MODE)
#define vf_b_open_in(f) \
    open_input (&(f), kpse_vf_format, FOPEN_RBIN_MODE)

extern int open_outfile(FILE ** f, char *name, char *mode);

#define do_a_open_out(f) open_outfile(&(f),(char *)(nameoffile+1),FOPEN_W_MODE)
#define do_b_open_out(f) open_outfile(&(f),(char *)(nameoffile+1),FOPEN_WBIN_MODE)

#define pdfassert assert
#define voidcast(a) (void *)(a)
#define varmemcast(a) (memory_word *)(a)
#define fixmemcast(a) (smemory_word *)(a)
extern volatile memory_word *varmem;
extern halfword var_mem_min;
extern halfword var_mem_max;
extern halfword get_node(integer s);
extern void free_node(halfword p, integer s);
extern void init_node_mem(integer r, integer s);
extern void dump_node_mem(void);
extern void undump_node_mem(void);

#include <luatexdir/ptexlib.h>
