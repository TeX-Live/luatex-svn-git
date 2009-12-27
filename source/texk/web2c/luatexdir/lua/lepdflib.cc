/* lepdflib.cc

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

static const char _svn_version[] =
    "$Id$ "
    "$URL$";

#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef POPPLER_VERSION
#  define GString GooString
#  include <dirent.h>
#  include <poppler-config.h>
#  include <goo/GooString.h>
#  include <goo/gmem.h>
#  include <goo/gfile.h>
#else
#  include <aconf.h>
#  include <GString.h>
#  include <gmem.h>
#  include <gfile.h>
#  include <assert.h>
#endif
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Link.h"
#include "Page.h"
#include "GfxFont.h"
#include "PDFDoc.h"
#include "GlobalParams.h"
#include "Error.h"

#include "../image/epdf.h"
#include "../image/pdftoepdf.h"

extern "C" {
#include "lua/luatex-api.h"
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
};

//**********************************************************************

int l_new_pdfdoc(lua_State * L)
{
    PdfDocument *pdf_doc;
    char *file_path;
    if (lua_gettop(L) != 1)
        luaL_error(L, "epdf.new() needs exactly 1 argument");
    if (!lua_isstring(L, -1))
        luaL_error(L, "epdf.new() needs filename (string)");
    file_path = (char *) lua_tostring(L, -1);

    printf("\n======================== 1 <%s>\n", file_path);

    pdf_doc = refPdfDocument(file_path);

    return 0;
}

//**********************************************************************

static const struct luaL_Reg epdflib[] = {
    // {"new", l_new_pdfdoc},
    {NULL, NULL}                /* sentinel */
};

int luaopen_epdf(lua_State * L)
{
    luaL_register(L, "epdf", epdflib);
    return 1;
}

//**********************************************************************
