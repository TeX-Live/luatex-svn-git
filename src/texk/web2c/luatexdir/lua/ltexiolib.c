/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

typedef void (*texio_printer) (strnumber s);

static char *loggable_info = NULL;

static int
do_texio_print (lua_State *L, texio_printer printfunction) {
  strnumber s,u;
  char *outputstr;
  char save_selector;
  int n;
  u = 0;
  n = lua_gettop(L);
  if (!lua_isstring(L, -1)) {
    lua_pushstring(L, "no string to print");
    lua_error(L);
  }
  if (n==2) {
    if (!lua_isstring(L, -2)) {
      lua_pushstring(L, "invalid argument for print selector");
      lua_error(L);
    } else {
      save_selector = selector;  
      outputstr=(char *)lua_tostring(L, -2);
      if      (strcmp(outputstr,"log") == 0)          { selector = log_only;     }
      else if (strcmp(outputstr,"term") == 0)         { selector = term_only;    }
      else if (strcmp(outputstr,"term and log") == 0) {	selector = term_and_log; }
      else {
		lua_pushstring(L, "invalid argument for print selector");
		lua_error(L);
      }
    }
  } else {
    if (n!=1) {
      lua_pushstring(L, "invalid number of arguments");
      lua_error(L);
    }
    save_selector = selector;
    if (selector!=log_only &&
	selector!=term_only &&
	selector != term_and_log) {
      normalize_selector(); /* sets selector */
    }
  }
  if (str_start[str_ptr-0x200000]<pool_ptr) 
    u=make_string();
  s = maketexstring(lua_tostring(L, -1));
  printfunction(s);
  flush_str(s);
  selector = save_selector;
  if (u!=0) str_ptr--;
  return 0; 
}

static int
texio_print (lua_State *L) {
  char *s;
  if (ready_already!=314159 || pool_ptr==0) {
	if(lua_isstring(L, -1)) {
	  s = (char *)lua_tostring(L, -1);
	  fprintf(stdout,"%s",s);
	  if ((lua_isstring(L, -2) && strcmp(lua_tostring(L, -2),"term"))||
		  (!lua_isstring(L, -2))) {
		if (loggable_info==NULL)
		  loggable_info = s;
		else
		  loggable_info = concat (loggable_info,s);
	  } else {
	  }
	}
	return 0;
  }
  return do_texio_print(L,zprint);
}

static int
texio_printnl (lua_State *L) {
  char *s;
  if (ready_already!=314159 || pool_ptr==0) {
	if(lua_isstring(L, -1)) {
	  s = (char *)lua_tostring(L, -1);
	  fprintf(stdout,"\n%s",s);
	  if ((lua_isstring(L, -2) && strcmp(lua_tostring(L, -2),"term"))||
          (!lua_isstring(L, -2))) {
		if (loggable_info==NULL)
		  loggable_info = s;
		else
		  loggable_info = concat3 (loggable_info,"\n", s);
	  }
	}
	return 0;
  }
  return do_texio_print(L,zprint_nl);
}

/* at the point this function is called, the selector is log_only */
void flush_loggable_info (void) {
  if (loggable_info!=NULL) {
	fprintf(log_file,"%s\n",loggable_info);
	free(loggable_info);
	loggable_info=NULL;
  }
}


static const struct luaL_reg texiolib [] = {
  {"write", texio_print},
  {"write_nl", texio_printnl},
  {NULL, NULL}  /* sentinel */
};

int
luaopen_texio (lua_State *L) {
  luaL_register(L,"texio",texiolib);
  return 1;
}
