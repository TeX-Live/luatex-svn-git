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

static const char _svn_version[] =
    "$Id$ "
    "$URL$";

#include "lua/luatex-api.h"
#include <ptexlib.h>

#define buf_to_pdfbuf_macro(p, s, l)              \
for (i = 0; i < (l); i++) {                       \
    if (i % 16 == 0)                              \
        pdf_room(p, 16);                          \
    pdf_quick_out(p, ((unsigned char *) (s))[i]); \
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
    check_o_mode(static_pdf, "pdf.print()", 1 << OMODE_PDF, true);
    switch (literal_mode) {
    case (set_origin):
        pdf_goto_pagemode(static_pdf);
        pdf_set_pos(static_pdf, static_pdf->posstruct->pos);
        (void)calc_pdfpos(static_pdf->pstruct, static_pdf->posstruct->pos);            
        break;
    case (direct_page):
        pdf_goto_pagemode(static_pdf);
        (void)calc_pdfpos(static_pdf->pstruct, static_pdf->posstruct->pos);            
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
    check_o_mode(static_pdf, "immediateobj()", 1 << OMODE_PDF, true);
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

/**********************************************************************/
/* for LUA_ENVIRONINDEX table lookup (instead of repeated strcmp()) */

typedef enum { P__ZERO, P_RAW, P_STREAM, P__SENTINEL } parm_idx;

typedef struct {
    const char *name;           /* parameter name */
    parm_idx idx;               /* index within img_parms array */
} parm_struct;

const parm_struct obj_parms[] = {
    {NULL, P__ZERO},            /* dummy; lua indices run from 1 */
    {"raw", P_RAW},
    {"stream", P_STREAM},
    {NULL, P__SENTINEL}
};

/**********************************************************************/

static int table_obj(lua_State * L)
{
    int k, type;
    int compress_level = -1;    /* unset */
    int os_level = 1;           /* default: put non-stream objects into object streams */
    int saved_compress_level = static_pdf->compress_level;
    size_t i, buflen, len, l_attr;
    unsigned char *buf;
    const char *s = NULL, *s_attr = NULL;
    int immediate = 0;          /* default: not immediate */
    assert(lua_istable(L, 1));  /* t */

    /* get object "type" */

    lua_pushstring(L, "type");  /* ks t */
    lua_gettable(L, -2);        /* vs? t */
    if (lua_isnil(L, -1))       /* !vs t */
        luaL_error(L, "pdf.obj(): object \"type\" missing");
    if (!lua_isstring(L, -1))   /* !vs t */
        luaL_error(L, "pdf.obj(): object \"type\" must be string");
    lua_pushvalue(L, -1);       /* vs vs t */
    lua_gettable(L, LUA_ENVIRONINDEX);  /* i? vs t */
    if (!lua_isnumber(L, -1))   /* !i vs t */
        luaL_error(L, "pdf.obj(): \"%s\" is not a valid object type",
                   lua_tostring(L, -2));
    type = lua_tointeger(L, -1);        /* i vs t */
    lua_pop(L, 1);              /* vs t */
    switch (type) {
    case P_RAW:
    case P_STREAM:
        break;
    default:
        assert(0);
    }
    lua_pop(L, 1);              /* t */

    /* get optional "immediate" */

    lua_pushstring(L, "immediate");     /* ks t */
    lua_gettable(L, -2);        /* b? t */
    if (!lua_isnil(L, -1)) {    /* b? t */
        if (!lua_isboolean(L, -1))      /* !b t */
            luaL_error(L, "pdf.obj(): \"immediate\" must be boolean");
        immediate = lua_toboolean(L, -1);       /* 0 or 1 */
    }
    lua_pop(L, 1);              /* t */

    /* is a reserved object referenced by "objnum"? */

    lua_pushstring(L, "objnum");        /* ks t */
    lua_gettable(L, -2);        /* vi? t */
    if (!lua_isnil(L, -1)) {    /* vi? t */
        if (!lua_isnumber(L, -1))       /* !vi t */
            luaL_error(L, "pdf.obj(): \"objnum\" must be integer");
        k = lua_tointeger(L, -1);       /* vi t */
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->obj_count);
        pdf_last_obj = k = static_pdf->obj_ptr;
    }
    if (immediate == 0) {
        set_obj_data_ptr(static_pdf, k,
                         pdf_get_mem(static_pdf, pdfmem_obj_size));
        set_obj_obj_is_stream(static_pdf, k, 0);
        set_obj_obj_stream_attr(static_pdf, k, 0);
        set_obj_obj_is_file(static_pdf, k, 0);
        set_obj_obj_pdfcompresslevel(static_pdf, k, -1);        /* unset */
    }
    lua_pop(L, 1);              /* t */

    /* get optional "attr" (allowed only for stream case) */

    lua_pushstring(L, "attr");  /* ks t */
    lua_gettable(L, -2);        /* attr-s? t */
    if (!lua_isnil(L, -1)) {    /* attr-s? t */
        if (type != P_STREAM)
            luaL_error(L,
                       "pdf.obj(): \"attr\" key not allowed for non-stream object");
        if (!lua_isstring(L, -1))       /* !attr-s t */
            luaL_error(L, "pdf.obj(): object \"attr\" must be string");
        if (immediate == 1)
            s_attr = (char *) lua_tolstring(L, -1, &l_attr);    /* attr-s t */
        else
            set_obj_obj_stream_attr(static_pdf, k, tokenlist_from_lua(L));
    }
    lua_pop(L, 1);              /* t */

    /* get optional "compresslevel" (allowed only for stream case) */

    lua_pushstring(L, "compresslevel"); /* ks t */
    lua_gettable(L, -2);        /* vi? t */
    if (!lua_isnil(L, -1)) {    /* vi? t */
        if (type == P_RAW)
            luaL_error(L,
                       "pdf.obj(): \"compresslevel\" key not allowed for raw object");
        if (!lua_isnumber(L, -1))       /* !vi t */
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be integer");
        compress_level = lua_tointeger(L, -1);  /* vi t */
        if (compress_level > 9)
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be <= 9");
        else if (compress_level < 0)
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be >= 0");
    }
    lua_pop(L, 1);              /* t */

    /* get optional "objcompression" (allowed only for non-stream case) */

    lua_pushstring(L, "objcompression");        /* ks t */
    lua_gettable(L, -2);        /* b? t */
    if (!lua_isnil(L, -1)) {    /* b? t */
        if (type == P_STREAM)
            luaL_error(L,
                       "pdf.obj(): \"objcompression\" key not allowed for stream object");
        if (!lua_isboolean(L, -1))      /* !b t */
            luaL_error(L, "pdf.obj(): \"objcompression\" must be boolean");
        os_level = lua_toboolean(L, -1);        /* 0 or 1 */
        /* 0: never compress; 1: depends then on \pdfobjcompresslevel */
        if (immediate == 0 && os_level == 0)
            set_obj_obj_pdfcompresslevel(static_pdf, k, 0);
        /* a kludge, maybe better keep pdfcompresslevel and os_level separate */
    }
    lua_pop(L, 1);              /* t */

    /* now the object contents for all cases are handled */

    lua_pushstring(L, "string");        /* ks t */
    lua_gettable(L, -2);        /* string-s? t */
    lua_pushstring(L, "file");  /* ks string-s? t */
    lua_gettable(L, -3);        /* file-s? string-s? t */
    if (!lua_isnil(L, -1) && !lua_isnil(L, -2)) /* file-s? string-s? t */
        luaL_error(L,
                   "pdf.obj(): \"string\" and \"file\" must not be given together");
    if (lua_isnil(L, -1) && lua_isnil(L, -2))   /* nil nil t */
        luaL_error(L, "pdf.obj(): no \"string\" or \"file\" given");

    switch (type) {
    case P_RAW:
        if (immediate == 1)
            pdf_begin_obj(static_pdf, k, os_level);
        if (!lua_isnil(L, -2)) {        /* file-s? string-s? t */
            lua_pop(L, 1);      /* string-s? t */
            if (!lua_isstring(L, -1))   /* !string-s t */
                luaL_error(L,
                           "pdf.obj(): \"string\" must be string for raw object");
            if (immediate == 1) {
                s = (char *) lua_tolstring(L, -1, &len);
                buf_to_pdfbuf_macro(static_pdf, s, len);
                if (s[len - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            } else
                set_obj_obj_data(static_pdf, k, tokenlist_from_lua(L));
        } else {
            if (!lua_isstring(L, -1))   /* !file-s nil t */
                luaL_error(L,
                           "pdf.obj(): \"file\" name must be string for raw object");
            if (immediate == 1) {
                s = (char *) lua_tolstring(L, -1, &len);        /* file-s nil t */
                buf = fread_to_buf(L, (char *) s, &buflen);
                buf_to_pdfbuf_macro(static_pdf, buf, buflen);
                if (buf[buflen - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
                xfree(buf);
            } else {
                set_obj_obj_is_file(static_pdf, k, 2); /* 2 = lua generated file name */
                set_obj_obj_data(static_pdf, k, luaL_ref(L, LUA_REGISTRYINDEX));
            }
        }
        if (immediate == 1)
            pdf_end_obj(static_pdf);
        break;
    case P_STREAM:
        if (immediate == 1) {
            pdf_begin_dict(static_pdf, k, 0);   /* 0 = not an object stream candidate! */
            if (s_attr != NULL) {
                buf_to_pdfbuf_macro(static_pdf, s_attr, l_attr);
                if (s_attr[l_attr - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            }
            if (compress_level > -1)
                static_pdf->compress_level = compress_level;
            pdf_begin_stream(static_pdf);
        } else {
            if (compress_level > -1)
                set_obj_obj_pdfcompresslevel(static_pdf, k, compress_level);
            set_obj_obj_is_stream(static_pdf, k, 2); /* 2 = lua generated stream */
        }
        if (!lua_isnil(L, -2)) {        /* file-s? string-s? t */
            lua_pop(L, 1);      /* string-s? t */
            if (!lua_isstring(L, -1))   /* !string-s t */
                luaL_error(L,
                           "pdf.obj(): \"string\" must be string for stream object");
            if (immediate == 1) {
                s = (char *) lua_tolstring(L, -1, &len);        /* string-s t */
                buf_to_pdfbuf_macro(static_pdf, s, len);
            } else {
                set_obj_obj_data(static_pdf, k, luaL_ref(L, LUA_REGISTRYINDEX));
            }
        } else {
            if (!lua_isstring(L, -1))   /* !file-s nil t */
                luaL_error(L,
                           "pdf.obj(): \"file\" name must be string for stream object");
            if (immediate == 1) {
                s = (char *) lua_tolstring(L, -1, &len);        /* file-s nil t */
                buf = fread_to_buf(L, (char *) s, &buflen);
                buf_to_pdfbuf_macro(static_pdf, buf, buflen);
                xfree(buf);
            } else {
                set_obj_obj_is_file(static_pdf, k, 2); /* 2 == lua_generated */
                set_obj_obj_data(static_pdf, k, luaL_ref(L,LUA_REGISTRYINDEX));
            }
        }
        if (immediate == 1)
            pdf_end_stream(static_pdf);
        break;
    default:
        assert(0);
    }
    static_pdf->compress_level = saved_compress_level;
    return k;
}

static int orig_obj(lua_State * L)
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
    set_obj_obj_pdfcompresslevel(static_pdf, k, -1);    /* unset */
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
            set_obj_obj_is_file(static_pdf, k, 2);
        } else {
            if (n == 3) {       /* write attr text */
                if (!lua_isstring(L, (first_arg + 2)))
                    luaL_error(L, "pdf.obj() 3rd argument must be string");
                set_obj_obj_stream_attr(static_pdf, k, tokenlist_from_lua(L));
                lua_pop(L, 1);
            }
            if (len1 == 6 && strcmp(st1, "stream") == 0) {
                set_obj_obj_is_stream(static_pdf, k, 2);
            } else if (len1 == 10 && strcmp(st1, "streamfile") == 0) {
                set_obj_obj_is_stream(static_pdf, k, 2);
                set_obj_obj_is_file(static_pdf, k, 2);
            } else
                luaL_error(L, "pdf.obj() invalid argument");
        }
        set_obj_obj_data(static_pdf, k, luaL_ref(L, LUA_REGISTRYINDEX));
        break;
    default:
        luaL_error(L, "pdf.obj() allows max. 3 arguments");
    }
    return k;
}

static int l_obj(lua_State * L)
{
    int n;
    integer k;
    n = lua_gettop(L);
    if (n == 1 && lua_istable(L, 1))
        k = table_obj(L);       /* new */
    else
        k = orig_obj(L);
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
    char *st, *s;
    int l;
    if (lua_isstring(L, 2)) {
        st = (char *) lua_tostring(L, 2);
        if (st) {
            if (strcmp(st,"h")==0) {
                lua_pushnumber(L, static_pdf->posstruct->pos.h);
            } else if (strcmp(st,"v")==0) {
                lua_pushnumber(L, static_pdf->posstruct->pos.v);
            } else if (strcmp(st,"pdfcatalog")==0) {
                s = tokenlist_to_cstring(pdf_catalog_toks, true, &l);
                lua_pushlstring(L,s,l);
            } else if (strcmp(st,"pdfinfo")==0) {
                s = tokenlist_to_cstring(pdf_info_toks, true, &l);
                lua_pushlstring(L,s,l);
            } else if (strcmp(st,"pdfnames")==0) {
                s = tokenlist_to_cstring(pdf_names_toks, true, &l);
                lua_pushlstring(L,s,l);
            } else if (strcmp(st,"pdftrailer")==0) {
                s = tokenlist_to_cstring(pdf_trailer_toks, true, &l);
                lua_pushlstring(L,s,l);
            } else {
                lua_pushnil(L);
            }
        } else {
            lua_pushnil(L);
        }
    }  else {
        lua_pushnil(L);
    }
    return 1;
}

static int setpdf(lua_State * L)
{
    char *st;
    if (lua_gettop(L) != 3) {
        return 0;
    } 
    /* can't set |h| and |v| yet */
    st = (char *) luaL_checkstring(L, 2);
    if (strcmp(st,"pdfcatalog")==0) {
        pdf_catalog_toks = tokenlist_from_lua(L);
    } else if (strcmp(st,"pdfinfo")==0) {
        pdf_info_toks = tokenlist_from_lua(L);
    } else if (strcmp(st,"pdfnames")==0) {
        pdf_names_toks = tokenlist_from_lua(L);
    } else if (strcmp(st,"pdftrailer")==0) {
        pdf_trailer_toks = tokenlist_from_lua(L);
    }
    return 0;
}

static const struct luaL_reg pdflib[] = {
    {"print", luapdfprint},
    {"immediateobj", l_immediateobj},
    {"obj", l_obj},
    {"reserveobj", l_reserveobj},
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/

static void preset_environment(lua_State * L, const parm_struct * p)
{
    int i;
    assert(L != NULL);
    lua_newtable(L);            /* t */
    for (i = 1, ++p; p->name != NULL; i++, p++) {
        assert(i == (int) p->idx);
        lua_pushstring(L, p->name);     /* k t */
        lua_pushinteger(L, (int) p->idx);       /* v k t */
        lua_settable(L, -3);    /* t */
    }
    lua_replace(L, LUA_ENVIRONINDEX);   /* - */
}

int luaopen_pdf(lua_State * L)
{
    preset_environment(L, obj_parms);
    luaL_register(L, "pdf", pdflib);
    /* build meta table */
    luaL_newmetatable(L, "pdf_meta");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, getpdf);
    /* do these later, NYI */
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, setpdf);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);    /* meta to itself */
    return 1;
}
