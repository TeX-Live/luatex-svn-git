/* pdfgen.h

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

#ifndef PDFGEN_H
#  define PDFGEN_H

#  define inf_pdf_mem_size 10000        /* min size of the |pdf_mem| array */
#  define sup_pdf_mem_size 10000000     /* max size of the |pdf_mem| array */

extern integer pdf_mem_size;
extern integer *pdf_mem;
extern integer pdf_mem_ptr;

#endif
