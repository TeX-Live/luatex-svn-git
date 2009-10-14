/* pdfobj.h

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

#ifndef PDFOBJ_H
#  define PDFOBJ_H

/* data structure for \.{\\pdfobj} and \.{\\pdfrefobj} */

#  define set_pdf_obj_objnum(A,B) pdf_obj_objnum(A)=B


#  define pdfmem_obj_size          5    /* size of memory in |mem| which |obj_data_ptr| holds */

#  define obj_obj_data(pdf,A)          pdf->mem[obj_data_ptr(pdf,A) + 0]        /* object data */
#  define obj_obj_is_stream(pdf,A)     pdf->mem[obj_data_ptr(pdf,A) + 1]        /* will this object
                                                                                   be written as a stream instead of a dictionary? */
#  define obj_obj_stream_attr(pdf,A)   pdf->mem[obj_data_ptr(pdf,A) + 2]        /* additional object attributes for streams */
#  define obj_obj_is_file(pdf,A)       pdf->mem[obj_data_ptr(pdf,A) + 3]        /* data should be
                                                                                   read from an external file? */

#  define obj_obj_uncompressed(pdf,A)  pdf->mem[obj_data_ptr(pdf,A) + 4]        /* should this object be uncompressed always? */

#  define set_obj_obj_uncompressed(pdf,A,B) obj_obj_uncompressed(pdf,A)=B
#  define set_obj_obj_is_stream(pdf,A,B) obj_obj_is_stream(pdf,A)=B
#  define set_obj_obj_stream_attr(pdf,A,B) obj_obj_stream_attr(pdf,A)=B
#  define set_obj_obj_is_file(pdf,A,B) obj_obj_is_file(pdf,A)=B
#  define set_obj_obj_data(pdf,A,B) obj_obj_data(pdf,A)=B

extern integer pdf_last_obj;

extern void pdf_check_obj(PDF pdf, integer t, integer n);
extern void pdf_write_obj(PDF pdf, integer n);
extern void scan_obj(PDF pdf);
extern void pdf_ref_obj(PDF pdf, halfword p);

#endif
