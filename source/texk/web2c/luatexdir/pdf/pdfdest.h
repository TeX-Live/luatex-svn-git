/* pdfdest.h

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

#ifndef PDFDEST_H
#  define PDFDEST_H

/* types of destinations */

typedef enum {
    pdf_dest_xyz = 0,
    pdf_dest_fit = 1,
    pdf_dest_fith = 2,
    pdf_dest_fitv = 3,
    pdf_dest_fitb = 4,
    pdf_dest_fitbh = 5,
    pdf_dest_fitbv = 6,
    pdf_dest_fitr = 7
} pdf_destination_types;

typedef struct dest_name_entry_ {
    str_number objname;         /* destination name */
    integer objnum;             /* destination object number */
} dest_name_entry;

/* max number of kids for balanced trees */
#  define name_tree_kids_max 32 /* max number of kids of node of name tree for name destinations */

/* data structure of destinations */

#  define obj_dest_ptr              obj_aux     /* pointer to |pdf_dest_node| */
#  define set_obj_dest_ptr(pdf,A,B) obj_dest_ptr(pdf,A)=B

#  define set_pdf_dest_id(A,B) pdf_dest_id(A)=B
#  define set_pdf_dest_named_id(A,B) pdf_dest_named_id(A)=B
#  define set_pdf_dest_type(A,B) pdf_dest_type(A)=B
#  define set_pdf_dest_xyz_zoom(A,B) pdf_dest_xyz_zoom(A)=B

#  define inf_dest_names_size 1000      /* min size of the destination names table for PDF output */
#  define sup_dest_names_size 131072    /* max size of the destination names table for PDF output */

extern integer dest_names_size;
extern integer pdf_dest_names_ptr;
extern dest_name_entry *dest_names;

extern integer dest_names_size; /* maximum number of names in name tree of PDF output file */

extern void append_dest_name(str_number s, integer n);
extern void do_dest(PDF pdf, halfword p, halfword parent_box,
                    scaledpos cur_orig);
extern void warn_dest_dup(integer id, small_number byname, char *s1, char *s2);

extern void write_out_pdf_mark_destinations(PDF);
extern void scan_pdfdest(PDF);
extern void init_dest_names(void);
extern void sort_dest_names(integer l, integer r);

#endif
