/* pdfgen.c
   
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

#include "ptexlib.h"

static const char __svn_version[] = "$Id" "$URL";


/*
Sometimes it is neccesary to allocate memory for PDF output that cannot
be deallocated then, so we use |pdf_mem| for this purpose.
*/

integer pdf_mem_size = inf_pdf_mem_size;        /* allocated size of |pdf_mem| array */
integer *pdf_mem;
/* the first word is not used so we can use zero as a value for testing
   whether a pointer to |pdf_mem| is valid  */
integer pdf_mem_ptr = 1;

/*
  We use |pdf_get_mem| to allocate memory in |pdf_mem|
*/

integer pdf_get_mem(integer s)
{                               /* allocate |s| words in |pdf_mem| */
    integer a;
    integer ret;
    if (s > sup_pdf_mem_size - pdf_mem_ptr)
        overflow(maketexstring("PDF memory size (pdf_mem_size)"), pdf_mem_size);
    if (pdf_mem_ptr + s > pdf_mem_size) {
        a = 0.2 * pdf_mem_size;
        if (pdf_mem_ptr + s > pdf_mem_size + a) {
            pdf_mem_size = pdf_mem_ptr + s;
        } else if (pdf_mem_size < sup_pdf_mem_size - a) {
            pdf_mem_size = pdf_mem_size + a;
        } else {
            pdf_mem_size = sup_pdf_mem_size;
        }
        pdf_mem = xreallocarray(pdf_mem, integer, pdf_mem_size);
    }
    ret = pdf_mem_ptr;
    pdf_mem_ptr = pdf_mem_ptr + s;
    return ret;
}
