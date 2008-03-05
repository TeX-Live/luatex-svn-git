/* $Id$ */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef pdfTeX
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#else
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <../lua51/lualib.h>
#endif

#include "mplib.h"
#include "mpmp.h"
#include "mppsout.h" /* for mp_edge_object */

#define MPLIB_METATABLE     "MPlib"
#define MPLIB_FIG_METATABLE "MPlib.fig"
#define MPLIB_GR_METATABLE  "MPlib.gr"

#define xfree(A) if ((A)!=NULL) { free((A)); A = NULL; }

#define is_mp(L,b) (MP *)luaL_checkudata(L,b,MPLIB_METATABLE)
#define is_fig(L,b) (struct mp_edge_object **)luaL_checkudata(L,b,MPLIB_FIG_METATABLE)
#define is_gr_object(L,b) (struct mp_graphic_object **)luaL_checkudata(L,b,MPLIB_GR_METATABLE)

/* Enumeration string arrays */

static const char *interaction_options[] = 
  { "unknown","batch","nonstop","scroll","errorstop", NULL};

static const char *mplib_filetype_names[] = 
  {"term", "error", "mp", "log", "ps", "mem", "tfm", "map", "pfb", "enc", NULL};

/* only "endpoint" and "explicit" actually happen in paths, 
   as well as "open" in elliptical pens */

static const char *knot_type_enum[]  = 
  { "endpoint", "explicit", "given", "curl", "open", "end_cycle"  };

static const char *knot_originator_enum[]  = 
  { "program" ,"user" };


/* this looks a bit funny because of the holes */

static const char *color_model_enum[] = 
  { NULL, "none",  NULL, "grey", NULL, "rgb", NULL, "cmyk", NULL, "uninitialized" };

/* object fields */

#define FIELD(A) (strcmp(field,#A)==0)

static const char *fill_fields[] = 
  { "type", "path", "htap", "pen", "color", "linejoin", "miterlimit", 
    "prescript", "postscript", NULL };

static const  char *stroked_fields[] = 
  { "type", "path", "pen", "color", "linejoin", "miterlimit", "linecap", "dash", 
    "prescript", "postscript", NULL };

static const char *text_fields[] = 
  { "type", "text", "dsize", "font", "color", "width", "height", "depth", "transform", 
    "prescript", "postscript", NULL };

static const char *special_fields[] = 
  { "type", "prescript", NULL };

static const char *start_bounds_fields[] = 
  { "type", "path", NULL };

static const char *start_clip_fields[] = 
  { "type", "path", NULL };

static const char *stop_bounds_fields[] = 
  { "type", NULL };

static const char *stop_clip_fields[] = 
  { "type", NULL };

static const char *no_fields[] =  
  { NULL };

typedef enum {  
  P_ERROR_LINE,  P_HALF_LINE,   P_MAX_LINE,    P_MAIN_MEMORY, 
  P_HASH_SIZE,   P_HASH_PRIME,  P_PARAM_SIZE,  P_IN_OPEN,     P_RANDOM_SEED, 
  P_INTERACTION, P_INI_VERSION, P_TROFF_MODE,  P_PRINT_NAMES, P_MEM_NAME, 
  P_JOB_NAME,    P_FIND_FILE,   P__SENTINEL,
} mplib_parm_idx;

typedef struct {
    const char *name;           /* parameter name */
    mplib_parm_idx idx;         /* parameter index */
} mplib_parm_struct;

static mplib_parm_struct mplib_parms[] = {
  {"error_line",        P_ERROR_LINE   },
  {"half_error_line",   P_HALF_LINE    },
  {"max_print_line",    P_MAX_LINE     },
  {"main_memory",       P_MAIN_MEMORY  },
  {"hash_size",         P_HASH_SIZE    },
  {"hash_prime",        P_HASH_PRIME   },
  {"param_size",        P_PARAM_SIZE   },
  {"max_in_open",       P_IN_OPEN      },
  {"random_seed",       P_RANDOM_SEED  },
  {"interaction",       P_INTERACTION  },
  {"ini_version",       P_INI_VERSION  },
  {"troff_mode",        P_TROFF_MODE   },
  {"print_found_names", P_PRINT_NAMES  },
  {"mem_name",          P_MEM_NAME     },
  {"job_name",          P_JOB_NAME     },
  {"find_file",         P_FIND_FILE    },
  {NULL,                P__SENTINEL    }
};

typedef struct _FILE_ITEM {
  FILE *f;
} _FILE_ITEM ;

typedef struct _FILE_ITEM File;

typedef struct _MPLIB_INSTANCE_DATA {
  void *term_file_ptr;
  void *err_file_ptr;
  void *log_file_ptr;
  void *ps_file_ptr;
  char *term_out;
  char *error_out;
  char *log_out;
  char *ps_out;
  char *input_data;
  char *input_data_ptr;
  size_t input_data_len;
  struct mp_edge_object *edges ;
  lua_State *LL;
} _MPLIB_INSTANCE_DATA;

typedef struct _MPLIB_INSTANCE_DATA mplib_instance;

static mplib_instance *mplib_get_data (MP mp) {
  return (mplib_instance *)mp->userdata;
}

static mplib_instance *mplib_make_data (void) {
  mplib_instance *mplib_data = malloc(sizeof(mplib_instance));
  memset(mplib_data,0,sizeof(mplib_instance));
  return mplib_data ;
}


/* Start by defining all the callback routines for the library 
 * except |run_make_mpx| and |run_editor|.
 */


char *mplib_find_file (MP mp, char *fname, char *fmode, int ftype)  {
  mplib_instance *mplib_data = mplib_get_data(mp);
  lua_State *L = mplib_data->LL;
  lua_checkstack(L,4);
  lua_getfield(L,LUA_REGISTRYINDEX,"mplib_file_finder");
  if (lua_isfunction(L,-1)) {
    char *s = NULL, *x = NULL;
    lua_pushstring(L, fname);
    lua_pushstring(L, fmode);
    if (ftype >= mp_filetype_text) {
      lua_pushnumber(L, ftype-mp_filetype_text);
    } else {
      lua_pushstring(L, mplib_filetype_names[ftype]);
    }
    if(lua_pcall(L,3,1,0) != 0) {
      fprintf(stdout,"Error in mp.find_file: %s\n", (char *)lua_tostring(L,-1));
      return NULL;
    }
    x = (char *)lua_tostring(L,-1);
    if (x!=NULL)
      s = strdup(x);
      lua_pop(L,1); /* pop the string */
      return s;
  } else {
    lua_pop(L,1);
  }
  if (fmode[0] != 'r' || (! access (fname,R_OK)) || ftype) {  
     return strdup(fname);
  }
  return NULL;
}

static int 
mplib_find_file_function (lua_State *L) {
  if (! (lua_isfunction(L,-1)|| lua_isnil(L,-1) )) {
    return 1; /* error */
  }
  lua_pushstring(L, "mplib_file_finder");
  lua_pushvalue(L,-2);
  lua_rawset(L,LUA_REGISTRYINDEX);
  return 0;
}

void *mplib_open_file(MP mp, char *fname, char *fmode, int ftype)  {
  mplib_instance *mplib_data = mplib_get_data(mp);
  File *ff = malloc(sizeof (File));
  if (ff) {
    ff->f = NULL;
    if (ftype==mp_filetype_terminal) {
      if (fmode[0] == 'r') {
	ff->f = stdin;
      } else {
	xfree(mplib_data->term_file_ptr); 
	ff->f = malloc(1);
	mplib_data->term_file_ptr = ff->f;
      }
    } else if (ftype==mp_filetype_error) {
      xfree(mplib_data->err_file_ptr); 
      ff->f = malloc(1);
      mplib_data->err_file_ptr = ff->f;
    } else if (ftype == mp_filetype_log) {
      xfree(mplib_data->log_file_ptr); 
      ff->f = malloc(1);
      mplib_data->log_file_ptr = ff->f;
    } else if (ftype == mp_filetype_postscript) {
      xfree(mplib_data->ps_file_ptr); 
      ff->f = malloc(1);
      mplib_data->ps_file_ptr = ff->f;
    } else { 
      char *f = fname;
      if (fmode[0] == 'r') {
	f = mplib_find_file(mp, fname,fmode,ftype);
	if (f==NULL)
	  return NULL;
      }
      ff->f = fopen(f, fmode);
      if ((fmode[0] == 'r') && (ff->f == NULL)) {
	free(ff);
	return NULL;  
      }
    }
    return ff;
  }
  return NULL;
}

static int 
mplib_get_char (void *f, mplib_instance *mplib_data) {
  int c;
  if (f==stdin && mplib_data->input_data != NULL) {
	if (mplib_data->input_data_len==0) {
	  if (mplib_data->input_data_ptr!=NULL)
		mplib_data->input_data_ptr = NULL;
	  else
		mplib_data->input_data = NULL;
	  c = EOF;
	} else {
	  mplib_data->input_data_len--;
	  c = *(mplib_data->input_data_ptr)++;
      }
  } else {
	c = fgetc(f);
  }
  return c;
}

static void
mplib_unget_char (void *f, mplib_instance *mplib_data, int c) {
  if (f==stdin && mplib_data->input_data_ptr != NULL) {
	mplib_data->input_data_len++;	
	mplib_data->input_data_ptr--;
  } else {
	ungetc(c,f);
  }
}


char *mplib_read_ascii_file (MP mp, void *ff, size_t *size) {
  int c;
  size_t len = 0, lim = 128;
  char *s = NULL;
  mplib_instance *mplib_data = mplib_get_data(mp);
  if (ff!=NULL) {
    FILE *f = ((File *)ff)->f;
    if (f==NULL)
      return NULL;
    *size = 0;
    c = mplib_get_char(f,mplib_data);
    if (c==EOF)
      return NULL;
    s = malloc(lim); 
    if (s==NULL) return NULL;
    while (c!=EOF && c!='\n' && c!='\r') { 
      if (len==lim) {
	s =realloc(s, (lim+(lim>>2)));
	if (s==NULL) return NULL;
	lim+=(lim>>2);
      }
      s[len++] = c;
      c = mplib_get_char(f,mplib_data);
    }
    if (c=='\r') {
      c = mplib_get_char(f,mplib_data);
      if (c!=EOF && c!='\n')
	mplib_unget_char(f,mplib_data,c);
    }
    s[len] = 0;
    *size = len;
  }
  return s;
}

#define APPEND_STRING(a,b) do {			\
    if (a==NULL) {				\
      a = strdup(b);				\
    } else {					\
      a = realloc(a, strlen(a)+strlen(b)+1);	\
      strcpy(a+strlen(a),b);			\
    }						\
  } while (0)

void mplib_write_ascii_file (MP mp, void *ff, char *s) {
  mplib_instance *mplib_data = mplib_get_data(mp);
  if (ff!=NULL) {
    void *f = ((File *)ff)->f;
    if (f!=NULL) {
      if (f==mplib_data->term_file_ptr) {
	APPEND_STRING(mplib_data->term_out,s);
      } else if (f==mplib_data->err_file_ptr) {
	APPEND_STRING(mplib_data->error_out,s);
      } else if (f==mplib_data->log_file_ptr) {
	APPEND_STRING(mplib_data->log_out,s);
      } else if (f==mplib_data->ps_file_ptr) {
        APPEND_STRING(mplib_data->ps_out,s);
      } else {
	fprintf((FILE *)f,s);
      }
    }
  }
}

void mplib_read_binary_file (MP mp, void *ff, void **data, size_t *size) {
  size_t len = 0;
  if (ff!=NULL) {
    FILE *f = ((File *)ff)->f;
    if (f!=NULL) 
      len = fread(*data,1,*size,f);
    *size = len;
  }
}

void mplib_write_binary_file (MP mp, void *ff, void *s, size_t size) {
  if (ff!=NULL) {
    FILE *f = ((File *)ff)->f;
    if (f!=NULL)
      fwrite(s,size,1,f);
  }
}


void mplib_close_file (MP mp, void *ff) {
  mplib_instance *mplib_data = mplib_get_data(mp);
  if (ff!=NULL) {
    void *f = ((File *)ff)->f;
    if (f != NULL && f != mplib_data->term_file_ptr && f != mplib_data->err_file_ptr
	&& f != mplib_data->log_file_ptr && f != mplib_data->ps_file_ptr) {
      fclose(f);
    }
    free(ff);
  }
}

int mplib_eof_file (MP mp, void *ff) {
  mplib_instance *mplib_data = mplib_get_data(mp);
  if (ff!=NULL) {
    FILE *f = ((File *)ff)->f;
    if (f==NULL)
      return 1;
    if (f==stdin && mplib_data->input_data != NULL) {	
      return (mplib_data->input_data_len==0);
    }
    return feof(f);
  }
  return 1;
}

void mplib_flush_file (MP mp, void *ff) {
  return ;
}

#define APPEND_TO_EDGES(a) do {			\
    if (mplib_data->edges==NULL) {				\
      mplib_data->edges = hh;				\
    } else {					\
      struct mp_edge_object *p = mplib_data->edges;		\
      while (p->_next!=NULL) { p = p->_next; }	\
      p->_next = hh;				\
    }						\
} while (0)

void mplib_shipout_backend (MP mp, int h) {
  struct mp_edge_object *hh; 
  mplib_instance *mplib_data = mplib_get_data(mp);
  hh = mp_gr_export(mp, h);
  if (hh) {
    APPEND_TO_EDGES(hh); 
  }
}


static void 
mplib_setup_file_ops(struct MP_options * options) {
  options->find_file         = mplib_find_file;
  options->open_file         = mplib_open_file;
  options->close_file        = mplib_close_file;
  options->eof_file          = mplib_eof_file;
  options->flush_file        = mplib_flush_file;
  options->write_ascii_file  = mplib_write_ascii_file;
  options->read_ascii_file   = mplib_read_ascii_file;
  options->write_binary_file = mplib_write_binary_file;
  options->read_binary_file  = mplib_read_binary_file;
  options->shipout_backend   = mplib_shipout_backend;
}

static int 
mplib_new (lua_State *L) {
  MP *mp_ptr;
  int i;
  mplib_instance *mplib_data;
  struct MP_options * options; /* instance options */
  mp_ptr = lua_newuserdata(L, sizeof(MP *));
  if (mp_ptr) {
    options = mp_options();
    mplib_setup_file_ops(options);
    mplib_data = mplib_make_data();
    mplib_data->LL = L;
    options->userdata = (void *)mplib_data;
    options->noninteractive = 1; /* required ! */
    options->print_found_names = 0;
    if (lua_type(L,1)==LUA_TTABLE) {
      for (i=0;mplib_parms[i].name!=NULL;i++) {
	lua_getfield(L,1,mplib_parms[i].name);
	if (lua_isnil(L,-1)) {
          lua_pop(L,1);
	  continue; /* skip unset */
	}
        switch(mplib_parms[i].idx) {
	case P_ERROR_LINE: 
	  options->error_line = lua_tointeger(L,-1);
          break;
	case P_HALF_LINE:   
	  options->half_error_line = lua_tointeger(L,-1);
          break;
	case P_MAX_LINE:
	  options->max_print_line = lua_tointeger(L,-1);
          break;
	case P_MAIN_MEMORY:
	  options->main_memory = lua_tointeger(L,-1);
          break;
	case P_HASH_SIZE:
	  options->hash_size = lua_tointeger(L,-1);
          break;
	case P_HASH_PRIME:
	  options->hash_prime = lua_tointeger(L,-1);
          break;
	case P_PARAM_SIZE:
	  options->param_size = lua_tointeger(L,-1);
          break;
	case P_IN_OPEN:
	  options->max_in_open = lua_tointeger(L,-1);
          break;
	case P_RANDOM_SEED:
	  options->random_seed = lua_tointeger(L,-1);
          break;
	case P_INTERACTION:
          options->interaction = luaL_checkoption(L,-1,"errorstopmode", interaction_options);
	  break;
	case P_INI_VERSION:
	  options->ini_version = lua_toboolean(L,-1);
          break;
	case P_TROFF_MODE:
	  options->troff_mode = lua_toboolean(L,-1);
          break;
	case P_PRINT_NAMES:
	  options->print_found_names = lua_toboolean(L,-1);
          break;
	  /*	  
	case P_COMMAND_LINE:
	  options->command_line = strdup((char *)lua_tostring(L,-1));
          break;
	  */
	case P_MEM_NAME:
	  options->mem_name = strdup((char *)lua_tostring(L,-1));
          break;
	case P_JOB_NAME:
	  options->job_name = strdup((char *)lua_tostring(L,-1));
          break;
	case P_FIND_FILE:  
	  if(mplib_find_file_function(L)) { /* error here */
	    fprintf(stdout,"Invalid arguments to mp.new({find_file=...})\n");
	  }
	  break;
        default:
	  break;
	}
        lua_pop(L,1);
      }
    }
    *mp_ptr = mp_new(options);
    xfree(options->command_line);
    xfree(options->mem_name);
    xfree(options->job_name);
    free(options);
    if (*mp_ptr) {
      luaL_getmetatable(L,MPLIB_METATABLE);
      lua_setmetatable(L,-2);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int
mplib_collect (lua_State *L) {
  MP *mp_ptr = is_mp(L,1);
  xfree(*mp_ptr);
  return 0;
}

static int
mplib_tostring (lua_State *L) {
  MP *mp_ptr = is_mp(L,1);
  if (*mp_ptr!=NULL) {
    lua_pushfstring(L,"<MP %p>",*mp_ptr);
     return 1;
  }
  return 0;
}

static int 
mplib_wrapresults(lua_State *L, mplib_instance *mplib_data, int h) {
   lua_checkstack(L,5);
   lua_newtable(L);
   if (mplib_data->term_out != NULL) {
     lua_pushstring(L,mplib_data->term_out);
     lua_setfield(L,-2,"term");
     xfree(mplib_data->term_out);
   }
   if (mplib_data->error_out != NULL) {
     lua_pushstring(L,mplib_data->error_out);
     lua_setfield(L,-2,"error");
     xfree(mplib_data->error_out);
   } 
   if (mplib_data->log_out != NULL ) {
     lua_pushstring(L,mplib_data->log_out);
     lua_setfield(L,-2,"log");
     xfree(mplib_data->log_out);
   }
   if (mplib_data->edges != NULL ) {
     struct mp_edge_object **v;
     struct mp_edge_object *p = mplib_data->edges;
     int i = 1;
     lua_newtable(L);
     while (p!=NULL) { 
       v = lua_newuserdata (L, sizeof(struct mp_edge_object *));
       *v = p;
       luaL_getmetatable(L,MPLIB_FIG_METATABLE);
       lua_setmetatable(L,-2);
       lua_rawseti(L,-2,i); i++;
       p = p->_next;
     }
     lua_setfield(L,-2,"fig");
     mplib_data->edges = NULL;
   }
   lua_pushnumber(L,h);
   lua_setfield(L,-2,"status");
   return 1;
}

static int
mplib_execute (lua_State *L) {
  int h;
  MP *mp_ptr = is_mp(L,1);
  if (*mp_ptr!=NULL && lua_isstring(L,2)) {
    mplib_instance *mplib_data = mplib_get_data(*mp_ptr);
    mplib_data->input_data = (char *)lua_tolstring(L,2, &(mplib_data->input_data_len));
    mplib_data->input_data_ptr = mplib_data->input_data;
    if ((*mp_ptr)->run_state==0) {
      h = mp_initialize(*mp_ptr);
    }
    h = mp_execute(*mp_ptr);
    if (mplib_data->input_data_len!=0) {
      xfree(mplib_data->input_data);
      xfree(mplib_data->input_data_ptr);
      mplib_data->input_data_len=0;
    }
    return mplib_wrapresults(L, mplib_data, h);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int
mplib_finish (lua_State *L) {
  MP *mp_ptr = is_mp(L,1);
  if (*mp_ptr!=NULL) {
    mplib_instance *mplib_data = mplib_get_data(*mp_ptr);
    int h = mp_finish(*mp_ptr);
    return mplib_wrapresults(L, mplib_data, h);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

/* figure methods */

static int
mplib_fig_collect (lua_State *L) {
  struct mp_edge_object **hh = is_fig(L,1);
  if (*hh!=NULL) {
    mp_gr_toss_objects (*hh);
    *hh=NULL;
  }
  return 0;
}

static int
mplib_fig_body (lua_State *L) {
  int i = 1;
  struct mp_graphic_object **v;
  struct mp_graphic_object *p;
  struct mp_edge_object **hh = is_fig(L,1);
  lua_newtable(L);
  p = (*hh)->body;
  while (p!=NULL) {
    v = lua_newuserdata (L, sizeof(struct mp_graphic_object *));
    *v = p;
    luaL_getmetatable(L,MPLIB_GR_METATABLE);
    lua_setmetatable(L,-2);
    lua_rawseti(L,-2,i); i++;
    p = p->_link_field;
  }
  (*hh)->body = NULL; /* prevent double free */
  return 1;
}


static int
mplib_fig_tostring (lua_State *L) {
  struct mp_edge_object **hh = is_fig(L,1);
  lua_pushfstring(L,"<figure %p>",*hh);
  return 1;
}



static int 
mp_wrapped_shipout (struct mp_edge_object *hh, int prologues, int procset) {
  MP mp = hh->_parent;
  if (setjmp(mp->jump_buf)) {
    return 0;
  }
  mp_gr_ship_out(hh,prologues,procset);
  return 1;
}

static int
mplib_fig_postscript (lua_State *L) {
  struct mp_edge_object **hh = is_fig(L,1);
  int prologues = luaL_optnumber(L,2,-1);
  int procset = luaL_optnumber(L,3,-1);
  mplib_instance *mplib_data = mplib_get_data((*hh)->_parent);
  if (mplib_data->ps_out == NULL) {
    if (mp_wrapped_shipout(*hh,prologues, procset)) {
      if (mplib_data->ps_out!=NULL ) {
	lua_pushstring(L, mplib_data->ps_out);
	xfree(mplib_data->ps_out);
      } else {
	lua_pushnil(L);
      }
      return 1;
    } else {
      lua_pushnil(L);
      lua_pushstring(L,mplib_data->log_out);
      xfree(mplib_data->ps_out); 
      return 2;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int
mplib_fig_filename (lua_State *L) {
  struct mp_edge_object **hh = is_fig(L,1);
  if (*hh!= NULL) { 
	char *s = (*hh)->_filename;
	lua_pushstring(L,s);
  } else {
	lua_pushnil(L);
  }
  return 1;
}


static int
mplib_fig_bb (lua_State *L) {
  struct mp_edge_object **hh = is_fig(L,1);
  lua_newtable(L);
  lua_pushnumber(L, (double)(*hh)->_minx/65536.0);
  lua_rawseti(L,-2,1);
  lua_pushnumber(L, (double)(*hh)->_miny/65536.0);
  lua_rawseti(L,-2,2);
  lua_pushnumber(L, (double)(*hh)->_maxx/65536.0);
  lua_rawseti(L,-2,3);
  lua_pushnumber(L, (double)(*hh)->_maxy/65536.0);
  lua_rawseti(L,-2,4);
  return 1;
}

/* object methods */

static int
mplib_gr_collect (lua_State *L) {
  struct mp_graphic_object **hh = is_gr_object(L,1);
  if (*hh!=NULL) {
    mp_gr_toss_object(*hh);
    *hh=NULL;
  }
  return 0;
}

static int
mplib_gr_tostring (lua_State *L) {
  struct mp_graphic_object **hh = is_gr_object(L,1);
  lua_pushfstring(L,"<object %p>",*hh);
  return 1;
}

static void 
mplib_push_number (lua_State *L, integer x ) {
  lua_Number y;
  y = x / 65536.0;
  lua_pushnumber(L, y);
}

static void 
mplib_push_path (lua_State *L, struct mp_knot *h ) {
  struct mp_knot *p; /* for scanning the path */
  int i=1;
  p=h;
  if (p!=NULL) {
    lua_newtable(L);
    do {  
      lua_newtable(L);
      lua_pushstring(L,knot_originator_enum[p->originator_field]);
      lua_setfield(L,-2,"originator");
      lua_pushstring(L,knot_type_enum[p->left_type_field]);
      lua_setfield(L,-2,"left_type");
      lua_pushstring(L,knot_type_enum[p->right_type_field]);
      lua_setfield(L,-2,"right_type");
      mplib_push_number(L,p->x_coord_field);
      lua_setfield(L,-2,"x_coord");
      mplib_push_number(L,p->y_coord_field);
      lua_setfield(L,-2,"y_coord");
      mplib_push_number(L,p->left_x_field);
      lua_setfield(L,-2,"left_x");
      mplib_push_number(L,p->left_y_field);
      lua_setfield(L,-2,"left_y");
      mplib_push_number(L,p->right_x_field);
      lua_setfield(L,-2,"right_x");
      mplib_push_number(L,p->right_y_field);
      lua_setfield(L,-2,"right_y");
      lua_rawseti(L,-2,i); i++;
      if ( p->right_type_field==mp_endpoint ) { 
	return;
      }
      p=p->next_field;
    } while (p!=h) ;
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_push_color (lua_State *L, struct mp_graphic_object *h ) {
  if (h!=NULL) {
    lua_newtable(L);
    lua_pushstring(L,color_model_enum[h->color_model_field]);
    lua_setfield(L,-2,"model");

    if (h->color_model_field == mp_rgb_model ||
	h->color_model_field == mp_uninitialized_model) {
      lua_newtable(L);
      mplib_push_number(L,h->color_field.rgb._red_val);
      lua_rawseti(L,-2,1);
      mplib_push_number(L,h->color_field.rgb._green_val);
      lua_rawseti(L,-2,2);
      mplib_push_number(L,h->color_field.rgb._blue_val);
      lua_rawseti(L,-2,3);
      lua_setfield(L,-2,"rgb");
    }

    if (h->color_model_field == mp_cmyk_model ||
	h->color_model_field == mp_uninitialized_model) {
      lua_newtable(L);
      mplib_push_number(L,h->color_field.cmyk._cyan_val);
      lua_rawseti(L,-2,1);
      mplib_push_number(L,h->color_field.cmyk._magenta_val);
      lua_rawseti(L,-2,2);
      mplib_push_number(L,h->color_field.cmyk._yellow_val);
      lua_rawseti(L,-2,3);
      mplib_push_number(L,h->color_field.cmyk._black_val);
      lua_rawseti(L,-2,4);
      lua_setfield(L,-2,"cmyk");
    }
    if (h->color_model_field == mp_grey_model ||
	h->color_model_field == mp_uninitialized_model) {
      lua_newtable(L);
      mplib_push_number(L,h->color_field.grey._grey_val);
      lua_rawseti(L,-2,1);
      lua_setfield(L,-2,"grey");
    }
    
  } else {
    lua_pushnil(L);
  }
}

/* dashes are complicated, perhaps it would be better if the
  object had a PS-compatible representation */
static void 
mplib_push_dash (lua_State *L, struct mp_graphic_object *h ) {
  if (h!=NULL) {
    lua_newtable(L);
    mplib_push_number(L,h->dash_scale_field);
    lua_setfield(L,-2,"scale");
    /* todo */
    lua_pushnumber(L,(int)h->dash_p_field);
    lua_setfield(L,-2,"dashes");
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_push_transform (lua_State *L, struct mp_graphic_object *h ) {
  int i = 1;
  if (h!=NULL) {
    lua_newtable(L);
    mplib_push_number(L,h->tx_field);
    lua_rawseti(L,-2,i); i++;
    mplib_push_number(L,h->ty_field);
    lua_rawseti(L,-2,i); i++;
    mplib_push_number(L,h->txx_field);
    lua_rawseti(L,-2,i); i++;
    mplib_push_number(L,h->txy_field);
    lua_rawseti(L,-2,i); i++;
    mplib_push_number(L,h->tyx_field);
    lua_rawseti(L,-2,i); i++;
    mplib_push_number(L,h->tyy_field);
    lua_rawseti(L,-2,i); i++;
  } else {
    lua_pushnil(L);
  }
}


static void 
mplib_fill_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (FIELD(type)) {
    lua_pushstring(L,"fill");
  } else if (FIELD(path)) {
    mplib_push_path(L, h->path_p_field);
  } else if (FIELD(htap)) {
    mplib_push_path(L, h->htap_p_field);
  } else if (FIELD(pen)) {
    mplib_push_path(L, h->pen_p_field);
  } else if (FIELD(color)) {
    mplib_push_color(L, h);
  } else if (FIELD(linejoin)) {
    lua_pushnumber(L,h->ljoin_field);
  } else if (FIELD(miterlimit)) {
    mplib_push_number(L,h->miterlim_field);
  } else if (FIELD(prescript)) {
    lua_pushstring(L,h->pre_script_field);
  } else if (FIELD(postscript)) {
    lua_pushstring(L,h->post_script_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_stroked_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (FIELD(type)) {
    lua_pushstring(L,"outline");
  } else if (FIELD(path)) {
    mplib_push_path(L, h->path_p_field);
  } else if (FIELD(pen)) {
    mplib_push_path(L, h->pen_p_field);
  } else if (FIELD(color)) {
    mplib_push_color(L, h);
  } else if (FIELD(dash)) {
    mplib_push_dash(L, h);
  } else if (FIELD(linecap)) {
    lua_pushnumber(L,h->lcap_field);
  } else if (FIELD(linejoin)) {
    lua_pushnumber(L,h->ljoin_field);
  } else if (FIELD(miterlimit)) {
    mplib_push_number(L,h->miterlim_field);
  } else if (FIELD(prescript)) {
    lua_pushstring(L,h->pre_script_field);
  } else if (FIELD(postscript)) {
    lua_pushstring(L,h->post_script_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_text_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  
  if (FIELD(type)) {
    lua_pushstring(L,"text");
  } else if (FIELD(text)) {
    lua_pushstring(L,h->text_p_field);
  } else if (FIELD(dsize)) {
    lua_pushnumber(L,h->font_dsize_field);
  } else if (FIELD(font)) {
    lua_pushstring(L,h->font_name_field);
  } else if (FIELD(color)) {
    mplib_push_color(L, h);
  } else if (FIELD(width)) {
    mplib_push_number(L,h->width_field);
  } else if (FIELD(height)) {
    mplib_push_number(L,h->height_field);
  } else if (FIELD(depth)) {
    mplib_push_number(L,h->depth_field);
  } else if (FIELD(transform)) {
    mplib_push_transform(L,h);
  } else if (FIELD(prescript)) {
    lua_pushstring(L,h->pre_script_field);
  } else if (FIELD(postscript)) {
    lua_pushstring(L,h->post_script_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_special_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (FIELD(type)) {
    lua_pushstring(L,"special");
  } else if (FIELD(prescript)) {
    lua_pushstring(L,h->pre_script_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_start_bounds_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (FIELD(type)) {
    lua_pushstring(L,"start_bounds");
  } else if (FIELD(path)) {
    mplib_push_path(L,h->path_p_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_start_clip_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (FIELD(type)) {
    lua_pushstring(L,"start_clip");
  } else if (FIELD(path)) {
    mplib_push_path(L,h->path_p_field);
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_stop_bounds_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (h!=NULL && FIELD(type)) {
    lua_pushstring(L,"stop_bounds");
  } else {
    lua_pushnil(L);
  }
}

static void 
mplib_stop_clip_field (lua_State *L, struct mp_graphic_object *h, char *field) {
  if (h!=NULL && FIELD(type)) {
    lua_pushstring(L,"stop_clip");
  } else {
    lua_pushnil(L);
  }
}

static int
mplib_gr_fields (lua_State *L) {
  const char **fields;
  const char *f;
  int i = 1;
  struct mp_graphic_object **hh = is_gr_object(L,1);
  if (*hh) {
    switch ((*hh)->_type_field) {
    case mp_fill_code:         fields = fill_fields;         break;
    case mp_stroked_code:      fields = stroked_fields;      break;
    case mp_text_code:         fields = text_fields;         break;
    case mp_special_code:      fields = special_fields;      break;
    case mp_start_clip_code:   fields = start_clip_fields;   break;
    case mp_start_bounds_code: fields = start_bounds_fields; break;
    case mp_stop_clip_code:    fields = stop_clip_fields;    break;
    case mp_stop_bounds_code:  fields = stop_bounds_fields;  break;
    default:                   fields = no_fields;
    }
    lua_newtable(L);
    for (f = *fields; f != NULL; f++) {
      lua_pushstring(L,f);
      lua_rawseti(L,-2,i); i++;
    }
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int
mplib_gr_index (lua_State *L) {
  struct mp_graphic_object **hh = is_gr_object(L,1);
  char *field = (char *)luaL_checkstring(L,2);
  if (*hh) {
    switch ((*hh)->_type_field) {
    case mp_fill_code:         mplib_fill_field(L,*hh,field);         break;
    case mp_stroked_code:      mplib_stroked_field(L,*hh,field);      break;
    case mp_text_code:         mplib_text_field(L,*hh,field);         break;
    case mp_special_code:      mplib_special_field(L,*hh,field);      break;
    case mp_start_clip_code:   mplib_start_clip_field(L,*hh,field);   break;
    case mp_start_bounds_code: mplib_start_bounds_field(L,*hh,field); break;
    case mp_stop_clip_code:    mplib_stop_clip_field(L,*hh,field);    break;
    case mp_stop_bounds_code:  mplib_stop_bounds_field(L,*hh,field);  break;
    default:                   lua_pushnil(L);
    }    
  } else {
    lua_pushnil(L);
  }
  return 1;
}


static const struct luaL_reg mplib_meta[] = {
  {"__gc",               mplib_collect}, 
  {"__tostring",         mplib_tostring},
  {NULL, NULL}                /* sentinel */
};

static const struct luaL_reg mplib_fig_meta[] = {
  {"__gc",               mplib_fig_collect    },
  {"__tostring",         mplib_fig_tostring   },
  {"objects",            mplib_fig_body       },
  {"filename",           mplib_fig_filename   },
  {"postscript",         mplib_fig_postscript },
  {"boundingbox",        mplib_fig_bb         },
  {NULL, NULL}                /* sentinel */
};

static const struct luaL_reg mplib_gr_meta[] = {
  {"__gc",               mplib_gr_collect  },
  {"__tostring",         mplib_gr_tostring },
  {"__index",            mplib_gr_index    },
  {"fields",             mplib_gr_fields   },
  {NULL, NULL}                /* sentinel */
};


static const struct luaL_reg mplib_d [] = {
  {"execute",            mplib_execute },
  {"finish",             mplib_finish },
  {NULL, NULL}  /* sentinel */
};


static const struct luaL_reg mplib_m[] = {
  {"new",                 mplib_new},
  {NULL, NULL}                /* sentinel */
};


int 
luaopen_mp (lua_State *L) {
  luaL_newmetatable(L,MPLIB_GR_METATABLE);
  lua_pushvalue(L, -1); /* push metatable */
  lua_setfield(L, -2, "__index"); /* metatable.__index = metatable */
  luaL_register(L, NULL, mplib_gr_meta);  /* object meta methods */
  lua_pop(L,1);

  luaL_newmetatable(L,MPLIB_FIG_METATABLE);
  lua_pushvalue(L, -1); /* push metatable */
  lua_setfield(L, -2, "__index"); /* metatable.__index = metatable */
  luaL_register(L, NULL, mplib_fig_meta);  /* figure meta methods */
  lua_pop(L,1);

  luaL_newmetatable(L,MPLIB_METATABLE);
  lua_pushvalue(L, -1); /* push metatable */
  lua_setfield(L, -2, "__index"); /* metatable.__index = metatable */
  luaL_register(L, NULL, mplib_meta);  /* meta methods */
  luaL_register(L, NULL, mplib_d);  /* dict methods */
  luaL_register(L, "mplib", mplib_m); /* module functions */
  return 1;
}

