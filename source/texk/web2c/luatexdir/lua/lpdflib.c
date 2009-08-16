/* lpdflib.c

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

#include "lua/luatex-api.h"
#include <ptexlib.h>

static const char _svn_version[] =
    "$Id$ "
    "$URL$";

static int find_pos_h(lua_State * L)
{
    int j;
    j = static_pdf->posstruct->pos.h;
    lua_pushnumber(L, j);
    return 1;
}

static int find_pos_v(lua_State * L)
{
    int j;
    j = static_pdf->posstruct->pos.v;
    lua_pushnumber(L, j);
    return 1;
}

#define buf_to_pdfbuf_macro(p, s, l)                 \
for (i = 0; i < (l); i++) {                          \
    if (i % 16 == 0)                                 \
        pdf_room(p,16);                              \
    pdf_quick_out(p,((unsigned char *) (s))[i]);     \
}

int luapdfprint(lua_State * L)
{
    int n;
    unsigned i;
    size_t len;
    const char *outputstr, *st;
    ctm_transform_modes literal_mode;
    n = lua_gettop(L);
    if (!lua_isstring(L, -1)) {
        lua_pushstring(L, "no string to print");
        lua_error(L);
    }
    literal_mode = set_origin;
    if (n == 2) {
        if (!lua_isstring(L, -2)) {
            lua_pushstring(L, "invalid argument for print literal mode");
            lua_error(L);
        } else {
            outputstr = (char *) lua_tostring(L, -2);
            if (strcmp(outputstr, "direct") == 0)
                literal_mode = direct_always;
            else if (strcmp(outputstr, "page") == 0)
                literal_mode = direct_page;
            else {
                lua_pushstring(L, "invalid argument for print literal mode");
                lua_error(L);
            }
        }
    } else {
        if (n != 1) {
            lua_pushstring(L, "invalid number of arguments");
            lua_error(L);
        }
    }
    ensure_pdf_header_written(static_pdf);
    switch (literal_mode) {
    case (set_origin):
        pdf_goto_pagemode(static_pdf);
        pdf_set_pos(static_pdf, static_pdf->posstruct->pos);
        break;
    case (direct_page):
        pdf_goto_pagemode(static_pdf);
        break;
    case (direct_always):
        pdf_end_string_nl(static_pdf);
        break;
    default:
        assert(0);
    }
    st = lua_tolstring(L, n, &len);
    buf_to_pdfbuf_macro(static_pdf, st, len);
    return 0;
}

unsigned char *fread_to_buf(lua_State * L, char *filename, size_t * len)
{
    int i = 0;
    FILE *f;
    unsigned char *buf = NULL;
    if ((f = fopen(filename, "rb")) == NULL)
        luaL_error(L, "pdf.immediateobj() cannot open input file");
    if ((i = readbinfile(f, &buf, (integer *) len)) == 0)
        luaL_error(L, "pdf.immediateobj() cannot read input file");
    fclose(f);
    return buf;
}

static int l_immediateobj(lua_State * L)
{
    int n, first_arg = 1;
    unsigned i;
    integer k;
    size_t buflen, len1, len2, len3;
    unsigned char *buf;
    const char *st1 = NULL, *st2 = NULL, *st3 = NULL;
    n = lua_gettop(L);
    ensure_pdf_header_written(static_pdf);
    if (n > 0 && lua_type(L, 1) == LUA_TNUMBER) {
        first_arg++;
        k = lua_tonumber(L, 1);
        n--;
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->obj_count);
        pdf_last_obj = k = static_pdf->obj_ptr;
    }
    switch (n) {
    case 0:
        luaL_error(L, "pdf.immediateobj() needs at least one argument");
        break;
    case 1:                    /* original case unchanged */
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.immediateobj() 1st argument must be string");
        pdf_begin_obj(static_pdf, k, 1);
        st1 = lua_tolstring(L, first_arg, &len1);
        buf_to_pdfbuf_macro(static_pdf, st1, len1);
        if (st1[len1 - 1] != '\n')
            pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
        break;
    case 2:
    case 3:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.immediateobj() 1st argument must be string");
        if (!lua_isstring(L, (first_arg + 1)))
            luaL_error(L, "pdf.immediateobj() 2nd argument must be string");
        st1 = (char *) lua_tolstring(L, first_arg, &len1);
        st2 = (char *) lua_tolstring(L, (first_arg + 1), &len2);
        if (len1 == 4 && strcmp(st1, "file") == 0) {
            if (n == 3)
                luaL_error(L,
                           "pdf.immediateobj() 3rd argument forbidden in file mode");
            pdf_begin_obj(static_pdf, k, 1);
            buf = fread_to_buf(L, (char *) st2, &buflen);
            buf_to_pdfbuf_macro(static_pdf, buf, buflen);
            if (buf[buflen - 1] != '\n')
                pdf_puts(static_pdf, "\n");
            xfree(buf);
            pdf_end_obj(static_pdf);
        } else {
            pdf_begin_dict(static_pdf, k, 0);   /* 0 = not an object stream candidate! */
            if (n == 3) {       /* write attr text */
                if (!lua_isstring(L, (first_arg + 2)))
                    luaL_error(L,
                               "pdf.immediateobj() 3rd argument must be string");
                st3 = (char *) lua_tolstring(L, (first_arg + 2), &len3);
                buf_to_pdfbuf_macro(static_pdf, st3, len3);
                if (st3[len3 - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            }
            pdf_begin_stream(static_pdf);
            if (len1 == 6 && strcmp(st1, "stream") == 0) {
                buf_to_pdfbuf_macro(static_pdf, st2, len2);
            } else if (len1 == 10 && strcmp(st1, "streamfile") == 0) {
                buf = fread_to_buf(L, (char *) st2, &buflen);
                buf_to_pdfbuf_macro(static_pdf, buf, buflen);
                xfree(buf);
            } else
                luaL_error(L, "pdf.immediateobj() invalid argument");
            pdf_end_stream(static_pdf);
        }
        break;
    default:
        luaL_error(L, "pdf.immediateobj() allows max. 3 arguments");
    }
    lua_pushinteger(L, k);
    return 1;
}


static int l_obj(lua_State * L)
{
    int n, first_arg = 1;
    integer k;
    size_t len1;
    const char *st1 = NULL;
    n = lua_gettop(L);
    if (n > 0 && lua_type(L, 1) == LUA_TNUMBER) {
        first_arg++;
        k = lua_tonumber(L, 1);
        n--;
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->obj_count);
        pdf_last_obj = k = static_pdf->obj_ptr;
    }
    set_obj_data_ptr(static_pdf, k, pdf_get_mem(static_pdf, pdfmem_obj_size));
    set_obj_obj_is_stream(static_pdf, k, 0);
    set_obj_obj_stream_attr(static_pdf, k, 0);
    set_obj_obj_is_file(static_pdf, k, 0);
    switch (n) {
    case 0:
        luaL_error(L, "pdf.obj() needs at least one argument");
        break;
    case 1:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.obj() 1st argument must be string");
        set_obj_obj_data(static_pdf, k, tokenlist_from_lua(L));
        break;
    case 2:
    case 3:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.obj() 1st argument must be string");
        if (!lua_isstring(L, (first_arg + 1)))
            luaL_error(L, "pdf.obj() 2nd argument must be string");
        st1 = (char *) lua_tolstring(L, first_arg, &len1);
        if (len1 == 4 && strcmp(st1, "file") == 0) {
            if (n == 3)
                luaL_error(L, "pdf.obj() 3rd argument forbidden in file mode");
            set_obj_obj_is_file(static_pdf, k, 1);
        } else {
            if (n == 3) {       /* write attr text */
                if (!lua_isstring(L, (first_arg + 2)))
                    luaL_error(L, "pdf.obj() 3rd argument must be string");
                set_obj_obj_stream_attr(static_pdf, k, tokenlist_from_lua(L));
                lua_pop(L, 1);
            }
            if (len1 == 6 && strcmp(st1, "stream") == 0) {
                set_obj_obj_is_stream(static_pdf, k, 1);
            } else if (len1 == 10 && strcmp(st1, "streamfile") == 0) {
                set_obj_obj_is_stream(static_pdf, k, 1);
                set_obj_obj_is_file(static_pdf, k, 1);
            } else
                luaL_error(L, "pdf.obj() invalid argument");
        }
        set_obj_obj_data(static_pdf, k, tokenlist_from_lua(L));
        break;
    default:
        luaL_error(L, "pdf.obj() allows max. 3 arguments");
    }
    lua_pushinteger(L, k);
    return 1;
}

static int l_reserveobj(lua_State * L)
{
    int n;
    size_t len;
    const char *st;
    n = lua_gettop(L);
    switch (n) {
    case 0:
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->obj_count);
        pdf_last_obj = static_pdf->obj_ptr;
        break;
    case 1:
        if (!lua_isstring(L, -1))
            luaL_error(L, "pdf.reserveobj() optional argument must be string");
        st = (char *) lua_tolstring(L, 1, &len);
        if (len == 5 && strcmp(st, "annot") == 0) {
            pdf_create_obj(static_pdf, obj_type_others, 0);
            pdf_last_annot = static_pdf->obj_ptr;
        } else {
            luaL_error(L, "pdf.reserveobj() optional string must be \"annot\"");
        }
        lua_pop(L, 1);
        break;
    default:
        luaL_error(L, "pdf.reserveobj() allows max. 1 argument");
    }
    lua_pushinteger(L, static_pdf->obj_ptr);
    return 1;
}

static int getpdf(lua_State * L)
{
    char *st;
    if (lua_isstring(L, 2)) {
        st = (char *) lua_tostring(L, 2);
        if (st && *st) {
            if (*st == 'h')
                return find_pos_h(L);
            else if (*st == 'v')
                return find_pos_v(L);
        }
    }
    lua_pushnil(L);
    return 1;
}

static int setpdf(lua_State * L)
{
    return (L == NULL ? 0 : 0); /* for -Wall */
}

static const struct luaL_reg pdflib[] = {
    {"print", luapdfprint},
    {"immediateobj", l_immediateobj},
    {"obj", l_obj},
    {"reserveobj", l_reserveobj},
    {NULL, NULL}                /* sentinel */
};

int luaopen_pdf(lua_State * L)
{
    luaL_register(L, "pdf", pdflib);
    /* build meta table */
    luaL_newmetatable(L, "pdf_meta");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, getpdf);
    /* do these later, NYI */
    if (0) {
        lua_settable(L, -3);
        lua_pushstring(L, "__newindex");
        lua_pushcfunction(L, setpdf);
    }
    lua_settable(L, -3);
    lua_setmetatable(L, -2);    /* meta to itself */
    return 1;
}
