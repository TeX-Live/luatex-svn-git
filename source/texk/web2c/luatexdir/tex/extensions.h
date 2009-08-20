/* extensions.h

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

#ifndef EXTENSIONS_H
#  define EXTENSIONS_H

extern alpha_file write_file[16];
extern halfword write_file_mode[16];
extern halfword write_file_translation[16];
extern boolean write_open[18];
extern scaled neg_wd;
extern scaled pos_wd;
extern scaled neg_ht;

extern halfword write_loc;

extern void do_extension(PDF pdf);

/* Three extra node types carry information from |main_control|. */

/*
User defined whatsits can be inserted into node lists to pass data
along from one lua call to anotherb without interference from the
typesetting engine itself. Each has an id, a type, and a value. The
type of the value depends on the |user_node_type| field.
*/

#  define flushable(A)  ((A) == str_ptr - 1)

extern void new_whatsit(int s);
extern void new_write_whatsit(int w);
extern void scan_pdf_ext_toks(void);
extern halfword prev_rightmost(halfword s, halfword e);
extern integer pdf_last_xform;
extern integer pdf_last_ximage;
extern integer pdf_last_ximage_pages;
extern integer pdf_last_ximage_colordepth;
extern void flush_str(str_number s);
extern integer pdf_last_annot;
extern integer pdf_last_link;
extern scaledpos pdf_last_pos;
extern halfword concat_tokens(halfword q, halfword r);
extern integer pdf_retval;

extern halfword make_local_par_node(void);


/*
The \.{\\pagediscards} and \.{\\splitdiscards} commands share the
command code |un_vbox| with \.{\\unvbox} and \.{\\unvcopy}, they are
distinguished by their |chr_code| values |last_box_code| and
|vsplit_code|.  These |chr_code| values are larger than |box_code| and
|copy_code|.
*/

extern boolean *eof_seen;       /* has eof been seen? */
extern void print_group(boolean e);
extern void group_trace(boolean e);
extern save_pointer *grp_stack; /* initial |cur_boundary| */
extern halfword *if_stack;      /* initial |cond_ptr| */
extern void group_warning(void);
extern void if_warning(void);
extern void file_warning(void);

extern halfword last_line_fill; /* the |par_fill_skip| glue node of the new paragraph */

#  define get_tex_dimen_register(j) dimen(j)
#  define get_tex_skip_register(j) skip(j)
#  define get_tex_count_register(j) count(j)
#  define get_tex_attribute_register(j) attribute(j)
#  define get_tex_box_register(j) box(j)

extern integer set_tex_dimen_register(integer j, scaled v);
extern integer set_tex_skip_register(integer j, halfword v);
extern integer set_tex_count_register(integer j, scaled v);
extern integer set_tex_box_register(integer j, scaled v);
extern integer set_tex_attribute_register(integer j, scaled v);
extern integer get_tex_toks_register(integer l);
extern integer set_tex_toks_register(integer j, str_number s);
extern scaled get_tex_box_width(integer j);
extern integer set_tex_box_width(integer j, scaled v);
extern scaled get_tex_box_height(integer j);
extern integer set_tex_box_height(integer j, scaled v);
extern scaled get_tex_box_depth(integer j);
extern integer set_tex_box_depth(integer j, scaled v);

/* Synctex variables */

extern integer synctexoption;
extern integer synctexoffset;

/* Here are extra variables for Web2c. */

extern pool_pointer edit_name_start;
extern integer edit_name_length, edit_line;
extern int ipcon;
extern boolean stop_at_space;
extern str_number save_str_ptr;
extern pool_pointer save_pool_ptr;
extern int shellenabledp;
extern int restrictedshell;
extern char *output_comment;
extern boolean debug_format_file;

#endif
