/* pdftoepdf.h

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

#ifndef PDFTOEPDF_H
#  define PDFTOEPDF_H

struct InObj {
    Ref ref;                    // ref in original PDF
    int num;                    // new object number in output PDF
    InObj *next;                // next entry in list of indirect objects
};

struct PdfDocument {
    char *file_path;            // full file name including path
    char *checksum;             // for reopening
    PDFDoc *doc;
    InObj *inObjList;           // temporary linked list
    avl_table *ObjMapTree;      // permanent over luatex run
    int occurences;             // number of references to the PdfDocument; it can be deleted when occurences == 0
};

PdfDocument *refPdfDocument(char *file_path);

#endif                          /* PDFTOEPDF_H */
