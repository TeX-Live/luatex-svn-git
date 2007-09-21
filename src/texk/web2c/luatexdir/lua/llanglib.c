/* $Id: lpdflib.c,v 1.12 2006/12/04 21:20:09 hahe Exp $ */

#include "luatex-api.h"
#include <ptexlib.h>

#include "hyphen.h"

#define DICT_METATABLE "luatex.dict"

#define check_isdict(L,b) (HyphenDict **)luaL_checkudata(L,b,DICT_METATABLE)

static void
lua_push_dict(lua_State *L, HyphenDict *dict) {
  HyphenDict **a;
  a = lua_newuserdata(L, sizeof(HyphenDict *));
  *a = dict;
  luaL_getmetatable(L,DICT_METATABLE);
  lua_setmetatable(L,-2);
  return;
}


static int 
lang_load_patterns (lua_State *L) {
  HyphenDict *dict;
  unsigned char *buffer;
  if (lua_isstring(L,1)) {
    buffer = (char *)lua_tostring(L,1);
    dict = hnj_hyphen_load (buffer);
    if (dict)
      lua_push_dict(L,dict);
    else
      lua_pushnil(L);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int 
lang_hyphenate_patterns (lua_State *L) {
  HyphenDict **dict_ptr;
  char *word, *hyphens = NULL;
  int i;
  dict_ptr = check_isdict(L,1);
  if (lua_isstring(L,2)) {
    word = (char *)lua_tostring(L,2);
    hyphens = xmalloc(13);
    i =  hnj_hyphen_hyphenate(*dict_ptr,word,strlen(word),hyphens);
    printf("hyphens: %s\n",hyphens);
  }
  return 0;
}


#define STORE_WORD()                    \
  if (w>0) {                            \
    word[w] = 0;                        \
    *s = 0;                             \
    if (!(limit>0 && w<limit)) {        \
    lua_pushlstring(L,(char *)word,w);  \
    lua_pushlstring(L,value,(s-value)); \
    lua_rawset(L,-3);                   \
    }                                   \
    w=0;                                \
}

static int 
lang_load_dictionary (lua_State *L) {
  char *filename;
  FILE *dict;
  char *s, *key, *value;
  int w, limit;
  integer len;
  unsigned char word [256];
  unsigned char *buffer = NULL;

  if (lua_isstring(L,1)) {
    filename = (char *)lua_tostring(L,1);
  }
  limit = luaL_optinteger (L,2,0);
  if((dict = fopen(filename,"rb"))) {
    if(readbinfile(dict,&buffer,&len)) {
      s = (char *)buffer;
      value = (char *)buffer;
      w = 0;
      lua_newtable(L);
      while (*s) {
		if (*s == ' ' || *s == '\r' || *s == '\t' || *s == '\n') {
		  STORE_WORD();
		  value = s+1;
		} else if (*s == '-' || *s == '=') {
		  /* skip */
		} else {
		  word[w] = *s;  
		  if (w<255) 
			w++;
		}
		s++;
      }
      /* fix a trailing word */
      STORE_WORD();
    } else {
      lua_pushnil(L);
    }
    fclose(dict);
    free(buffer);
  } else {
    lua_pushnil(L);
  }
  return 1;
}


static const struct luaL_reg langlib_d [] = {
  {"hyphenate",  lang_hyphenate_patterns},
  {NULL, NULL}  /* sentinel */
};


static const struct luaL_reg langlib[] = {
    {"patterns",   lang_load_patterns},
    {"hyphenate",  lang_hyphenate_patterns},
    {"dictionary", lang_load_dictionary},
    {NULL, NULL}                /* sentinel */
};


int 
luaopen_lang (lua_State *L) {
  luaL_newmetatable(L,DICT_METATABLE);
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_register(L, NULL, langlib_d);  /* dict methods */

  luaL_register(L, "lang", langlib);
  return 1;
}

