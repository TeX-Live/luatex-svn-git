% pdfluaapi.w
%
% Copyright 2010-2011 Taco Hoekwater <taco@@luatex.org>
% Copyright 2010-2011 Hartmut Henkel <hartmut@@luatex.org>

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

/**********************************************************************/

#define M_objBool   "objBool"
#define M_objInt    "objInt"
#define M_objReal   "objReal"
#define M_objString "objString"
#define M_objName   "objName"
#define M_objNull   "objNull"
#define M_objArray  "objArray"
#define M_objDict   "objDict"
#define M_objStream "objStream"
#define M_objRef    "objRef"

typedef struct {
    int num;                    /* object number, or zero */
    union {
        boolean b;
        int i;
        lua_Number f;
    } data;
} pdfobj;

#define pdf_objBool   pdfobj
#define pdf_objInt    pdfobj

typedef struct {
    int num;                    /* object number, or zero */
    pdffloat f;
} pdf_objReal;

#define pdf_objString pdfobj
#define pdf_objName   pdfobj
#define pdf_objNull   pdfobj
#define pdf_objArray  pdfobj

typedef struct {
    int num;                    /* object number, or zero */
    int i;                      /* Lua table reference */
    int attr;                   /* Lua attributes string reference (should get obsolescent) */
} pdf_objDict;

#define pdf_objStream pdfobj
#define pdf_objRef    pdfobj

/**********************************************************************/

#define m_getobjnum(type)                               \
static int m_##type##_getobjnum(lua_State * L)          \
{                                                       \
    pdf_##type *a;                                      \
    a = (pdf_##type *) luaL_checkudata(L, 1, M_##type); \
    lua_pushinteger(L, a->num);                         \
    return 1;                                           \
}

#define m_setobjnum(type)                               \
static int m_##type##_setobjnum(lua_State * L)          \
{                                                       \
    pdf_##type *a;                                      \
    a = (pdf_##type *) luaL_checkudata(L, 1, M_##type); \
    a->num = (int) luaL_checkinteger(L, 2);             \
    return 0;                                           \
}

#define m__tostring(type)                               \
static int m_##type##__tostring(lua_State * L)          \
{                                                       \
    pdf_##type *a;                                      \
    a = (pdf_##type *) luaL_checkudata(L, 1, M_##type); \
    lua_pushstring(L, #type);                           \
    return 1;                                           \
}

/**********************************************************************/
/* objBool */

static int l_new_objBool(lua_State * L)
{
    pdf_objBool *a;
    int i = lua_gettop(L), j;
    a = (pdf_objBool *) lua_newuserdata(L, sizeof(pdf_objBool));        /* o */
    a->num = 0;
    luaL_getmetatable(L, M_objBool);    /* m o */
    lua_setmetatable(L, -2);    /* o */
    switch (i) {
    case 0:
        a->data.b = false;
        break;
    case 1:
        j = lua_toboolean(L, 1);
        a->data.b = j ? true : false;
        break;
    default:
        luaL_error(L, "newBool() needs maximum 1 argument");
    }
    return 1;
}

static int m_objBool_get(lua_State * L)
{
    pdf_objBool *a;
    a = (pdf_objBool *) luaL_checkudata(L, 1, M_objBool);
    lua_pushboolean(L, a->data.b ? 1 : 0);
    return 1;
}

static int m_objBool_set(lua_State * L)
{
    pdf_objBool *a;
    int j;
    a = (pdf_objBool *) luaL_checkudata(L, 1, M_objBool);       /* ... b? o */
    j = lua_toboolean(L, 2);
    a->data.b = j ? true : false;
    return 0;
}

m_getobjnum(objBool);
m_setobjnum(objBool);

static int m_objBool_pdfout(lua_State * L)
{
    pdf_objBool *a;
    a = (pdf_objBool *) luaL_checkudata(L, 1, M_objBool);       /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    if (a->data.b)
        pdf_puts(static_pdf, "true");
    else
        pdf_puts(static_pdf, "false");
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objBool);

static int m_objBool__gc(lua_State * L)
{
    pdf_objBool *a;
    a = (pdf_objBool *) luaL_checkudata(L, 1, M_objBool);
    return 0;
}

static const struct luaL_Reg objBool_m[] = {
    {"get", m_objBool_get},     /* */
    {"set", m_objBool_set},     /* */
    {"getobjnum", m_objBool_getobjnum}, /* */
    {"setobjnum", m_objBool_setobjnum}, /* */
    {"pdfout", m_objBool_pdfout},       /* */
    {"__tostring", m_objBool__tostring},        /* */
    {"__gc", m_objBool__gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objInt */

static int l_new_objInt(lua_State * L)
{
    pdf_objInt *a;
    int i = lua_gettop(L);
    a = (pdf_objInt *) lua_newuserdata(L, sizeof(pdf_objInt));  /* o */
    a->num = 0;
    luaL_getmetatable(L, M_objInt);     /* m o */
    lua_setmetatable(L, -2);    /* o */
    switch (i) {
    case 0:
        a->data.i = 0;
        break;
    case 1:
        a->data.i = (int) luaL_checkinteger(L, 1);
        break;
    default:
        luaL_error(L, "newInt() needs maximum 1 argument");
    }
    return 1;
}

static int m_objInt_get(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt);
    lua_pushinteger(L, a->data.i);
    return 1;
}

static int m_objInt_set(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt); /* i? o */
    a->data.i = (int) luaL_checkinteger(L, 2);  /* i o */
    return 0;
}

m_getobjnum(objInt);
m_setobjnum(objInt);

static int m_objInt_incr(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt); /* o */
    (a->data.i)++;              /* o */
    return 0;
}

static int m_objInt_decr(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt); /* o */
    (a->data.i)--;              /* o */
    return 0;
}

static int m_objInt_pdfout(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt); /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    pdf_print_int(static_pdf, (longinteger) a->data.i);
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objInt);

static int m_objInt__gc(lua_State * L)
{
    pdf_objInt *a;
    a = (pdf_objInt *) luaL_checkudata(L, 1, M_objInt);
    return 0;
}

static const struct luaL_Reg objInt_m[] = {
    {"get", m_objInt_get},      /* */
    {"set", m_objInt_set},      /* */
    {"getobjnum", m_objInt_getobjnum},  /* */
    {"setobjnum", m_objInt_setobjnum},  /* */
    {"incr", m_objInt_incr},    /* */
    {"decr", m_objInt_decr},    /* */
    {"pdfout", m_objInt_pdfout},        /* */
    {"__tostring", m_objInt__tostring}, /* */
    {"__gc", m_objInt__gc},     /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objReal */

static int l_new_objReal(lua_State * L)
{
    pdf_objReal *a;
    int i = lua_gettop(L);
    a = (pdf_objReal *) lua_newuserdata(L, sizeof(pdf_objReal));        /* o */
    a->num = 0;
    luaL_getmetatable(L, M_objReal);    /* m o */
    lua_setmetatable(L, -2);    /* o */
    switch (i) {
    case 0:
        a->f.m = 0;             /* the integer zero */
        a->f.e = 0;
        break;
    case 2:
        a->f.m = (int) luaL_checkinteger(L, 1);
        a->f.e = (int) luaL_checkinteger(L, 2);
        break;
    default:
        luaL_error(L, "newReal() needs 0 or 2 arguments");
    }
    return 1;
}

static int m_objReal_get(lua_State * L)
{
    pdf_objReal *a;
    a = (pdf_objReal *) luaL_checkudata(L, 1, M_objReal);
    lua_pushnumber(L, a->f.m);
    return 1;
}

static int m_objReal_set(lua_State * L)
{
    pdf_objReal *a;
    a = (pdf_objReal *) luaL_checkudata(L, 1, M_objReal);       /* ... b? o */
    a->f.m = lua_tonumber(L, 2);
    return 0;
}

m_getobjnum(objReal);
m_setobjnum(objReal);

static int m_objReal_pdfout(lua_State * L)
{
    pdf_objReal *a;
    a = (pdf_objReal *) luaL_checkudata(L, 1, M_objReal);       /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    print_pdffloat(static_pdf, a->f);
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objReal);

static int m_objReal__gc(lua_State * L)
{
    pdf_objReal *a;
    a = (pdf_objReal *) luaL_checkudata(L, 1, M_objReal);
    return 0;
}

static const struct luaL_Reg objReal_m[] = {
    {"get", m_objReal_get},     /* */
    {"set", m_objReal_set},     /* */
    {"getobjnum", m_objReal_getobjnum}, /* */
    {"setobjnum", m_objReal_setobjnum}, /* */
    {"pdfout", m_objReal_pdfout},       /* */
    {"__tostring", m_objReal__tostring},        /* */
    {"__gc", m_objReal__gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objString */

static int l_new_objString(lua_State * L)
{
    pdf_objString *a;
    int i = lua_gettop(L);      /* (t) */
    a = (pdf_objString *) lua_newuserdata(L, sizeof(pdf_objString));    /* o (s) */
    a->num = 0;
    luaL_getmetatable(L, M_objString);  /* m o s */
    lua_setmetatable(L, -2);    /* o s */
    switch (i) {
    case 0:
        a->data.i = LUA_NOREF;  /* o */
        break;
    case 1:
        luaL_checkstring(L, 1); /* o s */
        lua_pushvalue(L, 1);    /* s o s */
        a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);        /* o s */
        break;
    default:
        luaL_error(L, "newString() needs maximum 1 argument");
    }
    return 1;
}

static int m_objString_get(lua_State * L)
{
    pdf_objString *a;
    a = (pdf_objString *) luaL_checkudata(L, 1, M_objString);
    lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);
    return 1;
}

static int m_objString_set(lua_State * L)
{
    pdf_objString *a;
    a = (pdf_objString *) luaL_checkudata(L, 1, M_objString);   /* ... s? o */
    if (a->data.i != LUA_NOREF) {
        luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
        a->data.i = LUA_NOREF;
    }
    luaL_checkstring(L, 1);     /* ... s o */
    lua_pushvalue(L, 2);        /* s ... s o */
    a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);    /* ... s o */
    return 0;
}

m_getobjnum(objString);
m_setobjnum(objString);

static int m_objString_pdfout(lua_State * L)
{
    pdf_objString *a;
    const_lstring st;
    a = (pdf_objString *) luaL_checkudata(L, 1, M_objString);   /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    if (a->data.i != LUA_NOREF) {
        lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);    /* s? o */
        if (lua_isstring(L, -1)) {      /* s o */
            st.s = lua_tolstring(L, -1, &st.l);
            pdf_out_block(static_pdf, st.s, st.l);
        }
    }
    if (a->num != 0) {
        pdf_out(static_pdf, '\n');
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objString);

static int m_objString__gc(lua_State * L)
{
    pdf_objString *a;
    a = (pdf_objString *) luaL_checkudata(L, 1, M_objString);
    luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
    return 0;
}

static const struct luaL_Reg objString_m[] = {
    {"get", m_objString_get},   /* */
    {"set", m_objString_set},   /* */
    {"getobjnum", m_objString_getobjnum},       /* */
    {"setobjnum", m_objString_setobjnum},       /* */
    {"pdfout", m_objString_pdfout},     /* */
    {"__tostring", m_objString__tostring},      /* */
    {"__gc", m_objString__gc},  /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objName */

static int l_new_objName(lua_State * L)
{
    pdf_objName *a;
    int i = lua_gettop(L);      /* (t) */
    a = (pdf_objName *) lua_newuserdata(L, sizeof(pdf_objName));        /* o (s) */
    a->num = 0;
    luaL_getmetatable(L, M_objName);    /* m o s */
    lua_setmetatable(L, -2);    /* o s */
    switch (i) {
    case 0:
        a->data.i = LUA_NOREF;  /* o */
        break;
    case 1:
        luaL_checkstring(L, 1); /* o s */
        lua_pushvalue(L, 1);    /* s o s */
        a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);        /* o s */
        break;
    default:
        luaL_error(L, "newName() needs maximum 1 argument");
    }
    return 1;
}

static int m_objName_get(lua_State * L)
{
    pdf_objName *a;
    a = (pdf_objName *) luaL_checkudata(L, 1, M_objName);
    lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);
    return 1;
}

static int m_objName_set(lua_State * L)
{
    pdf_objName *a;
    a = (pdf_objName *) luaL_checkudata(L, 1, M_objName);       /* ... s? o */
    if (a->data.i != LUA_NOREF) {
        luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
        a->data.i = LUA_NOREF;
    }
    luaL_checkstring(L, 1);     /* ... s o */
    lua_pushvalue(L, 2);        /* s ... s o */
    a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);    /* ... s o */
    return 0;
}

m_getobjnum(objName);
m_setobjnum(objName);

static int m_objName_pdfout(lua_State * L)
{
    pdf_objName *a;
    const_lstring st;
    a = (pdf_objName *) luaL_checkudata(L, 1, M_objName);       /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    assert(a->data.i != LUA_NOREF);
    lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);        /* s o */
    st.s = lua_tolstring(L, -1, &st.l);
    pdf_out(static_pdf, '/');
    pdf_puts(static_pdf, st.s);
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objName);

static int m_objName__gc(lua_State * L)
{
    pdf_objName *a;
    a = (pdf_objName *) luaL_checkudata(L, 1, M_objName);
    luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
    return 0;
}

static const struct luaL_Reg objName_m[] = {
    {"get", m_objName_get},     /* */
    {"set", m_objName_set},     /* */
    {"getobjnum", m_objName_getobjnum}, /* */
    {"setobjnum", m_objName_setobjnum}, /* */
    {"pdfout", m_objName_pdfout},       /* */
    {"__tostring", m_objName__tostring},        /* */
    {"__gc", m_objName__gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objNull */

static int l_new_objNull(lua_State * L)
{
    pdf_objNull *a;
    a = (pdf_objNull *) lua_newuserdata(L, sizeof(pdf_objNull));        /* o */
    a->num = 0;
    luaL_getmetatable(L, M_objNull);    /* m o */
    lua_setmetatable(L, -2);    /* o */
    a->data.i = 0;
    return 1;
}

m_getobjnum(objNull);
m_setobjnum(objNull);

m__tostring(objNull);

static int m_objNull__gc(lua_State * L)
{
    pdf_objNull *a;
    a = (pdf_objNull *) luaL_checkudata(L, 1, M_objNull);
    return 0;
}

static const struct luaL_Reg objNull_m[] = {
    {"getobjnum", m_objNull_getobjnum}, /* */
    {"setobjnum", m_objNull_setobjnum}, /* */
    {"__tostring", m_objNull__tostring},        /* */
    {"__gc", m_objNull__gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objArray */

static int l_new_objArray(lua_State * L)
{
    pdf_objArray *a;
    int i = lua_gettop(L);      /* (t) */
    a = (pdf_objArray *) lua_newuserdata(L, sizeof(pdf_objArray));      /* o (t) */
    a->num = 0;
    a->data.i = LUA_NOREF;
    luaL_getmetatable(L, M_objArray);   /* m o (t) */
    lua_setmetatable(L, -2);    /* o (t) */
    switch (i) {
    case 0:
        break;                  /* o */
    case 1:
        luaL_checktype(L, 1, LUA_TTABLE);       /* o t */
        lua_pushvalue(L, 1);    /* t o t */
        a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);        /* o t */
        break;
    default:
        luaL_error(L, "newArray() needs maximum 1 argument");
    }
    return 1;
}

static int m_objArray_get(lua_State * L)
{
    pdf_objArray *a;
    a = (pdf_objArray *) luaL_checkudata(L, 1, M_objArray);
    lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);
    return 1;
}

static int m_objArray_set(lua_State * L)
{
    pdf_objArray *a;
    a = (pdf_objArray *) luaL_checkudata(L, 1, M_objArray);     /* ... t? o */
    if (a->data.i != LUA_NOREF) {
        luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
        a->data.i = LUA_NOREF;
    }
    luaL_checktype(L, 2, LUA_TTABLE);   /* ... t o */
    lua_pushvalue(L, 2);        /* t ... t o */
    a->data.i = (int) luaL_ref(L, LUA_GLOBALSINDEX);    /* ... t o */
    return 0;
}

m_getobjnum(objArray);
m_setobjnum(objArray);

static int m_objArray_pdfout(lua_State * L)
{
    pdf_objArray *a;
    int i;
    boolean first = true;
    a = (pdf_objArray *) luaL_checkudata(L, 1, M_objArray);     /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    pdf_out(static_pdf, '[');
    if (a->data.i != LUA_NOREF) {
        lua_rawgeti(L, LUA_GLOBALSINDEX, a->data.i);    /* t o */
        lua_pushnil(L);         /* n t o */
        while (lua_next(L, -2) != 0) {
            if (!first)
                pdf_out(static_pdf, ' ');
            i = luaL_callmeta(L, -1, "pdfout");
            if (i == 1)
                lua_pop(L, 1);
            lua_pop(L, 1);
            first = false;
        }
    }
    pdf_out(static_pdf, ']');
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objArray);

static int m_objArray__gc(lua_State * L)
{
    pdf_objArray *a;
    a = (pdf_objArray *) luaL_checkudata(L, 1, M_objArray);
    luaL_unref(L, LUA_GLOBALSINDEX, a->data.i);
    return 0;
}

static const struct luaL_Reg objArray_m[] = {
    {"get", m_objArray_get},    /* */
    {"set", m_objArray_set},    /* */
    {"getobjnum", m_objArray_getobjnum},        /* */
    {"setobjnum", m_objArray_setobjnum},        /* */
    {"pdfout", m_objArray_pdfout},      /* */
    {"__tostring", m_objArray__tostring},       /* */
    {"__gc", m_objArray__gc},   /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objDict */

static int l_new_objDict(lua_State * L)
{
    pdf_objDict *a;
    int i = lua_gettop(L);      /* (t) */
    a = (pdf_objDict *) lua_newuserdata(L, sizeof(pdf_objDict));        /* o (t) */
    a->num = 0;
    a->i = LUA_NOREF;
    a->attr = LUA_NOREF;
    luaL_getmetatable(L, M_objDict);    /* m o (t) */
    lua_setmetatable(L, -2);    /* o (t) */
    switch (i) {
    case 0:
        break;                  /* o */
    case 1:
        luaL_checktype(L, 1, LUA_TTABLE);       /* o t */
        lua_pushvalue(L, 1);    /* t o t */
        a->i = (int) luaL_ref(L, LUA_GLOBALSINDEX);     /* o t */
        break;
    default:
        luaL_error(L, "newDict() needs maximum 1 argument");
    }
    return 1;
}

static int m_objDict_get(lua_State * L)
{
    pdf_objDict *a;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);       /* o */
    lua_rawgeti(L, LUA_GLOBALSINDEX, a->i);     /* t o */
    return 1;                   /* t o */
}

static int m_objDict_set(lua_State * L)
{
    pdf_objDict *a;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);       /* ... t? o */
    if (a->i != LUA_NOREF) {
        luaL_unref(L, LUA_GLOBALSINDEX, a->i);
        a->i = LUA_NOREF;
    }
    luaL_checktype(L, 2, LUA_TTABLE);   /* ... t o */
    lua_pushvalue(L, 2);        /* t ... t o */
    a->i = (int) luaL_ref(L, LUA_GLOBALSINDEX); /* ... t o */
    return 0;
}

m_getobjnum(objDict);
m_setobjnum(objDict);

static int m_objDict_getattr(lua_State * L)
{
    pdf_objDict *a;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);       /* o */
    if (a->attr != LUA_NOREF)
        lua_rawgeti(L, LUA_GLOBALSINDEX, a->attr);      /* a o */
    else
        lua_pushnil(L);         /* n o */
    return 1;
}

static int m_objDict_setattr(lua_State * L)
{
    pdf_objDict *a;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);       /* ... a o */
    lua_pushvalue(L, 2);        /* a ... a o */
    if (a->attr != LUA_NOREF) {
        luaL_unref(L, LUA_GLOBALSINDEX, a->attr);
        a->attr = LUA_NOREF;
    }
    if (lua_isnil(L, -1) == 0)
        a->attr = (int) luaL_ref(L, LUA_GLOBALSINDEX);  /* ... a o */
    return 0;
}

static int m_objDict_pdfout(lua_State * L)
{
    pdf_objDict *a;
    const_lstring st;
    int i;
    boolean first = true;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);       /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    pdf_puts(static_pdf, "<<");
    if (a->i != LUA_NOREF) {
        lua_rawgeti(L, LUA_GLOBALSINDEX, a->i); /* t o */
        lua_pushnil(L);         /* n t o */
        while (lua_next(L, -2) != 0) {
            st.s = lua_tolstring(L, -2, &st.l);
            if (!first)
                pdf_out(static_pdf, ' ');
            pdf_out(static_pdf, '/');
            pdf_puts(static_pdf, st.s);
            pdf_out(static_pdf, ' ');
            /* object on stack top */
            i = luaL_callmeta(L, -1, "pdfout");
            if (i == 1)
                lua_pop(L, 1);
            lua_pop(L, 1);
            first = false;
        }
    }
    if (a->attr != LUA_NOREF) { /* should get obsolescent */
        lua_rawgeti(L, LUA_GLOBALSINDEX, a->attr);
        if (lua_isstring(L, -1)) {
            pdf_puts(static_pdf, "\n");
            st.s = lua_tolstring(L, -1, &st.l);
            pdf_out_block(static_pdf, st.s, st.l);
        }
    }
    pdf_puts(static_pdf, ">>");
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objDict);

static int m_objDict__gc(lua_State * L)
{
    pdf_objDict *a;
    a = (pdf_objDict *) luaL_checkudata(L, 1, M_objDict);
    luaL_unref(L, LUA_GLOBALSINDEX, a->i);
    luaL_unref(L, LUA_GLOBALSINDEX, a->attr);
    return 0;
}

static const struct luaL_Reg objDict_m[] = {
    {"get", m_objDict_get},     /* */
    {"set", m_objDict_set},     /* */
    {"getobjnum", m_objDict_getobjnum}, /* */
    {"setobjnum", m_objDict_setobjnum}, /* */
    {"getattr", m_objDict_getattr},     /* */
    {"setattr", m_objDict_setattr},     /* */
    {"pdfout", m_objDict_pdfout},       /* */
    {"__tostring", m_objDict__tostring},        /* */
    {"__gc", m_objDict__gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objStream */

static int l_new_objStream(lua_State * L)
{
    assert(0);
    return 0;
}

m__tostring(objStream);

static int m_objStream__gc(lua_State * L)
{
    pdf_objStream *a;
    a = (pdf_objStream *) luaL_checkudata(L, 1, M_objStream);
    return 0;
}

static const struct luaL_Reg objStream_m[] = {
    {"__tostring", m_objStream__tostring},      /* */
    {"__gc", m_objStream__gc},  /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* objRef */

static int l_new_objRef(lua_State * L)
{
    pdf_objRef *a;
    int i = lua_gettop(L);
    a = (pdf_objRef *) lua_newuserdata(L, sizeof(pdf_objRef));  /* o */
    a->num = 0;
    luaL_getmetatable(L, M_objRef);     /* m o */
    lua_setmetatable(L, -2);    /* o */
    switch (i) {
    case 0:
        a->data.i = 0;
        break;
    case 1:
        a->data.i = (int) luaL_checkinteger(L, 1);
        break;
    default:
        luaL_error(L, "newRef() needs maximum 1 argument");
    }
    return 1;
}

static int m_objRef_get(lua_State * L)
{
    pdf_objRef *a;
    a = (pdf_objRef *) luaL_checkudata(L, 1, M_objRef);
    lua_pushinteger(L, a->data.i);
    return 1;
}

static int m_objRef_set(lua_State * L)
{
    pdf_objRef *a;
    a = (pdf_objRef *) luaL_checkudata(L, 1, M_objRef); /* ... b? o */
    a->data.i = lua_tointeger(L, 2);
    return 0;
}

m_getobjnum(objRef);
m_setobjnum(objRef);

static int m_objRef_pdfout(lua_State * L)
{
    pdf_objRef *a;
    a = (pdf_objRef *) luaL_checkudata(L, 1, M_objRef); /* o */
    if (a->num != 0)
        pdf_begin_obj(static_pdf, a->num, 1);
    pdf_printf(static_pdf, "%d 0 R", a->data.i);
    if (a->num != 0) {
        pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
    }
    return 0;
}

m__tostring(objRef);

static int m_objRef__gc(lua_State * L)
{
    pdf_objRef *a;
    a = (pdf_objRef *) luaL_checkudata(L, 1, M_objRef);
    return 0;
}

static const struct luaL_Reg objRef_m[] = {
    {"get", m_objRef_get},      /* */
    {"set", m_objRef_set},      /* */
    {"getobjnum", m_objRef_getobjnum},  /* */
    {"setobjnum", m_objRef_setobjnum},  /* */
    {"pdfout", m_objRef_pdfout},        /* */
    {"__tostring", m_objRef__tostring}, /* */
    {"__gc", m_objRef__gc},     /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* get stuff from the C world */

static int l_get_last_page(lua_State * L)
{
    lua_pushinteger(L, static_pdf->last_page);  /* i */
    return 1;
}

static int l_get_last_pages(lua_State * L)
{
    lua_pushinteger(L, static_pdf->last_pages); /* i */
    return 1;
}

static int l_get_total_pages(lua_State * L)
{
    lua_pushinteger(L, total_pages);    /* i */
    return 1;
}

static int l_get_last_stream(lua_State * L)
{
    lua_pushinteger(L, static_pdf->last_stream);        /* i */
    return 1;
}

static int l_get_last_resources(lua_State * L)
{
    lua_pushinteger(L, static_pdf->page_resources->last_resources);     /* i */
    return 1;
}

static int l_get_annots(lua_State * L)
{
    lua_pushinteger(L, static_pdf->annots);     /* i */
    return 1;
}

static int l_get_beads(lua_State * L)
{
    lua_pushinteger(L, static_pdf->beads);      /* i */
    return 1;
}

static int l_get_group(lua_State * L)
{
    lua_pushinteger(L, static_pdf->img_page_group_val); /* i */
    return 1;
}

static int l_get_cur_page_size_h(lua_State * L)
{
    pdffloat f;
    f = pdf_calc_mag_bp(static_pdf, cur_page_size.h);
    lua_pushinteger(L, f.m);    /* m */
    lua_pushinteger(L, f.e);    /* e m */
    return 2;
}

static int l_get_cur_page_size_v(lua_State * L)
{
    pdffloat f;
    f = pdf_calc_mag_bp(static_pdf, cur_page_size.v);
    lua_pushinteger(L, f.m);    /* m */
    lua_pushinteger(L, f.e);    /* e m */
    return 2;
}

#define pdf_page_attr equiv(pdf_page_attr_loc)

static int l_get_pdf_page_attr(lua_State * L)
{
    char *s;
    int l;
    s = tokenlist_to_cstring(pdf_page_attr, true, &l);
    lua_pushlstring(L, s, l);
    xfree(s);
    return 1;
}

static int l_get_threads(lua_State * L)
{
    lua_pushinteger(L, static_pdf->threads);    /* i */
    return 1;
}

static int l_get_outlines(lua_State * L)
{
    lua_pushinteger(L, static_pdf->outlines);   /* i */
    return 1;
}

static int l_get_names(lua_State * L)
{
    lua_pushinteger(L, static_pdf->namestree);  /* i */
    return 1;
}

static int l_get_catalog_openaction(lua_State * L)
{
    lua_pushinteger(L, static_pdf->catalog_openaction); /* i */
    return 1;
}

static int l_get_catalogtoks(lua_State * L)
{
    char *s;
    int l;
    s = tokenlist_to_cstring(static_pdf->catalog_toks, true, &l);
    delete_token_ref(static_pdf->catalog_toks);
    static_pdf->catalog_toks = null;
    lua_pushlstring(L, s, l);
    xfree(s);
    return 1;
}

/**********************************************************************/

static const struct luaL_Reg pdfobjlib[] = {
    {"newBool", l_new_objBool},
    {"newInt", l_new_objInt},
    {"newReal", l_new_objReal},
    {"newString", l_new_objString},
    {"newName", l_new_objName},
    {"newNull", l_new_objNull},
    {"newArray", l_new_objArray},
    {"newDict", l_new_objDict},
    {"newStream", l_new_objStream},
    {"newRef", l_new_objRef},
    /* get info needed for page dict */
    {"get_last_page", l_get_last_page},
    {"get_total_pages", l_get_total_pages},
    {"get_last_stream", l_get_last_stream},
    {"get_last_resources", l_get_last_resources},
    {"get_annots", l_get_annots},
    {"get_beads", l_get_beads},
    {"get_group", l_get_group},
    {"get_cur_page_size_h", l_get_cur_page_size_h},
    {"get_cur_page_size_v", l_get_cur_page_size_v},
    {"get_pdf_page_attr", l_get_pdf_page_attr},
    /* get info needed for catalog dict */
    {"get_last_pages", l_get_last_pages},
    {"get_threads", l_get_threads},
    {"get_outlines", l_get_outlines},
    {"get_names", l_get_names},
    {"get_catalog_openaction", l_get_catalog_openaction},
    {"get_catalogtoks", l_get_catalogtoks},
    {NULL, NULL}                /* sentinel */
};

#define register_meta(type)                 \
    luaL_newmetatable(L, M_##type);         \
    lua_pushvalue(L, -1);                   \
    lua_setfield(L, -2, "__index");         \
    lua_pushstring(L, "no user access");    \
    lua_setfield(L, -2, "__metatable");     \
    luaL_register(L, NULL, type##_m)

int luaopen_pdfobj(lua_State * L)
{
    register_meta(objBool);
    register_meta(objInt);
    register_meta(objReal);
    register_meta(objString);
    register_meta(objName);
    register_meta(objNull);
    register_meta(objArray);
    register_meta(objDict);
    register_meta(objStream);
    register_meta(objRef);

    luaL_register(L, "pdfobj", pdfobjlib);
    return 1;
}

/**********************************************************************/

int new_pdflua(void)
{
    int err, i, i1, i2;
    Byte *uncompr;              /* ... */
    zlib_struct *zp = (zlib_struct *) pdflua_zlib_struct_ptr;
    uLong uncomprLen = zp->uncomprLen;
    i1 = lua_gettop(Luas);
    if ((uncompr = xtalloc(zp->uncomprLen, Byte)) == NULL)
        pdftex_fail("new_pdflua(): xtalloc()");
    err = uncompress(uncompr, &uncomprLen, zp->compr, zp->comprLen);
    if (err != Z_OK)
        pdftex_fail("new_pdflua(): uncompress()");
    assert(uncomprLen == zp->uncomprLen);
    if (luaL_loadbuffer(Luas, (const char *) uncompr, uncomprLen, "pdflua")
        || lua_pcall(Luas, 0, 1, 0))
        pdftex_fail("new_pdflua(): lua_pcall()");
    luaL_checktype(Luas, -1, LUA_TTABLE);       /* t ... */
    i = luaL_ref(Luas, LUA_GLOBALSINDEX);       /* ... */
    xfree(uncompr);
    i2 = lua_gettop(Luas);
    assert(i1 == i2);
    return i;
}

void pdflua_make_pagedict(PDF pdf)
{
    int err, i1, i2;
    i1 = lua_gettop(Luas);      /* ... */
    lua_rawgeti(Luas, LUA_GLOBALSINDEX, pdf->pdflua_ref);       /* t ... */
    lua_pushstring(Luas, "makepagedict");       /* s t ... */
    lua_gettable(Luas, -2);     /* f t ... */
    err = lua_pcall(Luas, 0, 0, 0);     /* (e) t ... */
    if (err != 0)
        pdftex_fail("pdflua.lua: makepagedict()");
    /* t ... */
    lua_pop(Luas, 1);           /* ... */
    i2 = lua_gettop(Luas);
    assert(i1 == i2);
}

void pdflua_output_pages_tree(PDF pdf)
{
    int err, i1, i2;            /* ... */
    i1 = lua_gettop(Luas);
    lua_rawgeti(Luas, LUA_GLOBALSINDEX, pdf->pdflua_ref);       /* t ... */
    lua_pushstring(Luas, "outputpagestree");    /* s t ... */
    lua_gettable(Luas, -2);     /* f t ... */
    err = lua_pcall(Luas, 0, 1, 0);     /* (e) i t ... */
    if (err != 0)
        pdftex_fail("pdflua.lua: outputpagestree()");
    /* i t ... */
    pdf->last_pages = (int) luaL_checkinteger(Luas, -1);        /* i t ... */
    lua_pop(Luas, 2);           /* ... */
    i2 = lua_gettop(Luas);
    assert(i1 == i2);
}

int pdflua_make_catalog(PDF pdf)
{
    int err, i1, i2, root_objnum;
    i1 = lua_gettop(Luas);      /* ... */
    lua_rawgeti(Luas, LUA_GLOBALSINDEX, pdf->pdflua_ref);       /* t ... */
    lua_pushstring(Luas, "makecatalog");        /* s t ... */
    lua_gettable(Luas, -2);     /* f t ... */
    err = lua_pcall(Luas, 0, 1, 0);     /* (e) t ... */
    if (err != 0)
        pdftex_fail("pdflua.lua: makecatalog()");
    /* i t ... */
    root_objnum = (int) luaL_checkinteger(Luas, -1);    /* i t ... */
    lua_pop(Luas, 2);           /* ... */
    i2 = lua_gettop(Luas);
    assert(i1 == i2);
    return root_objnum;
}
