% pdfluaapi.w
% 
% Copyright 2010 Taco Hoekwater <taco@@luatex.org>

% This file is part of LuaTeX.

% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.

% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.

% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
static const char _svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"

@ @c
lua_State *new_pdflua(void)
{
    int err;
    lua_State *P;
    Byte *uncompr;
    zlib_struct *zp = (zlib_struct *) pdflua_zlib_struct_ptr;
    uLong uncomprLen = zp->uncomprLen;
    P = lua_newstate(my_luaalloc, NULL);
    if (P == NULL)
        pdftex_fail("new_pdflua(): lua_newstate()");
    lua_atpanic(P, &my_luapanic);
    do_openlibs(P);             /* does all the 'simple' libraries */
    if ((uncompr = xtalloc(zp->uncomprLen, Byte)) == NULL)
        pdftex_fail("new_pdflua(): xtalloc()");
    err = uncompress(uncompr, &uncomprLen, zp->compr, zp->comprLen);
    if (err != Z_OK)
        pdftex_fail("new_pdflua(): uncompress()");
    assert(uncomprLen == zp->uncomprLen);
    if (luaL_loadbuffer(P, (const char *) uncompr, uncomprLen, "pdflua")
        || lua_pcall(P, 0, 0, 0))
        pdftex_fail("new_pdflua(): lua_pcall()");
    xfree(uncompr);
    return P;
}
