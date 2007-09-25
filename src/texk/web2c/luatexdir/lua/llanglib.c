/* $Id: lpdflib.c,v 1.12 2006/12/04 21:20:09 hahe Exp $ */

#include "luatex-api.h"
#include <ptexlib.h>

#include "hyphen.h"

#define LANG_METATABLE "luatex.lang"
#define MAX_WORD_LEN 255 /* in bytes, not chars */

#define check_islang(L,b) (struct tex_language *)luaL_checkudata(L,b,LANG_METATABLE)
#define max(a,b) ((a)>(b)? (b) : (a))

struct tex_language {
  HyphenDict *patterns;
  int exceptions; /* lua registry pointer, should be replaced */
  int lhmin;
  int rhmin;
  int id;
};


static int lang_new (lua_State *L) {
  struct tex_language *lang;
  lang = lua_newuserdata(L, sizeof(struct tex_language));
  lang->id = luaL_optinteger (L,2,0); /* default to zero (!) */
  lang->lhmin = 2;
  lang->rhmin = 3;
  lang->exceptions = 0;
  lang->patterns = NULL;
  luaL_getmetatable(L,LANG_METATABLE);
  lua_setmetatable(L,-2);
  return 1;
}

static int 
lang_exceptions (lua_State *L) {
  struct tex_language *lang_ptr;
  int i;
  lang_ptr = check_islang(L,1);
  if (lua_gettop(L)==2 && lua_istable(L,2)) {
	lang_ptr->exceptions = luaL_ref(L,LUA_REGISTRYINDEX);
    return 0;
  } else {
	if (lang_ptr->exceptions==0) {
	  lua_pushnil(L);
	} else {
      lua_rawgeti(L,LUA_REGISTRYINDEX,lang_ptr->exceptions);
	}
	return 1;
  }
}

static int 
lang_rhmin (lua_State *L) {
  struct tex_language *lang_ptr;
  int i;
  lang_ptr = check_islang(L,1);
  if ((i = luaL_optinteger(L,2,-1))>-1) {
    lang_ptr->rhmin = max(i,MAX_WORD_LEN);
    return 0;
  } else {
    lua_pushnumber(L,lang_ptr->rhmin);
    return 1;
  }
}


static int 
lang_lhmin (lua_State *L) {
  struct tex_language *lang_ptr;
  int i;
  lang_ptr = check_islang(L,1);
  if ((i = luaL_optinteger(L,2,-1))>-1) {
    lang_ptr->lhmin = max(i,MAX_WORD_LEN);
    return 0;
  } else {
    lua_pushnumber(L,lang_ptr->lhmin);
    return 1;
  }
}

static int 
lang_id (lua_State *L) {
  struct tex_language *lang_ptr;
  int i;
  lang_ptr = check_islang(L,1);
  if ((i = luaL_optinteger(L,2,-1))>-1) {
    lang_ptr->id = i;
    return 0;
  } else {
    lua_pushnumber(L,lang_ptr->id);
    return 1;
  }
}

static int 
lang_patterns (lua_State *L) {
  struct tex_language *lang_ptr;
  char *buffer;
  lang_ptr = check_islang(L,1);
  if (lua_isstring(L,2)) {
    buffer = (char *)lua_tostring(L,2);
    lang_ptr->patterns = hnj_hyphen_load (buffer);
  }
  return 1;
}

#define STORE_WORD()                    \
  if (w>0) {                            \
    word[w] = 0;                        \
    *s = 0;                             \
    lua_pushlstring(L,(char *)word,w);  \
    lua_pushlstring(L,value,(s-value)); \
    lua_rawset(L,-3);                   \
    w=0;                                \
}

#define STORE_CHAR(x)				\
  { word[w] = x ; if (w<MAX_WORD_LEN) w++; }


static void
lang_load_exceptions (lua_State *L, unsigned char *buffer) {
  char *s, *value;
  unsigned char word [MAX_WORD_LEN+1];
  int w = 0;
  s = value = (char *)buffer;
  while (*s) {
	if (isspace(*s)) {
      STORE_WORD();
      value = s+1;
    } else if (*s == '-') {	 /* skip */
    } else if (*s == '=') {
      STORE_CHAR('-');
	  *s = '-' ; 
    } else {
      STORE_CHAR(*s); 
    }
    s++;
  }
  /* fix a trailing word */
  STORE_WORD();
}

static int 
lang_hyphenation (lua_State *L) {
  struct tex_language *lang_ptr;
  unsigned char *buffer;

  lang_ptr = check_islang(L,1);
  if (!lua_isstring(L,2)) {
    lua_pushstring(L,"lang.hyphenation(): argument should be a string");
    return lua_error(L);
  }
  buffer = (unsigned char *)lua_tostring(L,2);
  /* start a ref if it doesn't exist yet */
  if (lang_ptr->exceptions==0) {
    lua_newtable(L);
    lang_ptr->exceptions = luaL_ref(L,LUA_REGISTRYINDEX);
  }
  lua_rawgeti(L, LUA_REGISTRYINDEX, lang_ptr->exceptions);
  lang_load_exceptions(L,buffer);
  return 1;
}

static int 
lang_hyphenate (lua_State *L) {
  int c,i,k,l,r;
  struct tex_language *lang_ptr;
  char *word;
  char hyphenated[2*MAX_WORD_LEN] = {0};
  char hyphens[MAX_WORD_LEN] = {0};
  lang_ptr = check_islang(L,1);
  if (lua_isstring(L,2)) { /* word lang */
    if (lang_ptr->exceptions!=0) {
      lua_rawgeti(L,LUA_REGISTRYINDEX,lang_ptr->exceptions);
      if (lua_istable(L,-1)) { /* ?? word lang */
		lua_pushvalue(L,-2);    /* word table word lang */
		lua_rawget(L,-2);    
		if (lua_isstring(L,-1)) { /* ?? table word lang */
		  lua_replace(L,-3); 
		  lua_pop(L,1); 
		  return 1; 
		} else {
		  lua_pop(L,2);
		}
      } else {
		lua_pop(L,1);
      }
    };
    word = (char *)lua_tolstring(L,2,&l);
	if (l>=MAX_WORD_LEN) {
	  lua_pushstring(L,"lang:hyphenate(): word too long");
	  return lua_error(L);
	}
    (void)hnj_hyphen_hyphenate(lang_ptr->patterns,word,l,hyphens);
	r = (lang_ptr->rhmin>0 ? lang_ptr->rhmin : 1);
	for (c=0,i=0,k=0;i<l;) {
	  hyphenated[k++] = word[i];
	  if (hyphens[c]!='0' 
		  && c<(l-r) 
		  && c>=lang_ptr->lhmin ) {
		hyphenated[k++] = '-';
	  }
	  i++; 
	  c++;
	}
	hyphenated[k]=0;
    lua_pushstring(L,hyphenated);
    return 1;
  }
  return 0;
}

static const struct luaL_reg langlib_d [] = {
  {"hyphenate",       lang_hyphenate},
  {"patterns",        lang_patterns},
  {"hyphenation",      lang_hyphenation},
  {"exceptions",      lang_exceptions},
  {"lefthyphenmin",   lang_lhmin},
  {"righthyphenmin",  lang_rhmin},
  {"id",              lang_id},
  {NULL, NULL}  /* sentinel */
};


static const struct luaL_reg langlib[] = {
  {"new",        lang_new},
  {NULL, NULL}                /* sentinel */
};


int 
luaopen_lang (lua_State *L) {
  luaL_newmetatable(L,LANG_METATABLE);
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_register(L, NULL, langlib_d);  /* dict methods */

  luaL_register(L, "lang", langlib);
  return 1;
}

