/* pdfimage.h

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

#ifndef PDFIMAGE_H
#  define PDFIMAGE_H

extern halfword pdf_ximage_list;        /* list of images in the current page */
extern integer pdf_ximage_count;        /* counter of images */

void place_img(PDF, image *);
void pdf_place_image(PDF pdf, integer idx);

#  define set_pdf_ximage_idx(A, B) pdf_ximage_idx(A) = B

#endif
