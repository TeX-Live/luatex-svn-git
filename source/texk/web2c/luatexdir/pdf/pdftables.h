/* pdftables.h

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

#ifndef PDFTABLES_H
#  define PDFTABLES_H

/*
The cross-reference table |obj_tab| is an array of |obj_tab_size| of
|obj_entry|. Each entry contains five integer fields and represents an object
in PDF file whose object number is the index of this entry in |obj_tab|.
Objects in |obj_tab| maybe linked into list; objects in such a linked list have
the same type.
*/

/*
The first field contains information representing identifier of this object.
It is usally a number for most of object types, but it may be a string number
for named destination or named thread.

The second field of |obj_entry| contains link to the next
object in |obj_tab| if this object is linked in a list.

The third field holds the byte offset of the object in the output PDF file,
or its byte offset within an object stream. As long as the object is not
written, this field is used for flags about the write status of the object;
then it has a negative value.

The fourth field holds the object number of the object stream, into which
the object is included.

The last field usually represents the pointer to some auxiliary data
structure depending on the object type; however it may be used as a counter as
well.
*/

#  define obj_info(pdf,A)  pdf->obj_tab[(A)].int0       /* information representing identifier of this object */
#  define obj_link(pdf,A)  pdf->obj_tab[(A)].int1       /* link to the next entry in linked list */
#  define obj_offset(pdf,A) pdf->obj_tab[(A)].int2
                                        /* negative (flags), or byte offset for this object in PDF 
                                           output file, or object stream number for this object */
#  define obj_os_idx(pdf,A) pdf->obj_tab[(A)].int3
                                        /* index of this object in object stream */
#  define obj_aux(pdf,A)  pdf->obj_tab[(A)].int4        /* auxiliary pointer */
#  define obj_data_ptr             obj_aux      /* pointer to |pdf->mem| */

#  define set_obj_link(pdf,A,B) obj_link(pdf,A)=B
#  define set_obj_info(pdf,A,B) obj_info(pdf,A)=B
#  define set_obj_offset(pdf,A,B) obj_offset(pdf,A)=B
#  define set_obj_aux(pdf,A,B) obj_aux(pdf,A)=B
#  define set_obj_data_ptr(pdf,A,B) obj_data_ptr(pdf,A)=B

#  define set_obj_fresh(pdf,A)      obj_offset(pdf,(A))=-2
#  define set_obj_scheduled(pdf,A)  if (intcast(obj_offset(pdf,A))==-2) obj_offset(pdf,A)=-1
#  define is_obj_scheduled(pdf,A)  (intcast(obj_offset(pdf,A))>-2)
#  define is_obj_written(pdf,A)    (intcast(obj_offset(pdf,A))>-1)

#  define head_tab_max obj_type_thread  /* max index of |head_tab| */

/*  NOTE: The data structure definitions for the nodes on the typesetting side are 
    inside |nodes.h| */

/* Some constants */
#  define inf_pk_dpi 72         /* min PK pixel density value from \.{texmf.cnf} */
#  define sup_pk_dpi 8000       /* max PK pixel density value from \.{texmf.cnf} */
#  define pdf_objtype_max head_tab_max

extern integer pdf_box_spec_media;
extern integer pdf_box_spec_crop;
extern integer pdf_box_spec_bleed;
extern integer pdf_box_spec_trim;
extern integer pdf_box_spec_art;

extern void pdf_create_obj(PDF pdf, integer t, integer i);
extern integer find_obj(PDF pdf, integer t, integer i, boolean byname);
extern integer get_obj(PDF pdf, integer t, integer i, boolean byname);
extern integer pdf_new_objnum(PDF pdf);

extern void set_rect_dimens(halfword p, halfword parent_box, scaledpos cur_orig,
                            scaled_whd alt_rule, scaled margin);

extern void libpdffinish(PDF);

extern void initialize_pdf_output(PDF);

extern void pdfshipoutbegin(boolean shipping_page);
extern void pdfshipoutend(boolean shipping_page);

extern void dump_pdftex_data(PDF pdf);
extern void undump_pdftex_data(PDF pdf);

#  define set_pdf_width(A,B) pdf_width(A)=B
#  define set_pdf_height(A,B) pdf_height(A)=B
#  define set_pdf_depth(A,B) pdf_depth(A)=B

/* interface definitions for eqtb locations */
#  define pdf_minor_version        int_par(param_pdf_minor_version_code)
#  define pdf_decimal_digits       int_par(param_pdf_decimal_digits_code)
#  define pdf_gamma                int_par(param_pdf_gamma_code)
#  define pdf_image_gamma          int_par(param_pdf_image_gamma_code)
#  define pdf_image_hicolor        int_par(param_pdf_image_hicolor_code)
#  define pdf_image_apply_gamma    int_par(param_pdf_image_apply_gamma_code)
#  define pdf_objcompresslevel     int_par(param_pdf_objcompresslevel_code)
#  define pdf_draftmode            int_par(param_pdf_draftmode_code)
#  define pdf_inclusion_copy_font  int_par(param_pdf_inclusion_copy_font_code)
#  define pdf_inclusion_errorlevel int_par(param_pdf_inclusion_errorlevel_code)
#  define pdf_replace_font         int_par(param_pdf_replace_font_code)
#  define pdf_pk_resolution        int_par(param_pdf_pk_resolution_code)
#  define pdf_pk_mode              int_par(param_pdf_pk_mode_code)
#  define pdf_unique_resname       int_par(param_pdf_unique_resname_code)
#  define pdf_compress_level       int_par(param_pdf_compress_level_code)
#  define pdf_move_chars           int_par(param_pdf_move_chars_code)

#endif
