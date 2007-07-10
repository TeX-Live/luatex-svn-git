/* $Id: lpdflib.c,v 1.12 2006/12/04 21:20:09 hahe Exp $ */

#include "luatex-api.h"
#include <ptexlib.h>


int findcurv (lua_State *L) {
  int j;
  j = get_cur_v();
  lua_pushnumber(L, j);
  return 1;
}

int findcurh (lua_State *L) {
  int j;
  j = get_cur_h();
  lua_pushnumber(L, j);
  return 1;
}

int makecurv (lua_State *L) {
  lua_pop(L,1); /* table at -1 */
  return 0;
}

int makecurh (lua_State *L) {
  lua_pop(L,1); /* table at -1 */
  return 0;
}

typedef enum { set_origin, direct_page, direct_always } pdf_lit_mode;

int luapdfprint(lua_State * L)
{
    int i, j, k, n, len;
    const char *outputstr, *st;
    pdf_lit_mode literal_mode;
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
                lua_pushstring(L,
                               "invalid argument for print literal mode");
                lua_error(L);
            }
        }
    } else {
	    if (n != 1) {
            lua_pushstring(L, "invalid number of arguments");
            lua_error(L);
        }
    }
    switch (literal_mode) {
    case (set_origin):
        pdf_end_text();
        pdf_set_origin(cur_h, cur_v);
        break;
    case (direct_page):
        pdf_end_text();
        break;
    case (direct_always):
        pdf_end_string_nl();
        break;
    default:
        assert(0);
    }
    st = lua_tolstring(L, n,&len);
    for (i = 0; i < len; i++) {
	  if (i%16 == 0) 
        pdfroom(16);
	  pdf_buf[pdf_ptr++] = st[i];
    }
    return 0;
}

static const struct luaL_reg pdflib[] = {
    {"print", luapdfprint},
    {"getv", findcurv},
    {"geth", findcurh},
    {"setv", makecurv},
    {"seth", makecurh},
    {NULL, NULL}                /* sentinel */
};

int luaopen_pdf (lua_State *L) 
{
  luaL_register(L, "pdf", pdflib);
  make_table(L,"v","getv","setv");
  make_table(L,"h","geth","seth");
  return 1;
}

