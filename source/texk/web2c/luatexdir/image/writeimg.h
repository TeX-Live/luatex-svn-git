/* writeimg.h

   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

#ifndef WRITEIMG_H
#  define WRITEIMG_H

#  include "../dvi/dvigen.h"    /* for scaled_whd only */
#  include "image.h"

typedef image *img_entry;
img_entry *img_array;

image_dict *new_image_dict(void);
image *new_image(void);
integer img_to_array(image *);
integer read_image(PDF, integer, integer, char *, integer, char *,
                   char *, integer, integer, integer, integer);
void check_pdfstream_dict(image_dict *);
void dumpimagemeta(void);
void free_image_dict(image_dict * p);
void init_image_dict(image_dict *);
void init_image(image *);
void new_img_pdfstream_struct(image_dict *);
void out_image(PDF, integer, scaled, scaled);
void pdf_print_resname_prefix(void);
void read_img(PDF, image_dict *, integer, integer);
void scale_img(image *);
void set_image_dimensions(integer, scaled_whd);
void undumpimagemeta(PDF, integer, integer);
void write_img(PDF, image_dict *);
void write_pdfstream(PDF, image_dict *);

#endif                          /* WRITEIMG_H */
