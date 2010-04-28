/* lepdflib.cc

   Copyright 2009-2010 Taco Hoekwater <taco@luatex.org>
   Copyright 2009-2010 Hartmut Henkel <hartmut@luatex.org>

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

#include "image/epdf.h"

#define ALPHA_TEST

#ifdef ALPHA_TEST
// define DEBUG

//**********************************************************************
// TODO: add more xpdf functions (many are still missing)

//**********************************************************************
// objects allocated by xpdf may not be deleted in the lepdflib

typedef enum { ALLOC_XPDF, ALLOC_LEPDF } alloctype;

typedef struct {
    void *d;
    alloctype atype;            // was it allocated by XPDF or the lepdflib.cc?
} udstruct;

const char *ErrorCodeNames[] = { "None", "OpenFile", "BadCatalog",
    "Damaged", "Encrypted", "HighlightFile", "BadPrinter", "Printing",
    "Permission", "BadPageNum", "FileIO"
};

//**********************************************************************

#  define M_Array        "Array"
#  define M_Catalog      "Catalog"
#  define M_Dict         "Dict"
#  define M_GString      "GString"
#  define M_LinkDest     "LinkDest"
#  define M_Object       "Object"
#  define M_ObjectStream "ObjectStream"
#  define M_Page         "Page"
#  define M_PDFDoc       "PDFDoc"
#  define M_PDFRectangle "PDFRectangle"
#  define M_Ref          "Ref"
#  define M_Stream       "Stream"
#  define M_XRef         "XRef"
#  define M_XRefEntry    "XRefEntry"

//**********************************************************************

#  define new_XPDF_userdata(type)                                               \
udstruct *new_##type##_userdata(lua_State * L)                                  \
{                                                                               \
    udstruct *a;                                                                \
    a = (udstruct *) lua_newuserdata(L, sizeof(udstruct));  /* udstruct ... */  \
    a->atype = ALLOC_XPDF;                                                      \
    luaL_getmetatable(L, M_##type);     /* m udstruct ... */                    \
    lua_setmetatable(L, -2);    /* udstruct ... */                              \
    return a;                                                                   \
}

new_XPDF_userdata(PDFDoc);

new_XPDF_userdata(Array);
new_XPDF_userdata(Catalog);
new_XPDF_userdata(Dict);
new_XPDF_userdata(GString);
new_XPDF_userdata(LinkDest);
new_XPDF_userdata(Object);
new_XPDF_userdata(ObjectStream);
new_XPDF_userdata(Page);
new_XPDF_userdata(PDFRectangle);
new_XPDF_userdata(Ref);
new_XPDF_userdata(Stream);
new_XPDF_userdata(XRef);
new_XPDF_userdata(XRefEntry);

//**********************************************************************

int l_open_PDFDoc(lua_State * L)
{
    const char *file_path;
    udstruct *uout;
    PdfDocument *d;
    file_path = luaL_checkstring(L, 1); // path
    d = refPdfDocument((char *) file_path, FE_RETURN_NULL);
    if (d == NULL)
        lua_pushnil(L);
    else {
        uout = new_PDFDoc_userdata(L);
        uout->d = d;
        uout->atype = ALLOC_LEPDF;
    }
    return 1;                   // doc path
}

static const struct luaL_Reg epdflib[] = {
    {"open", l_open_PDFDoc},
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************

#  define m_XPDF_get_XPDF(in, out, function)                            \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    udstruct *uin, *uout;                                               \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    uout = new_##out##_userdata(L);                                     \
    uout->d = ((in *) uin->d)->function();                              \
    return 1;                                                           \
}

#  define m_XPDF_get_BOOL(in, function)                                 \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    udstruct *uin;                                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    if (((in *) uin->d)->function())                                    \
        lua_pushboolean(L, 1);                                          \
    else                                                                \
        lua_pushboolean(L, 0);                                          \
    return 1;                                                           \
}

#  define m_XPDF_get_INT(in, function)                                  \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    int i;                                                              \
    udstruct *uin;                                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    i = (int) ((in *) uin->d)->function();                              \
    lua_pushinteger(L, i);                                              \
    return 1;                                                           \
}

#  define m_XPDF_get_DOUBLE(in, function)                               \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    double d;                                                           \
    udstruct *uin;                                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    d = (double) ((in *) uin->d)->function();                           \
    lua_pushnumber(L, d);                                               \
    return 1;                                                           \
}

#  define m_XPDF_get_OBJECT(in, function)                               \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    udstruct *uin, *uout;                                               \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    uout = new_Object_userdata(L);                                      \
    uout->d = new Object();                                             \
    ((in *) uin->d)->function((Object *) uout->d);                      \
    uout->atype = ALLOC_LEPDF;                                          \
    return 1;                                                           \
}

#  define m_XPDF_INT_get_XPDF(in, out, function)                        \
int m_##in##_##function(lua_State * L)                                  \
{                                                                       \
    int i;                                                              \
    udstruct *uin, *uout;                                               \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);                   \
    i = luaL_checkint(L, 2);                                            \
    uout = new_##out##_userdata(L);                                     \
    uout->d = ((in *) uin->d)->function(i);                             \
    return 1;                                                           \
}

#  define m_XPDF__tostring(type)                                        \
int m_##type##__tostring(lua_State * L)                                 \
{                                                                       \
    udstruct *uin;                                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##type);                 \
    lua_pushfstring(L, "%s: %p", #type, (type *) uin->d);               \
    return 1;                                                           \
}

//**********************************************************************
// Array

m_XPDF_get_INT(Array, getLength);

int m_Array_get(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Array);
    i = luaL_checkint(L, 2);
    len = ((Array *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Array *) uin->d)->get(i - 1, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Array_getNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Array);
    i = luaL_checkint(L, 2);
    len = ((Array *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Array *) uin->d)->getNF(i - 1, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

m_XPDF__tostring(Array);

static const struct luaL_Reg Array_m[] = {
    {"getLength", m_Array_getLength},   /* */
    {"get", m_Array_get},       /* */
    {"getNF", m_Array_getNF},   /* */
    {"__tostring", m_Array__tostring},  /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Catalog

m_XPDF_get_BOOL(Catalog, isOk);
m_XPDF_get_INT(Catalog, getNumPages);

int m_Catalog_getPage(lua_State * L)
{
    int i, pages;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    i = luaL_checkint(L, 2);
    pages = ((Catalog *) uin->d)->getNumPages();
    if (i > 0 && i <= pages) {
        uout = new_Page_userdata(L);
        uout->d = ((Catalog *) uin->d)->getPage(i);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Catalog_getPageRef(lua_State * L)
{
    int i, pages;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    i = luaL_checkint(L, 2);
    pages = ((Catalog *) uin->d)->getNumPages();
    if (i > 0 && i <= pages) {
        uout = new_Ref_userdata(L);
        uout->d = (Ref *) gmalloc(sizeof(Ref));
        ((Ref *) uout->d)->num = ((Catalog *) uin->d)->getPageRef(i)->num;
        ((Ref *) uout->d)->gen = ((Catalog *) uin->d)->getPageRef(i)->gen;
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Catalog_getBaseURI(lua_State * L)
{
    GString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    gs = ((Catalog *) uin->d)->getBaseURI();
    if (gs != NULL)
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    else
        lua_pushnil(L);
    return 1;
}

int m_Catalog_readMetadata(lua_State * L)
{
    GString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    gs = ((Catalog *) uin->d)->readMetadata();
    if (gs != NULL)
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    else
        lua_pushnil(L);
    return 1;
}

m_XPDF_get_XPDF(Catalog, Object, getStructTreeRoot);

int m_Catalog_findPage(lua_State * L)
{
    int num, gen, i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    num = luaL_checkint(L, 2);
    gen = luaL_checkint(L, 3);
    i = ((Catalog *) uin->d)->findPage(num, gen);
    if (i > 0)
        lua_pushinteger(L, i);
    else
        lua_pushnil(L);
    return 1;
}

int m_Catalog_findDest(lua_State * L)
{
    GString *name;
    LinkDest *dest;
    const char *s;
    size_t len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    s = luaL_checklstring(L, 2, &len);
    name = new GString(s, len);
    dest = ((Catalog *) uin->d)->findDest(name);
    if (dest != NULL) {
        uout = new_LinkDest_userdata(L);
        uout->d = dest;
    } else
        lua_pushnil(L);
    delete name;
    return 1;
}

m_XPDF_get_XPDF(Catalog, Object, getDests);
m_XPDF_get_XPDF(Catalog, Object, getNameTree);
m_XPDF_get_XPDF(Catalog, Object, getOutline);
m_XPDF_get_XPDF(Catalog, Object, getAcroForm);

m_XPDF__tostring(Catalog);

static const struct luaL_Reg Catalog_m[] = {
    {"isOk", m_Catalog_isOk},   /* */
    {"getNumPages", m_Catalog_getNumPages},     /* */
    {"getPage", m_Catalog_getPage},     /* */
    {"getPageRef", m_Catalog_getPageRef},       /* */
    {"getBaseURI", m_Catalog_getBaseURI},       /* */
    {"readMetadata", m_Catalog_readMetadata},   /* */
    {"getStructTreeRoot", m_Catalog_getStructTreeRoot}, /* */
    {"findPage", m_Catalog_findPage},   /* */
    {"findDest", m_Catalog_findDest},   /* */
    {"getDests", m_Catalog_getDests},   /* */
    {"getNameTree", m_Catalog_getNameTree},     /* */
    {"getOutline", m_Catalog_getOutline},       /* */
    {"getAcroForm", m_Catalog_getAcroForm},     /* */
    {"__tostring", m_Catalog__tostring},        /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Dict

m_XPDF_get_INT(Dict, getLength);

int m_Dict_is(lua_State * L)
{
    const char *s;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    s = luaL_checkstring(L, 2);
    if (((Dict *) uin->d)->is((char *) s))
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);
    return 1;
}

int m_Dict_lookup(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    s = luaL_checkstring(L, 2);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    ((Dict *) uin->d)->lookup((char *) s, (Object *) uout->d);
    uout->atype = ALLOC_LEPDF;
    return 1;
}

int m_Dict_lookupNF(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    s = luaL_checkstring(L, 2);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    ((Dict *) uin->d)->lookupNF((char *) s, (Object *) uout->d);
    uout->atype = ALLOC_LEPDF;
    return 1;
}

int m_Dict_getKey(lua_State * L)
{
    int i, len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    i = luaL_checkint(L, 2);
    len = ((Dict *) uin->d)->getLength();
    if (i > 0 && i <= len)
        lua_pushstring(L, ((Dict *) uin->d)->getKey(i - 1));
    else
        lua_pushnil(L);
    return 1;
}

int m_Dict_getVal(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    i = luaL_checkint(L, 2);
    len = ((Dict *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Dict *) uin->d)->getVal(i - 1, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Dict_getValNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    i = luaL_checkint(L, 2);
    len = ((Dict *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Dict *) uin->d)->getValNF(i - 1, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

m_XPDF__tostring(Dict);

const struct luaL_Reg Dict_m[] = {
    {"getLength", m_Dict_getLength},    /* */
    {"is", m_Dict_is},          /* */
    {"lookup", m_Dict_lookup},  /* */
    {"lookupNF", m_Dict_lookupNF},      /* */
    {"getKey", m_Dict_getKey},  /* */
    {"getVal", m_Dict_getVal},  /* */
    {"getValNF", m_Dict_getValNF},      /* */
    {"__tostring", m_Dict__tostring},   /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// GString

int m_GString__tostring(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_GString);
    lua_pushlstring(L, ((GString *) uin->d)->getCString(),
                    ((GString *) uin->d)->getLength());
    return 1;
}

static const struct luaL_Reg GString_m[] = {
    {"__tostring", m_GString__tostring},        /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// LinkDest

const char *LinkDestKindNames[] =
    { "XYZ", "Fit", "FitH", "FitV", "FitR", "FitB", "FitBH", "FitBV", NULL };

m_XPDF_get_BOOL(LinkDest, isOk);

int m_LinkDest_getKind(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_LinkDest);
    i = (int) ((LinkDest *) uin->d)->getKind();
    lua_pushinteger(L, i);
    return 1;
}

int m_LinkDest_getKindName(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_LinkDest);
    i = (int) ((LinkDest *) uin->d)->getKind();
    lua_pushstring(L, LinkDestKindNames[i]);
    return 1;
}

m_XPDF_get_BOOL(LinkDest, isPageRef);
m_XPDF_get_INT(LinkDest, getPageNum);

int m_LinkDest_getPageRef(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_LinkDest);
    uout = new_Ref_userdata(L);
    uout->d = (Ref *) gmalloc(sizeof(Ref));
    ((Ref *) uout->d)->num = ((LinkDest *) uin->d)->getPageRef().num;
    ((Ref *) uout->d)->gen = ((LinkDest *) uin->d)->getPageRef().gen;
    uout->atype = ALLOC_LEPDF;
    return 1;
}

m_XPDF_get_DOUBLE(LinkDest, getLeft);
m_XPDF_get_DOUBLE(LinkDest, getBottom);
m_XPDF_get_DOUBLE(LinkDest, getRight);
m_XPDF_get_DOUBLE(LinkDest, getTop);
m_XPDF_get_DOUBLE(LinkDest, getZoom);
m_XPDF_get_BOOL(LinkDest, getChangeLeft);
m_XPDF_get_BOOL(LinkDest, getChangeTop);
m_XPDF_get_BOOL(LinkDest, getChangeZoom);

m_XPDF__tostring(LinkDest);

static const struct luaL_Reg LinkDest_m[] = {
    {"isOk", m_LinkDest_isOk},  /* */
    {"getKind", m_LinkDest_getKind},    /* */
    {"getKindName", m_LinkDest_getKindName},    /* not xpdf */
    {"isPageRef", m_LinkDest_isPageRef},        /* */
    {"getPageNum", m_LinkDest_getPageNum},      /* */
    {"getPageRef", m_LinkDest_getPageRef},      /* */
    {"getLeft", m_LinkDest_getLeft},    /* */
    {"getBottom", m_LinkDest_getBottom},        /* */
    {"getRight", m_LinkDest_getRight},  /* */
    {"getTop", m_LinkDest_getTop},      /* */
    {"getZoom", m_LinkDest_getZoom},    /* */
    {"getChangeLeft", m_LinkDest_getChangeLeft},        /* */
    {"getChangeTop", m_LinkDest_getChangeTop},  /* */
    {"getChangeZoom", m_LinkDest_getChangeZoom},        /* */
    {"__tostring", m_LinkDest__tostring},       /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Object

int m_Object_fetch(lua_State * L)
{
    udstruct *uin, *uxref, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    uxref = (udstruct *) luaL_checkudata(L, 2, M_XRef);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    ((Object *) uin->d)->fetch((XRef *) uxref->d, (Object *) uout->d);
    uout->atype = ALLOC_LEPDF;
    return 1;
}

int m_Object_getType(lua_State * L)
{
    ObjType t;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    t = ((Object *) uin->d)->getType();
    lua_pushinteger(L, (int) t);
    return 1;
}

int m_Object_getTypeName(lua_State * L)
{
    char *s;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    s = ((Object *) uin->d)->getTypeName();
    lua_pushstring(L, s);
    return 1;
}

m_XPDF_get_BOOL(Object, isBool);
m_XPDF_get_BOOL(Object, isInt);
m_XPDF_get_BOOL(Object, isReal);
m_XPDF_get_BOOL(Object, isNum);
m_XPDF_get_BOOL(Object, isString);
m_XPDF_get_BOOL(Object, isName);
m_XPDF_get_BOOL(Object, isNull);
m_XPDF_get_BOOL(Object, isArray);
m_XPDF_get_BOOL(Object, isDict);
m_XPDF_get_BOOL(Object, isStream);
m_XPDF_get_BOOL(Object, isRef);
m_XPDF_get_BOOL(Object, isCmd);
m_XPDF_get_BOOL(Object, isError);
m_XPDF_get_BOOL(Object, isEOF);
m_XPDF_get_BOOL(Object, isNone);
// isName
// isDict
// isStream
// isCmd

int m_Object_getBool(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isBool()) {
        if (((Object *) uin->d)->getBool())
            lua_pushboolean(L, 1);
        else
            lua_pushboolean(L, 0);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getInt(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isInt())
        lua_pushnumber(L, ((Object *) uin->d)->getInt());
    else
        lua_pushnil(L);
    return 1;
}

int m_Object_getReal(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isReal())
        lua_pushnumber(L, ((Object *) uin->d)->getReal());
    else
        lua_pushnil(L);
    return 1;
}

int m_Object_getNum(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isNum())
        lua_pushnumber(L, ((Object *) uin->d)->getNum());
    else
        lua_pushnil(L);
    return 1;
}

int m_Object_getString(lua_State * L)
{
    GString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isString()) {
        gs = ((Object *) uin->d)->getString();
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getName(lua_State * L)
{
    char *s;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isName()) {
        s = ((Object *) uin->d)->getName();
        lua_pushstring(L, s);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getArray(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isArray()) {
        uout = new_Array_userdata(L);
        uout->d = ((Object *) uin->d)->getArray();
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getDict(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Dict_userdata(L);
        uout->d = ((Object *) uin->d)->getDict();
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getStream(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        uout = new_Dict_userdata(L);
        uout->d = ((Object *) uin->d)->getStream();
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getRef(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isRef()) {
        uout = new_Ref_userdata(L);
        uout->d = (Ref *) gmalloc(sizeof(Ref));
        ((Ref *) uout->d)->num = ((Object *) uin->d)->getRef().num;
        ((Ref *) uout->d)->gen = ((Object *) uin->d)->getRef().gen;
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getRefNum(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isRef()) {
        i = ((Object *) uin->d)->getRef().num;
        lua_pushinteger(L, i);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getRefGen(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isRef()) {
        i = ((Object *) uin->d)->getRef().gen;
        lua_pushinteger(L, i);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_getCmd(lua_State * L)
{
    char *s;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isCmd()) {
        s = ((Object *) uin->d)->getCmd();
        lua_pushstring(L, s);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_arrayGetLength(lua_State * L)
{
    int len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isArray()) {
        len = ((Object *) uin->d)->arrayGetLength();
        lua_pushnumber(L, len);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_arrayGet(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isArray()) {
        len = ((Object *) uin->d)->arrayGetLength();
        if (i > 0 && i <= len) {
            uout = new_Object_userdata(L);
            uout->d = new Object();
            ((Object *) uin->d)->arrayGet(i - 1, (Object *) uout->d);
            uout->atype = ALLOC_LEPDF;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_arrayGetNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isArray()) {
        len = ((Object *) uin->d)->arrayGetLength();
        if (i > 0 && i <= len) {
            uout = new_Object_userdata(L);
            uout->d = new Object();
            ((Object *) uin->d)->arrayGetNF(i - 1, (Object *) uout->d);
            uout->atype = ALLOC_LEPDF;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictGetLength(lua_State * L)
{
    int len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isDict()) {
        len = ((Object *) uin->d)->dictGetLength();
        lua_pushnumber(L, len);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictLookup(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    s = luaL_checkstring(L, 2);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Object *) uin->d)->dictLookup((char *) s, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictLookupNF(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    s = luaL_checkstring(L, 2);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        ((Object *) uin->d)->dictLookupNF((char *) s, (Object *) uout->d);
        uout->atype = ALLOC_LEPDF;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictGetKey(lua_State * L)
{
    int i, len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isDict()) {
        len = ((Object *) uin->d)->dictGetLength();
        if (i > 0 && i <= len)
            lua_pushstring(L, ((Object *) uin->d)->dictGetKey(i - 1));
        else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictGetVal(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isDict()) {
        len = ((Object *) uin->d)->dictGetLength();
        if (i > 0 && i <= len) {
            uout = new_Object_userdata(L);
            uout->d = new Object();
            ((Object *) uin->d)->dictGetVal(i - 1, (Object *) uout->d);
            uout->atype = ALLOC_LEPDF;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_dictGetValNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isDict()) {
        len = ((Object *) uin->d)->dictGetLength();
        if (i > 0 && i <= len) {
            uout = new_Object_userdata(L);
            uout->d = new Object();
            ((Object *) uin->d)->dictGetValNF(i - 1, (Object *) uout->d);
            uout->atype = ALLOC_LEPDF;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_streamGetChar(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        i = ((Object *) uin->d)->streamGetChar();
        lua_pushinteger(L, i);
    } else
        lua_pushnil(L);
    return 1;
}

int m_Object_streamGetDict(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        uout = new_Dict_userdata(L);
        uout->d = ((Object *) uin->d)->streamGetDict();
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
#  ifdef DEBUG
    printf("\n===== Type_Object GC FREE ===== a=<%p>\n", a);
#  endif
    if (uin->atype == ALLOC_LEPDF)
        delete((Object *) uin->d);
    return 0;
}

m_XPDF__tostring(Object);

static const struct luaL_Reg Object_m[] = {
    {"fetch", m_Object_fetch},  /* */
    {"getType", m_Object_getType},      /* */
    {"getTypeName", m_Object_getTypeName},      /* not XPDF */
    {"isBool", m_Object_isBool},        /* */
    {"isInt", m_Object_isInt},  /* */
    {"isReal", m_Object_isReal},        /* */
    {"isNum", m_Object_isNum},  /* */
    {"isString", m_Object_isString},    /* */
    {"isName", m_Object_isName},        /* */
    {"isNull", m_Object_isNull},        /* */
    {"isArray", m_Object_isArray},      /* */
    {"isDict", m_Object_isDict},        /* */
    {"isStream", m_Object_isStream},    /* */
    {"isRef", m_Object_isRef},  /* */
    {"isCmd", m_Object_isCmd},  /* */
    {"isError", m_Object_isError},      /* */
    {"isEOF", m_Object_isEOF},  /* */
    {"isNone", m_Object_isNone},        /* */
    //
    {"getBool", m_Object_getBool},      /* */
    {"getInt", m_Object_getInt},        /* */
    {"getReal", m_Object_getReal},      /* */
    {"getNum", m_Object_getNum},        /* */
    {"getString", m_Object_getString},  /* */
    {"getName", m_Object_getName},      /* */
    {"getArray", m_Object_getArray},    /* */
    {"getDict", m_Object_getDict},      /* */
    {"getStream", m_Object_getStream},  /* */
    {"getRef", m_Object_getRef},        /* */
    {"getRefNum", m_Object_getRefNum},  /* */
    {"getRefGen", m_Object_getRefGen},  /* */
    {"getCmd", m_Object_getCmd},        /* */
    {"arrayGetLength", m_Object_arrayGetLength},        /* */
    {"arrayGet", m_Object_arrayGet},    /* */
    {"arrayGetNF", m_Object_arrayGetNF},        /* */
    //
    {"dictGetLength", m_Object_dictGetLength},  /* */
    {"dictLookup", m_Object_dictLookup},        /* */
    {"dictLookupNF", m_Object_dictLookupNF},    /* */
    {"dictgetKey", m_Object_dictGetKey},        /* */
    {"dictgetVal", m_Object_dictGetVal},        /* */
    {"dictgetValNF", m_Object_dictGetValNF},    /* */
    //
    {"streamGetChar", m_Object_streamGetChar},  /* */
    //
    {"streamGetDict", m_Object_streamGetDict},  /* */
    //
    {"__gc", m_Object__gc},     /* finalizer */
    {"__tostring", m_Object__tostring}, /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// ObjectStream

m_XPDF__tostring(ObjectStream);

static const struct luaL_Reg ObjectStream_m[] = {
    {"__tostring", m_ObjectStream__tostring},   /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Page

m_XPDF_get_BOOL(Page, isOk);
m_XPDF_get_INT(Page, getNum);
m_XPDF_get_XPDF(Page, PDFRectangle, getMediaBox);
m_XPDF_get_XPDF(Page, PDFRectangle, getCropBox);
m_XPDF_get_BOOL(Page, isCropped);
m_XPDF_get_DOUBLE(Page, getMediaWidth);
m_XPDF_get_DOUBLE(Page, getMediaHeight);
m_XPDF_get_DOUBLE(Page, getCropWidth);
m_XPDF_get_DOUBLE(Page, getCropHeight);
m_XPDF_get_XPDF(Page, PDFRectangle, getBleedBox);
m_XPDF_get_XPDF(Page, PDFRectangle, getTrimBox);
m_XPDF_get_XPDF(Page, PDFRectangle, getArtBox);
m_XPDF_get_INT(Page, getRotate);

int m_Page_getLastModified(lua_State * L)
{
    GString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    gs = ((Page *) uin->d)->getLastModified();
    if (gs != NULL)
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    else
        lua_pushnil(L);
    return 1;
}

int m_Page_getBoxColorInfo(lua_State * L)
{
    Dict *dict;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    dict = ((Page *) uin->d)->getBoxColorInfo();
    if (dict != NULL) {
        uout = new_Dict_userdata(L);
        uout->d = dict;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Page_getGroup(lua_State * L)
{
    Dict *dict;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    dict = ((Page *) uin->d)->getGroup();
    if (dict != NULL) {
        uout = new_Dict_userdata(L);
        uout->d = dict;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Page_getMetadata(lua_State * L)
{
    Stream *stream;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    stream = ((Page *) uin->d)->getMetadata();
    if (stream != NULL) {
        uout = new_Stream_userdata(L);
        uout->d = stream;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Page_getPieceInfo(lua_State * L)
{
    Dict *dict;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    dict = ((Page *) uin->d)->getPieceInfo();
    if (dict != NULL) {
        uout = new_Dict_userdata(L);
        uout->d = dict;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Page_getSeparationInfo(lua_State * L)
{
    Dict *dict;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    dict = ((Page *) uin->d)->getSeparationInfo();
    if (dict != NULL) {
        uout = new_Dict_userdata(L);
        uout->d = dict;
    } else
        lua_pushnil(L);
    return 1;
}

int m_Page_getResourceDict(lua_State * L)
{
    Dict *dict;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Page);
    dict = ((Page *) uin->d)->getResourceDict();
    if (dict != NULL) {
        uout = new_Dict_userdata(L);
        uout->d = dict;
    } else
        lua_pushnil(L);
    return 1;
}

m_XPDF_get_OBJECT(Page, getAnnots);
// getLinks
m_XPDF_get_OBJECT(Page, getContents);

m_XPDF__tostring(Page);

static const struct luaL_Reg Page_m[] = {
    {"isOk", m_Page_isOk},      /* */
    {"getNum", m_Page_getNum},  /* */
    {"getMediaBox", m_Page_getMediaBox},        /* */
    {"getCropBox", m_Page_getCropBox},  /* */
    {"isCropped", m_Page_isCropped},    /* */
    {"getMediaWidth", m_Page_getMediaWidth},    /* */
    {"getMediaHeight", m_Page_getMediaHeight},  /* */
    {"getCropWidth", m_Page_getCropWidth},      /* */
    {"getCropHeight", m_Page_getCropHeight},    /* */
    {"getBleedBox", m_Page_getBleedBox},        /* */
    {"getTrimBox", m_Page_getTrimBox},  /* */
    {"getArtBox", m_Page_getArtBox},    /* */
    {"getRotate", m_Page_getRotate},    /* */
    {"getLastModified", m_Page_getLastModified},        /* */
    {"getBoxColorInfo", m_Page_getBoxColorInfo},        /* */
    {"getGroup", m_Page_getGroup},      /* */
    {"getMetadata", m_Page_getMetadata},        /* */
    {"getPieceInfo", m_Page_getPieceInfo},      /* */
    {"getSeparationInfo", m_Page_getSeparationInfo},    /* */
    {"getResourceDict", m_Page_getResourceDict},        /* */
    {"getAnnots", m_Page_getAnnots},    /* */
    //
    {"getContents", m_Page_getContents},        /* */
    {"__tostring", m_Page__tostring},   /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// PDFDoc

int m_PDFDoc_isOk(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    if (((PdfDocument *) uin->d)->doc->isOk())
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);
    return 1;
}

int m_PDFDoc_getErrorCode(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    i = ((PdfDocument *) uin->d)->doc->getErrorCode();
    lua_pushinteger(L, i);
    return 1;
}

int m_PDFDoc_getErrorCodeName(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    i = ((PdfDocument *) uin->d)->doc->getErrorCode();
    lua_pushstring(L, ErrorCodeNames[i]);
    return 1;
}

int m_PDFDoc_getCatalog(lua_State * L)
{
    Catalog *cat;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    cat = ((PdfDocument *) uin->d)->doc->getCatalog();
    if (cat->isOk()) {
        uout = new_Catalog_userdata(L);
        uout->d = cat;
    } else
        lua_pushnil(L);
    return 1;
}

int m_PDFDoc_getXRef(lua_State * L)
{
    XRef *xref;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    xref = ((PdfDocument *) uin->d)->doc->getXRef();
    if (xref->isOk()) {
        uout = new_XRef_userdata(L);
        uout->d = xref;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_PDFDoc__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
#  ifdef DEBUG
    printf("\n===== Type_PDFDoc GC FREE ===== a=<%s>\n", a->file_path);
#  endif
    assert(uin->atype == ALLOC_LEPDF);
    unrefPdfDocument(((PdfDocument *) uin->d)->file_path);
    return 0;
}

static const struct luaL_Reg PDFDoc_m[] = {
    {"isOk", m_PDFDoc_isOk},    /* */
    {"getErrorCode", m_PDFDoc_getErrorCode},    /* */
    {"getErrorCodeName", m_PDFDoc_getErrorCodeName},    /* not xpdf */
    {"getCatalog", m_PDFDoc_getCatalog},        /* */
    {"getXRef", m_PDFDoc_getXRef},      /* */
    {"__gc", m_PDFDoc__gc},     /* finalizer */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// PDFRectangle

m_XPDF__tostring(PDFRectangle);

static const struct luaL_Reg PDFRectangle_m[] = {
    {"__tostring", m_PDFRectangle__tostring},   /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Ref

int m_Ref__index(lua_State * L)
{
    const char *s;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Ref);
    s = luaL_checkstring(L, 2);
    if (strcmp(s, "num") == 0)
        lua_pushinteger(L, ((Ref *) uin->d)->num);
    else if (strcmp(s, "gen") == 0)
        lua_pushinteger(L, ((Ref *) uin->d)->gen);
    else
        lua_pushnil(L);
    return 1;
}

m_XPDF__tostring(Ref);

static int m_Ref__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Ref);
#  ifdef DEBUG
    printf("\n===== Type_Ref GC FREE ===== a=<%p>\n", a);
#  endif
    if (uin->atype == ALLOC_LEPDF && ((Ref *) uin->d) != NULL)
        gfree(((Ref *) uin->d));
    return 0;
}

static const struct luaL_Reg Ref_m[] = {
    {"__index", m_Ref__index},  /* not XPDF */
    {"__tostring", m_Ref__tostring},    /* */
    {"__gc", m_Ref__gc},        /* finalizer */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// Stream

static const char *StreamKindNames[] =
    { "File", "ASCIIHex", "ASCII85", "LZW", "RunLength", "CCITTFax", "DCT",
    "Flate", "JBIG2", "JPX", "Weird", NULL
};

#  if 0
static const char *StreamColorSpaceModeNames[] =
    { "CSNone", "CSDeviceGray", "CSDeviceRGB", "CSDeviceCMYK", NULL };
#  endif

m_XPDF_get_INT(Stream, getKind);

int m_Stream_getKindName(lua_State * L)
{
    StreamKind t;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Stream);
    t = ((Stream *) uin->d)->getKind();
    lua_pushstring(L, StreamKindNames[t]);
    return 1;
}

int m_Stream_reset(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Stream);
    ((Stream *) uin->d)->reset();
    return 0;
}

int m_Stream_getChar(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Stream);
    i = ((Stream *) uin->d)->getChar();
    lua_pushinteger(L, i);
    return 1;
}

int m_Stream_lookChar(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Stream);
    i = ((Stream *) uin->d)->lookChar();
    lua_pushinteger(L, i);
    return 1;
}

m_XPDF_get_XPDF(Stream, Stream, getUndecodedStream);
m_XPDF_get_BOOL(Stream, isBinary);
m_XPDF_get_XPDF(Stream, Dict, getDict);

m_XPDF__tostring(Stream);

static const struct luaL_Reg Stream_m[] = {
    {"getKind", m_Stream_getKind},      /* */
    {"getKindName", m_Stream_getKindName},      /* not xpdf */
    {"reset", m_Stream_reset},  /* */
    {"getUndecodedStream", m_Stream_getUndecodedStream},        /* */
    {"getChar", m_Stream_getChar},      /* */
    {"lookChar", m_Stream_lookChar},    /* */
    {"isBinary", m_Stream_isBinary},    /* */
    {"getDict", m_Stream_getDict},      /* */
    {"__tostring", m_Stream__tostring}, /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// XRef

m_XPDF_get_BOOL(XRef, isOk);
m_XPDF_get_INT(XRef, getErrorCode);
m_XPDF_get_BOOL(XRef, isEncrypted);
m_XPDF_get_BOOL(XRef, okToPrint);
m_XPDF_get_BOOL(XRef, okToChange);
m_XPDF_get_BOOL(XRef, okToCopy);
m_XPDF_get_BOOL(XRef, okToAddNotes);
m_XPDF_get_OBJECT(XRef, getCatalog);

int m_XRef_fetch(lua_State * L)
{
    int num, gen;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_XRef);
    num = luaL_checkint(L, 2);
    gen = luaL_checkint(L, 3);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    ((XRef *) uin->d)->fetch(num, gen, (Object *) uout->d);
    uout->atype = ALLOC_LEPDF;
    return 1;
}

m_XPDF_get_OBJECT(XRef, getDocInfo);
m_XPDF_get_OBJECT(XRef, getDocInfoNF);
m_XPDF_get_INT(XRef, getNumObjects);
// getLastXRefPos
m_XPDF_get_INT(XRef, getRootNum);
m_XPDF_get_INT(XRef, getRootGen);
// getStreamEnd
m_XPDF_get_INT(XRef, getSize);
// getEntry
m_XPDF_get_XPDF(XRef, Object, getTrailerDict);
m_XPDF_get_XPDF(XRef, ObjectStream, getObjStr);

m_XPDF__tostring(XRef);

static const struct luaL_Reg XRef_m[] = {
    {"isOk", m_XRef_isOk},      /* */
    {"getErrorCode", m_XRef_getErrorCode},      /* */
    {"isEncrypted", m_XRef_isEncrypted},        /* */
    {"okToPrint", m_XRef_okToPrint},    /* */
    {"okToChange", m_XRef_okToChange},  /* */
    {"okToCopy", m_XRef_okToCopy},      /* */
    {"okToAddNotes", m_XRef_okToAddNotes},      /* */
    {"getCatalog", m_XRef_getCatalog},  /* */
    {"fetch", m_XRef_fetch},    /* */
    {"getDocInfo", m_XRef_getDocInfo},  /* */
    {"getDocInfoNF", m_XRef_getDocInfoNF},      /* */
    {"getNumObjects", m_XRef_getNumObjects},    /* */
    //
    {"getRootNum", m_XRef_getRootNum},  /* */
    {"getRootGen", m_XRef_getRootGen},  /* */
    //
    {"getSize", m_XRef_getSize},        /* */
    {"getTrailerDict", m_XRef_getTrailerDict},  /* */
    {"getObjStr", m_XRef_getObjStr},    /* */
    {"__tostring", m_XRef__tostring},   /* */
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************
// XRefEntry

static const struct luaL_Reg XRefEntry_m[] = {
    {NULL, NULL}                /* sentinel */
};

//**********************************************************************

#  define register_meta(type)               \
    luaL_newmetatable(L, M_##type);         \
    lua_pushvalue(L, -1);                   \
    lua_setfield(L, -2, "__index");         \
    lua_pushstring(L, "no user access"); \
    lua_setfield(L, -2, "__metatable");     \
    luaL_register(L, NULL, type##_m)

int luaopen_epdf(lua_State * L)
{
    register_meta(Array);
    register_meta(Catalog);
    register_meta(Dict);
    register_meta(GString);
    register_meta(LinkDest);
    register_meta(Object);
    register_meta(ObjectStream);
    register_meta(Page);
    register_meta(PDFDoc);
    register_meta(PDFRectangle);
    register_meta(Ref);
    register_meta(Stream);
    register_meta(XRef);
    register_meta(XRefEntry);

    luaL_register(L, "epdf", epdflib);
    return 1;
}

//**********************************************************************
#else                           // ALPHA_TEST

static const struct luaL_Reg epdflib[] = {
    {NULL, NULL}                /* sentinel */
};

int luaopen_epdf(lua_State * L)
{
    luaL_register(L, "epdf", epdflib);
    return 1;
}

#endif                          // ALPHA_TEST
