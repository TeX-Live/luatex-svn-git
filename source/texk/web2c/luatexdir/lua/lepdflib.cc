/* lepdflib.cc

   Copyright 2009-2015 Taco Hoekwater <taco@luatex.org>
   Copyright 2009-2013 Hartmut Henkel <hartmut@luatex.org>

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


#include "image/epdf.h"

/*
    Patches for the new poppler 0.59 from:

    https://www.mail-archive.com/arch-commits@archlinux.org/msg357548.html

    with some modifications to comply the poppler API.
*/

/* define DEBUG */

/* objects allocated by poppler may not be deleted in the lepdflib */

typedef enum { ALLOC_POPPLER, ALLOC_LEPDF } alloctype;

typedef struct {
    void *d;
    alloctype atype;   // was it allocated by poppler or the lepdflib.cc?
    PdfDocument *pd;   // reference to PdfDocument, or NULL
    unsigned long pc;  // counter to detect PDFDoc change
} udstruct;

#define M_Array            "epdf.Array"
#define M_Catalog          "epdf.Catalog"
#define M_Dict             "epdf.Dict"
#define M_Object           "epdf.Object"
#define M_Page             "epdf.Page"
#define M_PDFDoc           "epdf.PDFDoc"
#define M_Ref              "epdf.Ref"
#define M_Stream           "epdf.Stream"
#define M_XRefEntry        "epdf.XRefEntry"
#define M_XRef             "epdf.XRef"

#define new_poppler_userdata(type)                                              \
static udstruct *new_##type##_userdata(lua_State * L)                           \
{                                                                               \
    udstruct *a;                                                                \
    a = (udstruct *) lua_newuserdata(L, sizeof(udstruct));  /* udstruct ... */  \
    a->atype = ALLOC_POPPLER;                                                   \
    luaL_getmetatable(L, M_##type);     /* m udstruct ... */                    \
    lua_setmetatable(L, -2);    /* udstruct ... */                              \
    return a;                                                                   \
}

new_poppler_userdata(PDFDoc);
new_poppler_userdata(Array);
new_poppler_userdata(Catalog);
new_poppler_userdata(Dict);
new_poppler_userdata(Object);
new_poppler_userdata(Page);
new_poppler_userdata(Ref);
new_poppler_userdata(Stream);
new_poppler_userdata(XRef);

/* These two can go away as we don't change the document. */

static int l_open_PDFDoc(lua_State * L)
{
    const char *file_path;
    udstruct *uout;
    PdfDocument *d;
    file_path = luaL_checkstring(L, 1); // path
    d = refPdfDocument(file_path, FE_RETURN_NULL);
    if (d == NULL)
        lua_pushnil(L);
    else {
        if (!(globalParams)) // globalParams could be already created
            globalParams = new GlobalParams();
        uout = new_PDFDoc_userdata(L);
        uout->d = d;
        uout->atype = ALLOC_LEPDF;
        uout->pc = d->pc;
        uout->pd = d;
    }
    return 1;
}

static int l_open_MemStreamPDFDoc(lua_State * L)
{
    const char *docstream = NULL;
    char *docstream_usr = NULL ;
    const char *file_id;
    unsigned long long stream_size;
    udstruct *uout;
    PdfDocument *d;
    switch (lua_type(L, 1)) {
        case LUA_TSTRING:
            docstream = luaL_checkstring(L, 1); // stream as Lua string
            break;
        case LUA_TLIGHTUSERDATA:
            docstream = (const char *) lua_touserdata(L, 1); // stream as sequence of bytes
            break;
        default:
            luaL_error(L, "bad argument: string or lightuserdata expected");
    }
    if (docstream==NULL)
        luaL_error(L, "bad document");
    stream_size = (unsigned long long) luaL_checkint(L, 2);// size of the stream
    file_id  =  luaL_checkstring(L, 3); // a symbolic name for this stream, mandatory
    if (file_id == NULL)
        luaL_error(L, "PDFDoc has an invalid id");
    if (strlen(file_id) >STREAM_FILE_ID_LEN )  // a limit to the length of the string
        luaL_error(L, "PDFDoc has a too long id");
    docstream_usr = (char *)gmalloc((unsigned) (stream_size + 1));
    if (!docstream_usr)
        luaL_error(L, "no room for PDFDoc");
    memcpy(docstream_usr, docstream, (stream_size + 1));
    docstream_usr[stream_size]='\0';
    d = refMemStreamPdfDocument(docstream_usr, stream_size, file_id);
    if (d == NULL) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    } else if (d->file_path == NULL ) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    else {
        if (!(globalParams)) // globalParams could be already created
            globalParams = new GlobalParams();
        uout = new_PDFDoc_userdata(L);
        uout->d = d;
        uout->atype = ALLOC_LEPDF;
        uout->pc = d->pc;
        uout->pd = d;
        lua_pushstring(L,d->file_path);
        lua_pushstring(L,STREAM_URI);
    }
    return 3;                   // stream, stream_id, stream_uri
}

#define OBJECT_TYPE(name)                   \
    lua_pushstring(L, #name);                \
    lua_pushinteger(L, (int)name);           \
    lua_settable(L,-3)

static int l_Object_Type(lua_State * L) {
    lua_createtable(L,0,16);/*nr of ObjType values*/ ;
    OBJECT_TYPE(objBool);
    OBJECT_TYPE(objInt);
    OBJECT_TYPE(objReal);
    OBJECT_TYPE(objString);
    OBJECT_TYPE(objName);
    OBJECT_TYPE(objNull);
    OBJECT_TYPE(objArray);
    OBJECT_TYPE(objDict);
    OBJECT_TYPE(objStream);
    OBJECT_TYPE(objRef);
    OBJECT_TYPE(objCmd);
    OBJECT_TYPE(objError);
    OBJECT_TYPE(objEOF);
    OBJECT_TYPE(objNone);
    OBJECT_TYPE(objInt64);
    OBJECT_TYPE(objDead);
    return 1;
}

static const struct luaL_Reg epdflib_f[] = {
    {"open", l_open_PDFDoc},
    {"openMemStream", l_open_MemStreamPDFDoc},
    {"Object_Type", l_Object_Type},
    {NULL, NULL}
};

#define m_poppler_get_poppler(in, out, function)               \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    out *o;                                                    \
    udstruct *uin, *uout;                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    o = ((in *) uin->d)->function();                           \
    if (o != NULL) {                                           \
        uout = new_##out##_userdata(L);                        \
        uout->d = o;                                           \
        uout->pc = uin->pc;                                    \
        uout->pd = uin->pd;                                    \
    } else                                                     \
        lua_pushnil(L);                                        \
    return 1;                                                  \
}

#define m_poppler_get_BOOL(in, function)                       \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    if (((in *) uin->d)->function())                           \
        lua_pushboolean(L, 1);                                 \
    else                                                       \
        lua_pushboolean(L, 0);                                 \
    return 1;                                                  \
}

#define m_poppler_get_INT(in, function)                        \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    int i;                                                     \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    i = (int) ((in *) uin->d)->function();                     \
    lua_pushinteger(L, i);                                     \
    return 1;                                                  \
}


#define m_poppler_get_GUINT(in, function)                      \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    unsigned int i;                                            \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    i = (unsigned int) ((in *) uin->d)->function();            \
    lua_pushinteger(L, i);                                     \
    return 1;                                                  \
}

#define m_poppler_get_UINT(in, function)                       \
m_poppler_get_GUINT(in, function)



#define m_poppler_get_DOUBLE(in, function)                     \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    double d;                                                  \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    d = (double) ((in *) uin->d)->function();                  \
    lua_pushnumber(L, d); /* float */                          \
    return 1;                                                  \
}

#define m_poppler_get_GOOSTRING(in, function)                  \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    GooString *gs;                                             \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    gs = ((in *) uin->d)->function();                          \
    if (gs != NULL)                                            \
        lua_pushlstring(L, gs->getCString(), gs->getLength()); \
    else                                                       \
        lua_pushnil(L);                                        \
    return 1;                                                  \
}

#define m_poppler_get_OBJECT(in, function)                     \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    udstruct *uin, *uout;                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    uout = new_Object_userdata(L);                             \
    uout->d = new Object();                                    \
    *((Object *)uout->d) = ((in *) uin->d)->function();                  \
    uout->atype = ALLOC_LEPDF;                                 \
    uout->pc = uin->pc;                                        \
    uout->pd = uin->pd;                                        \
    return 1;                                                  \
}

#define m_poppler_do(in, function)                             \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    ((in *) uin->d)->function();                               \
    return 0;                                                  \
}

#define m_poppler__tostring(type)                              \
static int m_##type##__tostring(lua_State * L)                 \
{                                                              \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##type);        \
    lua_pushfstring(L, "%s: %p", #type, (type *) uin->d);      \
    return 1;                                                  \
}

#define m_poppler_check_string(in, function)                   \
static int m_##in##_##function(lua_State * L)                  \
{                                                              \
    const char *s;                                             \
    udstruct *uin;                                             \
    uin = (udstruct *) luaL_checkudata(L, 1, M_##in);          \
    s = luaL_checkstring(L, 2);                                \
    if (((in *) uin->d)->function(s))                          \
        lua_pushboolean(L, 1);                                 \
    else                                                       \
        lua_pushboolean(L, 0);                                 \
    return 1;                                                  \
}

/* Array */

m_poppler_get_INT(Array, getLength);

static int m_Array_get(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Array);
    i = luaL_checkint(L, 2);
    len = ((Array *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Array *) uin->d)->get(i - 1);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Array_getNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Array);
    i = luaL_checkint(L, 2);
    len = ((Array *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Array *) uin->d)->getNF(i - 1);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

m_poppler__tostring(Array);

static const struct luaL_Reg Array_m[] = {
    {"getLength", m_Array_getLength},
    {"get", m_Array_get},
    {"getNF", m_Array_getNF},
    {"__tostring", m_Array__tostring},
    {NULL, NULL}
};

/* Catalog */

m_poppler_get_INT(Catalog, getNumPages);

static int m_Catalog_getPage(lua_State * L)
{
    int i, pages;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Catalog);
    i = luaL_checkint(L, 2);
    pages = ((Catalog *) uin->d)->getNumPages();
    if (i > 0 && i <= pages) {
        uout = new_Page_userdata(L);
        uout->d = ((Catalog *) uin->d)->getPage(i);
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Catalog_getPageRef(lua_State * L)
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
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Catalog_findPage(lua_State * L)
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

m_poppler__tostring(Catalog);

static const struct luaL_Reg Catalog_m[] = {
    {"getNumPages", m_Catalog_getNumPages},
    {"getPage", m_Catalog_getPage},
    {"getPageRef", m_Catalog_getPageRef},
    {"findPage", m_Catalog_findPage},
    {"__tostring", m_Catalog__tostring},
    {NULL, NULL}
};

/* Dict */

m_poppler_get_INT(Dict, getLength);

static int m_Dict_lookup(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    s = luaL_checkstring(L, 2);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    *((Object *) uout->d) = ((Dict *) uin->d)->lookup(s);
    uout->atype = ALLOC_LEPDF;
    uout->pc = uin->pc;
    uout->pd = uin->pd;
    return 1;
}

static int m_Dict_lookupNF(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    s = luaL_checkstring(L, 2);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    *((Object *) uout->d) = ((Dict *) uin->d)->lookupNF(s);
    uout->atype = ALLOC_LEPDF;
    uout->pc = uin->pc;
    uout->pd = uin->pd;
    return 1;
}

static int m_Dict_getKey(lua_State * L)
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

static int m_Dict_getVal(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    i = luaL_checkint(L, 2);
    len = ((Dict *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Dict *) uin->d)->getVal(i - 1);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Dict_getValNF(lua_State * L)
{
    int i, len;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Dict);
    i = luaL_checkint(L, 2);
    len = ((Dict *) uin->d)->getLength();
    if (i > 0 && i <= len) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Dict *) uin->d)->getValNF(i - 1);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

m_poppler_check_string(Dict, hasKey);

m_poppler__tostring(Dict);

static const struct luaL_Reg Dict_m[] = {
    {"getLength", m_Dict_getLength},
    {"lookup", m_Dict_lookup},
    {"lookupNF", m_Dict_lookupNF},
    {"getKey", m_Dict_getKey},
    {"getVal", m_Dict_getVal},
    {"getValNF", m_Dict_getValNF},
    {"hasKey", m_Dict_hasKey},
    {"__tostring", m_Dict__tostring},
    {NULL, NULL}
};

/* Object */

#ifdef HAVE_OBJECT_INITCMD_CONST_CHARP
#define CHARP_CAST
#else
/*
    Must cast arg of Object::initCmd, Object::isStream, and Object::streamIs
    from 'const char *' to 'char *', although they are not modified.
*/
#define CHARP_CAST (char *)
#endif

/* Special type checking. */

#define m_Object_isType_(function, cast)                                   \
static int m_Object_##function(lua_State * L)                              \
{                                                                          \
    udstruct *uin;                                                         \
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);                    \
    if (lua_gettop(L) >= 2) {                                              \
        if (lua_isstring(L, 2)                                             \
            && ((Object *) uin->d)->function(cast lua_tostring(L, 2)))     \
            lua_pushboolean(L, 1);                                         \
        else                                                               \
            lua_pushboolean(L, 0);                                         \
    } else {                                                               \
        if (((Object *) uin->d)->function())                               \
            lua_pushboolean(L, 1);                                         \
        else                                                               \
            lua_pushboolean(L, 0);                                         \
    }                                                                      \
    return 1;                                                              \
}

#define m_Object_isType(function) m_Object_isType_(function, )
#define m_Object_isType_nonconst(function) m_Object_isType_(function, CHARP_CAST)

static int m_Object_fetch(lua_State * L)
{
    udstruct *uin, *uxref, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    uxref = (udstruct *) luaL_checkudata(L, 2, M_XRef);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    *((Object *) uout->d) = ((Object *) uin->d)->fetch((XRef *) uxref->d);
    uout->atype = ALLOC_LEPDF;
    uout->pc = uin->pc;
    uout->pd = uin->pd;
    return 1;
}

static int m_Object_getType(lua_State * L)
{
    ObjType t;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    t = ((Object *) uin->d)->getType();
    lua_pushinteger(L, (int) t);
    return 1;
}

static int m_Object_getBool(lua_State * L)
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

static int m_Object_getInt(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isInt())
        lua_pushinteger(L, ((Object *) uin->d)->getInt());
    else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getReal(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isReal())
        lua_pushnumber(L, ((Object *) uin->d)->getReal()); /* float */
    else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getString(lua_State * L)
{
    GooString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isString()) {
        gs = ((Object *) uin->d)->getString();
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getName(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isName())
        lua_pushstring(L, ((Object *) uin->d)->getName());
    else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getArray(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isArray()) {
        uout = new_Array_userdata(L);
        uout->d = ((Object *) uin->d)->getArray();
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getDict(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Dict_userdata(L);
        uout->d = ((Object *) uin->d)->getDict();
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getStream(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        uout = new_Stream_userdata(L);
        uout->d = ((Object *) uin->d)->getStream();
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getRef(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isRef()) {
        uout = new_Ref_userdata(L);
        uout->d = (Ref *) gmalloc(sizeof(Ref));
        ((Ref *) uout->d)->num = ((Object *) uin->d)->getRef().num;
        ((Ref *) uout->d)->gen = ((Object *) uin->d)->getRef().gen;
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_getRefNum(lua_State * L)
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

static int m_Object_getRefGen(lua_State * L)
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

static int m_Object_getCmd(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isCmd())
        lua_pushstring(L, ((Object *) uin->d)->getCmd());
    else
        lua_pushnil(L);
    return 1;
}

static int m_Object_arrayGetLength(lua_State * L)
{
    int len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isArray()) {
        len = ((Object *) uin->d)->arrayGetLength();
        lua_pushinteger(L, len);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_arrayGet(lua_State * L)
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
            *((Object *) uout->d) = ((Object *) uin->d)->arrayGet(i - 1);
            uout->atype = ALLOC_LEPDF;
            uout->pc = uin->pc;
            uout->pd = uin->pd;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_arrayGetNF(lua_State * L)
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
            *((Object *) uout->d) = ((Object *) uin->d)->arrayGetNF(i - 1);
            uout->atype = ALLOC_LEPDF;
            uout->pc = uin->pc;
            uout->pd = uin->pd;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_dictGetLength(lua_State * L)
{
    int len;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isDict()) {
        len = ((Object *) uin->d)->dictGetLength();
        lua_pushinteger(L, len);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_dictLookup(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    s = luaL_checkstring(L, 2);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Object *) uin->d)->dictLookup(s);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_dictLookupNF(lua_State * L)
{
    const char *s;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    s = luaL_checkstring(L, 2);
    if (((Object *) uin->d)->isDict()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((Object *) uin->d)->dictLookupNF(s);
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_dictGetKey(lua_State * L)
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

static int m_Object_dictGetVal(lua_State * L)
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
	    *((Object *) uout->d) = ((Object *) uin->d)->dictGetVal(i - 1);
            uout->atype = ALLOC_LEPDF;
            uout->pc = uin->pc;
            uout->pd = uin->pd;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_dictGetValNF(lua_State * L)
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
            *((Object *) uout->d) = ((Object *) uin->d)->dictGetValNF(i - 1);
            uout->atype = ALLOC_LEPDF;
            uout->pc = uin->pc;
            uout->pd = uin->pd;
        } else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_streamReset(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream())
        ((Object *) uin->d)->streamReset();
    return 0;
}

static int m_Object_streamGetChar(lua_State * L)
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

static int m_Object_streamGetAll(lua_State * L)
{
    char c;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    Object * o = (Object *) uin->d ;
    luaL_Buffer buf;
    luaL_buffinit(L, &buf);
 /* if (((Object *) uin->d)->isStream()) { */
    if (o->isStream()) {
        while(1) {
         /* c = ((Stream *) uin->d)->getChar() ; */
         /* c = ((Object *) uin->d)->streamGetChar(); */
            c = o->streamGetChar();
            if (c == EOF) {
                break;
            }
            luaL_addchar(&buf, c);
        }
        luaL_pushresult(&buf);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int m_Object_streamLookChar(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        i = ((Object *) uin->d)->streamLookChar();
        lua_pushinteger(L, i);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_streamGetPos(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        i = (int) ((Object *) uin->d)->streamGetPos();
        lua_pushinteger(L, i);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object_streamSetPos(lua_State * L)
{
    int i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    i = luaL_checkint(L, 2);
    if (((Object *) uin->d)->isStream())
        ((Object *) uin->d)->streamSetPos(i);
    return 0;
}

static int m_Object_streamGetDict(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (((Object *) uin->d)->isStream()) {
        uout = new_Dict_userdata(L);
        uout->d = ((Object *) uin->d)->streamGetDict();
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_Object__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Object);
    if (uin->atype == ALLOC_LEPDF) {
        /* free() seems to collide with the lua gc */
        /* */
        /* ((Object *) uin->d)->free(); */
        /* */
        delete(Object *) uin->d;
    }
    return 0;
}

m_poppler__tostring(Object);

static const struct luaL_Reg Object_m[] = {
    {"fetch", m_Object_fetch},
    {"getType", m_Object_getType},
    {"getBool", m_Object_getBool},
    {"getInt", m_Object_getInt},
    {"getReal", m_Object_getReal},
    {"getString", m_Object_getString},
    {"getName", m_Object_getName},
    {"getArray", m_Object_getArray},
    {"getDict", m_Object_getDict},
    {"getStream", m_Object_getStream},
    {"getRef", m_Object_getRef},
    {"getRefNum", m_Object_getRefNum},
    {"getRefGen", m_Object_getRefGen},
    {"getCmd", m_Object_getCmd},
    {"arrayGetLength", m_Object_arrayGetLength},
    {"arrayGet", m_Object_arrayGet},
    {"arrayGetNF", m_Object_arrayGetNF},
    {"dictGetLength", m_Object_dictGetLength},
    {"dictLookup", m_Object_dictLookup},
    {"dictLookupNF", m_Object_dictLookupNF},
    {"dictGetKey", m_Object_dictGetKey},
    {"dictGetVal", m_Object_dictGetVal},
    {"dictGetValNF", m_Object_dictGetValNF},
    {"streamReset", m_Object_streamReset},
    {"streamGetChar", m_Object_streamGetChar},
    {"streamGetAll", m_Object_streamGetAll},
    {"streamLookChar", m_Object_streamLookChar},
    {"streamGetPos", m_Object_streamGetPos},
    {"streamSetPos", m_Object_streamSetPos},
    {"streamGetDict", m_Object_streamGetDict},
    {"__tostring", m_Object__tostring},
    {"__gc", m_Object__gc},
    {NULL, NULL}
};

/* PDFDoc */

#define m_PDFDoc_BOOL(function)                         \
static int m_PDFDoc_##function(lua_State * L)           \
{                                                       \
    udstruct *uin;                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc); \
    if (((PdfDocument *) uin->d)->doc->function())      \
        lua_pushboolean(L, 1);                          \
    else                                                \
        lua_pushboolean(L, 0);                          \
    return 1;                                           \
}

#define m_PDFDoc_INT(function)                          \
static int m_PDFDoc_##function(lua_State * L)           \
{                                                       \
    int i;                                              \
    udstruct *uin;                                      \
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc); \
    i = ((PdfDocument *) uin->d)->doc->function();      \
    lua_pushinteger(L, i);                              \
    return 1;                                           \
}

m_PDFDoc_BOOL(isOk);

static int m_PDFDoc_getFileName(lua_State * L)
{
    GooString *gs;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    gs = ((PdfDocument *) uin->d)->doc->getFileName();
    if (gs != NULL)
        lua_pushlstring(L, gs->getCString(), gs->getLength());
    else
        lua_pushnil(L);
    return 1;
}

static int m_PDFDoc_getXRef(lua_State * L)
{
    XRef *xref;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    xref = ((PdfDocument *) uin->d)->doc->getXRef();
    if (xref->isOk()) {
        uout = new_XRef_userdata(L);
        uout->d = xref;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_PDFDoc_getCatalog(lua_State * L)
{
    Catalog *cat;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    cat = ((PdfDocument *) uin->d)->doc->getCatalog();
    if (cat->isOk()) {
        uout = new_Catalog_userdata(L);
        uout->d = cat;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

m_PDFDoc_INT(getNumPages);

static int m_PDFDoc_findPage(lua_State * L)
{
    int num, gen, i;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    num = luaL_checkint(L, 2);
    gen = luaL_checkint(L, 3);
    if (((PdfDocument *) uin->d)->doc->getCatalog()->isOk()) {
        i = ((PdfDocument *) uin->d)->doc->findPage(num, gen);
        if (i > 0)
            lua_pushinteger(L, i);
        else
            lua_pushnil(L);
    } else
        lua_pushnil(L);
    return 1;
}

static int m_PDFDoc_getDocInfo(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    if (((PdfDocument *) uin->d)->doc->getXRef()->isOk()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((PdfDocument *) uin->d)->doc->getDocInfo();
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

static int m_PDFDoc_getDocInfoNF(lua_State * L)
{
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    if (((PdfDocument *) uin->d)->doc->getXRef()->isOk()) {
        uout = new_Object_userdata(L);
        uout->d = new Object();
        *((Object *) uout->d) = ((PdfDocument *) uin->d)->doc->getDocInfoNF();
        uout->atype = ALLOC_LEPDF;
        uout->pc = uin->pc;
        uout->pd = uin->pd;
    } else
        lua_pushnil(L);
    return 1;
}

m_PDFDoc_INT(getPDFMajorVersion);
m_PDFDoc_INT(getPDFMinorVersion);

m_poppler__tostring(PDFDoc);

static int m_PDFDoc__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_PDFDoc);
    assert(uin->atype == ALLOC_LEPDF);
    unrefPdfDocument(((PdfDocument *) uin->d)->file_path);
    return 0;
}

static const struct luaL_Reg PDFDoc_m[] = {
    {"isOk", m_PDFDoc_isOk},
    {"getFileName", m_PDFDoc_getFileName},
    {"getXRef", m_PDFDoc_getXRef},
    {"getCatalog", m_PDFDoc_getCatalog},
    {"getNumPages", m_PDFDoc_getNumPages},
    {"findPage", m_PDFDoc_findPage},
    {"getDocInfo", m_PDFDoc_getDocInfo},
    {"getDocInfoNF", m_PDFDoc_getDocInfoNF},
    {"getPDFMajorVersion", m_PDFDoc_getPDFMajorVersion},
    {"getPDFMinorVersion", m_PDFDoc_getPDFMinorVersion},
    {"__tostring", m_PDFDoc__tostring},
    {"__gc", m_PDFDoc__gc},
    {NULL, NULL}
};

/* Ref */

static int m_Ref__index(lua_State * L)
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

m_poppler__tostring(Ref);

static int m_Ref__gc(lua_State * L)
{
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Ref);
    if (uin->atype == ALLOC_LEPDF && ((Ref *) uin->d) != NULL)
        gfree(((Ref *) uin->d));
    return 0;
}

static const struct luaL_Reg Ref_m[] = {
    {"__index", m_Ref__index},
    {"__tostring", m_Ref__tostring},
    {"__gc", m_Ref__gc},
    {NULL, NULL}
};

/* Stream */

static const char *StreamKindNames[] = {
    "File", "ASCIIHex", "ASCII85", "LZW", "RunLength", "CCITTFax", "DCT",
    "Flate", "JBIG2", "JPX", "Weird",
    NULL
};

m_poppler_get_INT(Stream, getKind);

static int m_Stream_getKindName(lua_State * L)
{
    StreamKind t;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_Stream);
    t = ((Stream *) uin->d)->getKind();
    lua_pushstring(L, StreamKindNames[t]);
    return 1;
}

m_poppler_do(Stream, reset);
m_poppler_do(Stream, close);
m_poppler_get_INT(Stream, getChar);
m_poppler_get_INT(Stream, lookChar);
m_poppler_get_INT(Stream, getRawChar);
m_poppler_get_INT(Stream, getUnfilteredChar);
m_poppler_do(Stream, unfilteredReset);
m_poppler_get_INT(Stream, getPos);
m_poppler_get_BOOL(Stream, isBinary);
m_poppler_get_poppler(Stream, Stream, getUndecodedStream);
m_poppler_get_poppler(Stream, Dict, getDict);

m_poppler__tostring(Stream);

static const struct luaL_Reg Stream_m[] = {
    {"getKind", m_Stream_getKind},
    {"getKindName", m_Stream_getKindName},      // not poppler
    {"reset", m_Stream_reset},
    {"close", m_Stream_close},
    {"getUndecodedStream", m_Stream_getUndecodedStream},
    {"getChar", m_Stream_getChar},
    {"lookChar", m_Stream_lookChar},
    {"getRawChar", m_Stream_getRawChar},
    {"getUnfilteredChar", m_Stream_getUnfilteredChar},
    {"unfilteredReset", m_Stream_unfilteredReset},
    {"getPos", m_Stream_getPos},
    {"isBinary", m_Stream_isBinary},
    {"getUndecodedStream", m_Stream_getUndecodedStream},
    {"getDict", m_Stream_getDict},
    {"__tostring", m_Stream__tostring},
    {NULL, NULL}
};

/* XRef */

m_poppler_get_OBJECT(XRef, getCatalog);

static int m_XRef_fetch(lua_State * L)
{
    int num, gen;
    udstruct *uin, *uout;
    uin = (udstruct *) luaL_checkudata(L, 1, M_XRef);
    num = luaL_checkint(L, 2);
    gen = luaL_checkint(L, 3);
    uout = new_Object_userdata(L);
    uout->d = new Object();
    *((Object *) uout->d) = ((XRef *) uin->d)->fetch(num, gen);
    uout->atype = ALLOC_LEPDF;
    uout->pc = uin->pc;
    uout->pd = uin->pd;
    return 1;
}

m_poppler_get_OBJECT(XRef, getDocInfo);
m_poppler_get_OBJECT(XRef, getDocInfoNF);
m_poppler_get_INT(XRef, getNumObjects);
m_poppler_get_INT(XRef, getRootNum);
m_poppler_get_INT(XRef, getRootGen);

static int m_XRef_getNumEntry(lua_State * L)
{
    int i, offset;
    udstruct *uin;
    uin = (udstruct *) luaL_checkudata(L, 1, M_XRef);
    offset = luaL_checkint(L, 2);
    i = ((XRef *) uin->d)->getNumEntry(offset);
    if (i >= 0)
        lua_pushinteger(L, i);
    else
        lua_pushnil(L);
    return 1;
}

m_poppler_get_poppler(XRef, Object, getTrailerDict);

m_poppler__tostring(XRef);

static const struct luaL_Reg XRef_m[] = {
    {"getCatalog", m_XRef_getCatalog},
    {"fetch", m_XRef_fetch},
    {"getDocInfo", m_XRef_getDocInfo},
    {"getDocInfoNF", m_XRef_getDocInfoNF},
    {"getNumObjects", m_XRef_getNumObjects},
    {"getRootNum", m_XRef_getRootNum},
    {"getRootGen", m_XRef_getRootGen},
    {"getNumEntry", m_XRef_getNumEntry},
    {"getTrailerDict", m_XRef_getTrailerDict},
    {"__tostring", m_XRef__tostring},
    {NULL, NULL}
};

/* XRefEntry */

m_poppler__tostring(XRefEntry);

static const struct luaL_Reg XRefEntry_m[] = {
    {"__tostring", m_XRefEntry__tostring},
    {NULL, NULL}
};

/* Done */

#ifdef LuajitTeX
#define setfuncs_meta(type)                 \
    luaL_newmetatable(L, M_##type);         \
    lua_pushvalue(L, -1);                   \
    lua_setfield(L, -2, "__index");         \
    lua_pushstring(L, "no user access");    \
    lua_setfield(L, -2, "__metatable");     \
    luaL_openlib(L, NULL, type##_m, 0)
#else
#define setfuncs_meta(type)                 \
    luaL_newmetatable(L, M_##type);         \
    lua_pushvalue(L, -1);                   \
    lua_setfield(L, -2, "__index");         \
    lua_pushstring(L, "no user access");    \
    lua_setfield(L, -2, "__metatable");     \
    luaL_setfuncs(L, type##_m, 0)
#endif

int luaopen_epdf(lua_State * L)
{
    setfuncs_meta(Array);
    setfuncs_meta(Catalog);
    setfuncs_meta(Dict);
    setfuncs_meta(Object);
    setfuncs_meta(PDFDoc);
        setfuncs_meta(Ref);
        setfuncs_meta(Stream);
    setfuncs_meta(XRef);
        setfuncs_meta(XRefEntry);
    luaL_openlib(L, "epdf", epdflib_f, 0);
    return 1;
}
