/* $Id$ */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <ptexlib.h>
#include "../image/image.h"

/**********************************************************************/

#ifdef DEBUG
static void stackDump(lua_State * L, char *s)
{
    int i;
    int top = lua_gettop(L);
    printf("\n=== stackDump <%s>: ", s);
    for (i = 1; i <= top; i++) {        /* repeat for each level */
        int t = lua_type(L, i);
        printf("%d: ", i);
        switch (t) {
        case LUA_TSTRING:      /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:     /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
        case LUA_TNUMBER:      /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
        default:               /* other values */
            printf("%s", lua_typename(L, t));
            break;
        }
        printf("  ");           /* put a separator */
    }
    printf("\n");
}
#endif

/**********************************************************************/

typedef enum { P__ZERO, P_ATTRIB, P_COLORDEPTH, P_COLORSPACE_OBJ, P_DEPTH,
    P_FILENAME, P_FILEPATH, P_HEIGHT, P_IMAGETYPE, P_INDEX, P_OBJNUM,
    P_PAGEBOX, P_PAGENAME, P_PAGENUM, P_TOTALPAGES, P_WIDTH, P_XRES,
    P_XSIZE, P_YRES, P_YSIZE, P__SENTINEL
} parm_idx;

typedef struct {
    const char *name;           /* parameter name */
    parm_idx idx;               /* index within img_parms array */
} parm_struct;

parm_struct img_parms[] = {
    {NULL, P__ZERO},            /* dummy; lua indices run from 1 */
    {"attrib", P_ATTRIB},
    {"colordepth", P_COLORDEPTH},
    {"colorspaceobj", P_COLORSPACE_OBJ},
    {"depth", P_DEPTH},
    {"filename", P_FILENAME},
    {"filepath", P_FILEPATH},
    {"height", P_HEIGHT},
    {"imagetype", P_IMAGETYPE},
    {"index", P_INDEX},
    {"objnum", P_OBJNUM},
    {"pagebox", P_PAGEBOX},
    {"pagename", P_PAGENAME},
    {"page", P_PAGENUM},
    {"pages", P_TOTALPAGES},
    {"width", P_WIDTH},
    {"xres", P_XRES},
    {"xsize", P_XSIZE},
    {"yres", P_YRES},
    {"ysize", P_YSIZE},
    {NULL, P__SENTINEL}
};

#define imgtype_max 4
const char *imgtype_s[] = { "none", "pdf", "png", "jpg", "jbig2", NULL };

#define pdfboxspec_max 5
const char *pdfboxspec_s[] =
    { "none", "media", "crop", "bleed", "trim", "art", NULL };

/**********************************************************************/

static void image_to_lua(lua_State * L, image * a)
{                               /* key user ... */
    int i, j;
    image_dict *d = img_dict(a);
    assert(d != NULL);
    lua_pushvalue(L, -1);       /* k k u ... */
    lua_gettable(L, LUA_ENVIRONINDEX);  /* i? k u ... */
    if (!lua_isnumber(L, -1))   /* !i k u ... */
        luaL_error(L, "image_to_lua not a valid image key: %s",
                   lua_tostring(L, -2));
    i = lua_tointeger(L, -1);   /* i k u ... */
    lua_pop(L, 2);              /* u ... */
    switch (i) {
    case P_FILENAME:
        if (img_filename(d) == NULL || strlen(img_filename(d)) == 0)
            lua_pushnil(L);
        else
            lua_pushstring(L, img_filename(d));
        break;
    case P_FILEPATH:
        if (img_filepath(d) == NULL || strlen(img_filepath(d)) == 0)
            lua_pushnil(L);
        else
            lua_pushstring(L, img_filepath(d));
        break;
    case P_ATTRIB:
        if (img_attrib(d) == NULL || strlen(img_attrib(d)) == 0)
            lua_pushnil(L);
        else
            lua_pushstring(L, img_attrib(d));
        break;
    case P_PAGENUM:
        lua_pushinteger(L, img_pagenum(d));
        break;
    case P_PAGENAME:
        if (img_pagename(d) == NULL || strlen(img_pagename(d)) == 0)
            lua_pushnil(L);
        else
            lua_pushstring(L, img_pagename(d));
        break;
    case P_TOTALPAGES:
        lua_pushinteger(L, img_totalpages(d));
        break;
    case P_WIDTH:
        if (is_wd_running(a))
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_width(a));
        break;
    case P_HEIGHT:
        if (is_ht_running(a))
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_height(a));
        break;
    case P_DEPTH:
        if (is_dp_running(a))
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_depth(a));
        break;
    case P_XSIZE:
        lua_pushinteger(L, img_xsize(d));
        break;
    case P_YSIZE:
        lua_pushinteger(L, img_ysize(d));
        break;
    case P_XRES:
        lua_pushinteger(L, img_xres(d));
        break;
    case P_YRES:
        lua_pushinteger(L, img_yres(d));
        break;
    case P_COLORSPACE_OBJ:
        if (img_colorspace_obj(d) == 0)
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_colorspace_obj(d));
        break;
    case P_COLORDEPTH:
        if (img_colordepth(d) == 0)
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_colordepth(d));
        break;
    case P_IMAGETYPE:
        j = img_type(d);
        if (j >= 0 && j <= imgtype_max) {
            if (j == IMAGE_TYPE_NONE)
                lua_pushnil(L);
            else
                lua_pushstring(L, imgtype_s[j]);
        } else
            assert(0);
        break;
    case P_PAGEBOX:
        j = img_pageboxspec(d);
        if (j >= 0 && j <= pdfboxspec_max) {
            if (j == PDF_BOX_SPEC_NONE)
                lua_pushnil(L);
            else
                lua_pushstring(L, pdfboxspec_s[j]);
        } else
            assert(0);
        break;
    case P_OBJNUM:
        if (img_objnum(d) == 0)
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_objnum(d));
        break;
    case P_INDEX:
        if (img_index(d) == 0)
            lua_pushnil(L);
        else
            lua_pushinteger(L, img_index(d));
        break;
    default:
        assert(0);
    }                           /* v u ... */
}

static void lua_to_image(lua_State * L, image * a)
{                               /* value key table ... */
    int i;
    image_dict *d = img_dict(a);
    assert(d != NULL);
    lua_pushvalue(L, -2);       /* k v k t ... */
    lua_gettable(L, LUA_ENVIRONINDEX);  /* i? v k t ... */
    if (!lua_isnumber(L, -1))   /* !i v k t ... */
        luaL_error(L, "lua_to_image not a valid image key: %s",
                   lua_tostring(L, -3));
    i = lua_tointeger(L, -1);   /* i v k t ... */
    lua_pop(L, 1);              /* v k t ... */
    switch (i) {
    case P_WIDTH:
        if (img_is_refered(a))
            luaL_error(L, "image.width is now read-only");
        if (lua_isnil(L, -1))
            set_wd_running(a);
        else if (lua_isnumber(L, -1)) {
            img_width(a) = lua_tointeger(L, -1);
        } else
            luaL_error(L, "image.width needs integer or nil value");
        img_unset_scaled(a);
        break;
    case P_HEIGHT:
        if (img_is_refered(a))
            luaL_error(L, "image.height is now read-only");
        if (lua_isnil(L, -1)) {
            set_ht_running(a);
        } else if (lua_isnumber(L, -1)) {
            img_height(a) = lua_tointeger(L, -1);
        } else
            luaL_error(L, "image.height needs integer or nil value");
        img_unset_scaled(a);
        break;
    case P_DEPTH:
        if (img_is_refered(a))
            luaL_error(L, "image.depth is now read-only");
        if (lua_isnil(L, -1))
            set_dp_running(a);
        else if (lua_isnumber(L, -1)) {
            img_depth(a) = lua_tointeger(L, -1);
        } else
            luaL_error(L, "image.depth needs integer or nil value");
        img_unset_scaled(a);
        break;
    case P_FILENAME:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.filename is now read-only");
        if (lua_isstring(L, -1) || lua_isnil(L, -1)) {
            if (img_filename(d) != NULL)
                xfree(img_filename(d));
            if (lua_isstring(L, -1))
                img_filename(d) = xstrdup(lua_tostring(L, -1));
        } else
            luaL_error(L, "image.filename needs string or nil value");
        break;
    case P_FILEPATH:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.filepath is now read-only");
        if (lua_isstring(L, -1) || lua_isnil(L, -1)) {
            if (img_filepath(d) != NULL)
                xfree(img_filepath(d));
            if (lua_isstring(L, -1))
                img_filepath(d) = xstrdup(lua_tostring(L, -1));
        } else
            luaL_error(L, "image.filepath needs string or nil value");
        break;
    case P_ATTRIB:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.attrib is now read-only");
        if (lua_isstring(L, -1) || lua_isnil(L, -1)) {
            if (img_attrib(d) != NULL)
                xfree(img_attrib(d));
            if (lua_isstring(L, -1))
                img_attrib(d) = xstrdup(lua_tostring(L, -1));
        } else
            luaL_error(L, "image.attrib needs string or nil value");
        break;
    case P_PAGENAME:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.pagename is now read-only");
        if (lua_isstring(L, -1) || lua_isnil(L, -1)) {
            if (img_pagename(d) != 0)
                xfree(img_pagename(d));
            if (lua_isstring(L, -1))
                img_pagename(d) = xstrdup(lua_tostring(L, -1));
        } else
            luaL_error(L, "image.pagename needs string or nil value");
        break;
    case P_COLORSPACE_OBJ:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.colorspaceobj is now read-only");
        if (lua_isnil(L, -1))
            img_colorspace_obj(d) = 0;
        else if (lua_isnumber(L, -1)) {
            img_colorspace_obj(d) = lua_tointeger(L, -1);
        } else
            luaL_error(L,
                       "image.colorspaceobj needs pos. integer or nil value");
        break;
    case P_PAGENUM:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.page is now read-only");
        if (lua_isnil(L, -1))
            img_pagenum(d) = 1;
        else if (lua_isnumber(L, -1)) {
            img_pagenum(d) = lua_tointeger(L, -1);
        } else
            luaL_error(L, "image.page needs integer or nil value");
        break;
    case P_PAGEBOX:
        if (img_state(d) >= DICT_FILESCANNED)
            luaL_error(L, "image.pagebox is now read-only");
        if (lua_isnil(L, -1))
            img_pageboxspec(d) = PDF_BOX_SPEC_NONE;
        else if (lua_isstring(L, -1)) {
            img_pageboxspec(d) = luaL_checkoption(L, -1, "none", pdfboxspec_s);
        } else
            luaL_error(L, "image.pagebox needs string or nil value");
        break;
    case P_TOTALPAGES:
    case P_XSIZE:
    case P_YSIZE:
    case P_XRES:
    case P_YRES:
    case P_IMAGETYPE:
    case P_OBJNUM:
    case P_INDEX:
    case P_COLORDEPTH:
        luaL_error(L, "image.%s is a read-only variable", img_parms[i].name);
        break;
    default:
        assert(0);
    }                           /* v k t ... */
}

/**********************************************************************/

void fix_image_size(lua_State * L, image * a)
{
    if (!img_is_scaled(a) || is_wd_running(a) || is_ht_running(a)
        || is_dp_running(a)) {
        if (img_is_refered(a))
            luaL_error(L, "image is read-only");
        scale_img(a);
    }
}

void copy_image(lua_State * L, lua_Number scale)
{
    image *a, **aa, *b, **bb;
    if (lua_gettop(L) != 1)
        luaL_error(L, "img.copy needs exactly 1 argument");
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);    /* a */
    lua_pop(L, 1);              /* - */
    a = *aa;
    bb = (image **) lua_newuserdata(L, sizeof(image *));        /* b */
    luaL_getmetatable(L, TYPE_IMG);     /* m b */
    lua_setmetatable(L, -2);    /* b */
    b = *bb = new_image();
    if (!is_wd_running(a))
        img_width(b) = img_width(a) * scale;
    if (!is_ht_running(a))
        img_height(b) = img_height(a) * scale;
    if (!is_dp_running(a))
        img_depth(b) = img_depth(a) * scale;
    img_dict(b) = img_dict(a);
    if (img_dictref(a) != LUA_NOREF) {
        lua_rawgeti(L, LUA_GLOBALSINDEX, img_dictref(a));       /* ad b */
        img_dictref(b) = luaL_ref(L, LUA_GLOBALSINDEX); /* b */
    } else
        assert(img_state(img_dict(a)) >= DICT_REFERED);
}

/**********************************************************************/

static int l_new_image(lua_State * L)
{
    image *a, **aa;
    int i;
    if (lua_gettop(L) > 1)
        luaL_error(L, "img.new needs maximum 1 argument");
    if (lua_gettop(L) == 1 && !lua_istable(L, 1))
        luaL_error(L, "img.new needs table as optional argument");      /* (t) */
    aa = (image **) lua_newuserdata(L, sizeof(image *));        /* i (t) */
    luaL_getmetatable(L, TYPE_IMG);     /* m i (t) */
    lua_setmetatable(L, -2);    /* i (t) */
    a = *aa = new_image();
    image_dict **add = (image_dict **) lua_newuserdata(L, sizeof(image_dict *));        /* ad i (t) */
    luaL_getmetatable(L, TYPE_IMG_DICT);        /* m ad i (t) */
    lua_setmetatable(L, -2);    /* ad i (t) */
    img_dict(a) = *add = new_image_dict();
    img_dictref(a) = luaL_ref(L, LUA_GLOBALSINDEX);     /* i (t) */
    if (lua_gettop(L) != 1) {   /* i t, else just i */
        lua_insert(L, -2);      /* t i */
        lua_pushnil(L);         /* n t i (1st key for iterator) */
        while (lua_next(L, -2) != 0) {  /* v k t i */
            lua_to_image(L, a); /* v k t i */
            lua_pop(L, 1);      /* k t i */
        }                       /* t i */
        lua_pop(L, 1);          /* i */
    }                           /* i */
    return 1;                   /* i */
}

static int l_copy_image(lua_State * L)
{
    if (lua_gettop(L) != 1)
        luaL_error(L, "img.copy needs exactly 1 argument");
    if (lua_istable(L, 1))
        l_new_image(L);         /* image --- if everything worked well */
    else
        copy_image(L, 1.0);     /* image */
    return 1;                   /* image */
}

static int l_scan_image(lua_State * L)
{
    image *a, **aa;
    char *s;
    if (lua_gettop(L) != 1)
        luaL_error(L, "img.scan needs exactly 1 argument");
    if (lua_istable(L, 1))
        l_new_image(L);         /* image --- if everything worked well */
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);    /* image */
    a = *aa;
    image_dict *ad = img_dict(a);
    if (img_state(ad) == DICT_NEW) {
        read_img(ad, 1, NULL, get_pdf_minor_version(),
                 get_pdf_inclusion_errorlevel());
        img_unset_scaled(a);
    }
    fix_image_size(L, a);
    return 1;                   /* image */
}

static int l_write_image(lua_State * L)
{
    image *a, **aa;
    integer ref, k;
    if (lua_gettop(L) != 1)
        luaL_error(L, "img.write needs exactly 1 argument");
    if (lua_istable(L, 1))
        l_new_image(L);         /* image --- if everything worked well */
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);    /* image */
    a = *aa;
    image_dict *ad = img_dict(a);
    if (img_state(ad) == DICT_NEW) {
        read_img(ad, 1, NULL, get_pdf_minor_version(),
                 get_pdf_inclusion_errorlevel());
        img_unset_scaled(a);
    }
    fix_image_size(L, a);
    ref = img_to_array(a);
    k = lua_refximage(ref);
    img_objnum(ad) = k;
    if (img_state(ad) < DICT_REFERED)
        img_state(ad) = DICT_REFERED;
    img_set_refered(a);         /* now image may not be freed by gc */
    return 1;                   /* image */
}

static int l_image_keys(lua_State * L)
{
    parm_struct *p = img_parms + 1;
    if (lua_gettop(L) != 0)
        luaL_error(L, "img.keys goes without argument");
    lua_newtable(L);            /* t */
    for (; p->name != NULL; p++) {
        lua_pushinteger(L, (int) p->idx);       /* k t */
        lua_pushstring(L, p->name);     /* v k t */
        lua_settable(L, -3);    /* t */
    }
    return 1;
}

static const struct luaL_Reg imglib[] = {
    {"new", l_new_image},
    {"copy", l_copy_image},
    {"scan", l_scan_image},
    {"write", l_write_image},
    {"keys", l_image_keys},
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* Metamethods for image */

static int m_img_get(lua_State * L)
{
    image *a, **aa;
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);    /* k u */
    a = *aa;
    image_dict *ad = img_dict(a);
    image_to_lua(L, a);         /* v u */
    return 1;
}

static int m_img_set(lua_State * L)
{
    image *a, **aa;
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);    /* value key user */
    a = *aa;
    lua_to_image(L, a);         /* v k u */
    return 0;
}

static int m_img_mul(lua_State * L)
{
    image *a, **aa;
    lua_Number scale;
    if (lua_isnumber(L, 1)) {   /* u? n */
        aa = (image **) luaL_checkudata(L, 2, TYPE_IMG);        /* u n */
        lua_insert(L, -2);      /* n a */
    } else if (lua_isnumber(L, 2)) {    /* n u? */
        aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);        /* n a */
    }                           /* n a */
    scale = lua_tonumber(L, 2); /* n a */
    lua_pop(L, 1);              /* a */
    copy_image(L, scale);       /* b */
    return 1;
}

static int m_img_gc(lua_State * L)
{
    image *a, **aa;
    aa = (image **) luaL_checkudata(L, 1, TYPE_IMG);
    a = *aa;
#ifdef DEBUG
    printf("\n===== IMG GC ===== a=%d ad=%d\n", a, img_dict(a));
#endif
    luaL_unref(L, LUA_GLOBALSINDEX, img_dictref(a));
    if (!img_is_refered(a))
        xfree(a);
    return 0;
}

static const struct luaL_Reg img_m[] = {
    {"__index", m_img_get},
    {"__newindex", m_img_set},
    {"__mul", m_img_mul},
    {"__gc", m_img_gc},         /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/
/* Metamethods for image_dict */

static int m_img_dict_gc(lua_State * L)
{
    image_dict *ad, **add;
    add = (image_dict **) luaL_checkudata(L, 1, TYPE_IMG_DICT);
    ad = *add;
#ifdef DEBUG
    printf("\n===== IMG_DICT GC FREE ===== ad=%d\n", ad);
#endif
    if (img_state(ad) < DICT_REFERED)
        free_image_dict(ad);
}

static const struct luaL_Reg img_dict_m[] = {
    {"__gc", m_img_dict_gc},    /* finalizer */
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/

void preset_environment(lua_State * L, parm_struct * p)
{
    int i;
    lua_newtable(L);            /* t */
    for (i = 1, ++p; p->name != NULL; i++, p++) {
        assert(i == (int) p->idx);
        lua_pushstring(L, p->name);     /* k t */
        lua_pushinteger(L, (int) p->idx);       /* v k t */
        lua_settable(L, -3);    /* t */
    }
    lua_replace(L, LUA_ENVIRONINDEX);   /* - */
}

int luaopen_img(lua_State * L)
{
    preset_environment(L, img_parms);
    luaL_newmetatable(L, TYPE_IMG);
    luaL_register(L, NULL, img_m);
    luaL_newmetatable(L, TYPE_IMG_DICT);
    luaL_register(L, NULL, img_dict_m);
    luaL_register(L, "img", imglib);
    return 1;
}

/**********************************************************************/
