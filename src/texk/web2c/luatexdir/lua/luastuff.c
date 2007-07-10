/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"
#include <ptexlib.h>

lua_State *Luas[65536];

extern char *startup_filename;
extern int safer_option;

int luastate_max = 0;
int luastate_bytes = 0;

void
make_table (lua_State *L, char *tab, char *getfunc, char *setfunc) {
  /* make the table */            /* [{<tex>}] */
  lua_pushstring(L,tab);          /* [{<tex>},"dimen"] */
  lua_newtable(L);                /* [{<tex>},"dimen",{}] */
  lua_settable(L, -3);            /* [{<tex>}] */
  /* fetch it back */
  lua_pushstring(L,tab);          /* [{<tex>},"dimen"] */
  lua_gettable(L, -2);            /* [{<tex>},{<dimen>}] */
  /* make the meta entries */
  luaL_newmetatable(L,tab);       /* [{<tex>},{<dimen>},{<dimen_m>}] */
  lua_pushstring(L, "__index");   /* [{<tex>},{<dimen>},{<dimen_m>},"__index"] */
  lua_pushstring(L, getfunc);     /* [{<tex>},{<dimen>},{<dimen_m>},"__index","getdimen"] */
  lua_gettable(L, -5);            /* [{<tex>},{<dimen>},{<dimen_m>},"__index",<tex.getdimen>]  */
  lua_settable(L, -3);            /* [{<tex>},{<dimen>},{<dimen_m>}]  */
  lua_pushstring(L, "__newindex");/* [{<tex>},{<dimen>},{<dimen_m>},"__newindex"] */
  lua_pushstring(L, setfunc);     /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex","setdimen"] */
  lua_gettable(L, -5);            /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex",<tex.setdimen>]  */
  lua_settable(L, -3);            /* [{<tex>},{<dimen>},{<dimen_m>}]  */ 
  lua_setmetatable(L,-2);         /* [{<tex>},{<dimen>}] : assign the metatable */
  lua_pop(L,1);                   /* [{<tex>}] : clean the stack */
}

static 
const char *getS(lua_State * L, void *ud, size_t * size) {
    LoadS *ls = (LoadS *) ud;
    (void) L;
    if (ls->size == 0)
        return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}


#ifdef WIN32
#include <process.h>
#define exec_command(a,b,c) execvpe(a,(const char* const*)b,(const char* const*)c)
#else
#include <unistd.h>
#define DEFAULT_PATH 	"/bin:/usr/bin:."

int exec_command(const char *file, char *const *argv, char *const *envp)
{
	char path[PATH_MAX];
	const char *searchpath, *esp;
	size_t prefixlen, filelen, totallen;

	if (strchr(file, '/'))	/* Specific path */
		return execve(file, argv, envp);

	filelen = strlen(file);

	searchpath = getenv("PATH");
	if (!searchpath)
		searchpath = DEFAULT_PATH;

	errno = ENOENT;		/* Default errno, if execve() doesn't
				   change it */

	do {
		esp = strchr(searchpath, ':');
		if (esp)
			prefixlen = esp - searchpath;
		else
			prefixlen = strlen(searchpath);

		if (prefixlen == 0 || searchpath[prefixlen - 1] == '/') {
			totallen = prefixlen + filelen;
			if (totallen >= PATH_MAX)
				continue;
			memcpy(path, searchpath, prefixlen);
			memcpy(path + prefixlen, file, filelen);
		} else {
			totallen = prefixlen + filelen + 1;
			if (totallen >= PATH_MAX)
				continue;
			memcpy(path, searchpath, prefixlen);
			path[prefixlen] = '/';
			memcpy(path + prefixlen + 1, file, filelen);
		}
		path[totallen] = '\0';

		execve(path, argv, envp);
		if (errno == E2BIG  || errno == ENOEXEC ||
		    errno == ENOMEM || errno == ETXTBSY)
			break;	/* Report this as an error, no more search */

		searchpath = esp + 1;
	} while (esp);

	return -1;
}
#endif

extern char **environ;


#define COMMAND_PARTS 20

static int
do_split_command(char *maincmd, char **cmdline, char target)
{
    char *piece;
    char *cmd;
    unsigned int i;
    int ret = 0;
    int in_string = 0;
    if (strlen(maincmd) == 0)
	return 0;
    cmd = strdup(maincmd);
    i = 0;
    while (cmd[i] == ' ')
	i++;
    piece = cmd;
    for (; i <= strlen(maincmd); i++) {
	if (in_string == 1) {
	    if (cmd[i] == '"') {
		in_string = 0;
	    }
	} else if (in_string == 2) {
	    if (cmd[i] == '\'') {
		  in_string = 0;
	    }
	} else {
	    if (cmd[i] == '"') {
		in_string = 1;
	    } else if (cmd[i] == '\'') {
		in_string = 2;
	    } else if (cmd[i] == target) {
		cmd[i] = 0;
		if (ret == COMMAND_PARTS) {
		  fprintf(stderr,"os.exec(): Executable command too complex.\n");
		}
		cmdline[ret++] = strdup(piece);
		while (i < strlen(maincmd) && cmd[(i + 1)] == ' ')
		    i++;
		piece = cmd + i + 1;
	    }
	}
    }
    if (*piece) {
	  if (ret == COMMAND_PARTS) {
	    fprintf(stderr, "os.exec(): Executable command too complex.\n");
	  }
	  cmdline[ret++] = strdup(piece);
    }
    return ret;
}

static int os_exec (lua_State *L) {
  char * maincmd;
  char ** cmdline;
  int i;
  maincmd =  (char *)luaL_optstring(L, 1, NULL);
  if (maincmd) {
	cmdline = malloc(sizeof(char *) * COMMAND_PARTS);
	for (i = 0; i < COMMAND_PARTS; i++) {
	  cmdline[i] = NULL;
	}
	i = do_split_command(maincmd, cmdline,' ');
	if (i) {
	  exec_command(cmdline[0], cmdline, environ);
	}
  }
  return 0;
}

void make_exec (lua_State *L){
  lua_getglobal(L,"os");
  lua_pushcfunction(L, os_exec);
  lua_setfield(L,-2,"exec");
} 

/*  Hans wants to set env values */

static int os_setenv (lua_State *L) {
  char *value, *key, *val;
  key =  (char *)luaL_optstring(L, 1, NULL);
  val =  (char *)luaL_optstring(L, 2, NULL);
  if (key) {
	if (val) {
	  value = xmalloc(strlen(key)+strlen(val)+2);
	  sprintf(value,"%s=%s",key,val);
	  if (putenv(value)) {
		return luaL_error(L, "unable to change environment");
	  }
	} else {
#if defined(WIN32) || defined(__sun__)
	  value = xmalloc(strlen(key)+2);
	  sprintf(value,"%s=",key);
	  if (putenv(value)) {
		return luaL_error(L, "unable to change environment");
	  } 
#else
	  (void)unsetenv(key);
#endif 
	}
  }
  lua_pushboolean (L, 1);
  return 1;
}


void put_env (lua_State *L){
  lua_getglobal(L,"os");
  lua_pushcfunction(L, os_setenv);
  lua_setfield(L,-2,"setenv");
} 

void find_env (lua_State *L){
  char *envitem, *envitem_orig;
  char *envkey;
  char **envpointer;
  envpointer = environ;
  lua_getglobal(L,"os");
  if (envpointer!=NULL && lua_istable(L,-1)) {
    luaL_checkstack(L,2,"out of stack space");
    lua_pushstring(L,"env"); 
    lua_newtable(L); 
    while (*envpointer) {
      /* TODO: perhaps a memory leak here  */
      luaL_checkstack(L,2,"out of stack space");
      envitem = xstrdup(*envpointer);
      envitem_orig = envitem;
      envkey=envitem;
      while (*envitem != '=') {
	envitem++;
      }
      *envitem=0;
      envitem++;
      lua_pushstring(L,envkey);
      lua_pushstring(L,envitem);
      lua_rawset(L,-3);
      envpointer++;
      free(envitem_orig);
    }
    lua_rawset(L,-3);
  }
  lua_pop(L,1);
}

void *my_luaalloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  void *ret = NULL;
  if (nsize == 0)
	free(ptr);
  else
	ret = realloc(ptr, nsize);
  luastate_bytes += (nsize-osize);
  return ret;
}

static int my_luapanic (lua_State *L) {
  (void)L;  /* to avoid warnings */
  fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
                   lua_tostring(L, -1));
  return 0;
}


void 
luainterpreter (int n) {
  lua_State *L;
  L = lua_newstate(my_luaalloc, NULL);
  if (L==NULL) {
	fprintf(stderr,"Can't create a new Lua state (%d).",n);
	return;
  }
  lua_atpanic(L, &my_luapanic);

  luastate_max++;
  luaL_openlibs(L);
  find_env(L);
  if (!safer_option) {
	put_env(L);
	make_exec(L);
  }

  /*luaopen_unicode(L);*/
  lua_pushcfunction(L, luaopen_unicode);
  lua_pushstring(L, "unicode");
  lua_call(L, 1, 0);

  /*luaopen_zip(L);*/
  lua_pushcfunction(L, luaopen_zip);
  lua_pushstring(L, "zip");
  lua_call(L, 1, 0);

  /*luaopen_lpeg(L);*/
  lua_pushcfunction(L, luaopen_lpeg);
  lua_pushstring(L, "lpeg");
  lua_call(L, 1, 0);

  /*luaopen_md5(L);*/
  lua_pushcfunction(L, luaopen_md5);
  lua_pushstring(L, "md5");
  lua_call(L, 1, 0);

  /*luaopen_lfs(L);*/
  lua_pushcfunction(L, luaopen_lfs);
  lua_pushstring(L, "lfs");
  lua_call(L, 1, 0);

  /* zlib. slightly odd calling convention */
  luaopen_zlib(L);
  lua_setglobal(L,"zlib");
  luaopen_gzip(L);
  /* fontforge */
  luaopen_ff(L);

  luaopen_pdf(L);
  luaopen_tex(L);
  luaopen_token(L);
  luaopen_node(L);
  luaopen_texio(L);
  luaopen_kpse(L);
  if (n==0) {
    luaopen_callback(L);
    lua_createtable(L, 0, 0);
    lua_setglobal(L, "texconfig");
  }
  luaopen_lua(L,n,startup_filename);
  luaopen_stats(L);
  luaopen_font(L);

  if (safer_option) {
	/* disable some stuff if --safer */
	(void)hide_lua_value(L, "os","execute");
	(void)hide_lua_value(L, "os","rename");
	(void)hide_lua_value(L, "os","remove");
	(void)hide_lua_value(L, "io","popen");
	/* make io.open only read files */
	luaL_checkstack(L,2,"out of stack space");
	lua_getglobal(L,"io");
	lua_getfield(L,-1,"open_ro");	
	lua_setfield(L,-2,"open");	
	(void)hide_lua_value(L, "io","tmpfile");
	(void)hide_lua_value(L, "io","output");
	(void)hide_lua_value(L, "lfs","chdir");
	(void)hide_lua_value(L, "lfs","lock");
	(void)hide_lua_value(L, "lfs","touch");
	(void)hide_lua_value(L, "lfs","rmdir");
	(void)hide_lua_value(L, "lfs","mkdir");
  }
  Luas[n] = L;
}

int hide_lua_table(lua_State *L, char *name) {
  int r=0;
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
    r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushnil(L);
    lua_setglobal(L,name);
  }
  return r;
}

void unhide_lua_table(lua_State *L, char *name, int r) {
  lua_rawgeti(L,LUA_REGISTRYINDEX,r);
  lua_setglobal(L,name);
  luaL_unref(L,LUA_REGISTRYINDEX,r);
}

int hide_lua_value(lua_State *L, char *name, char *item) {
  int r=0;
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
	lua_getfield(L,-1,item);
    r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushnil(L);
    lua_setfield(L,-2,item);
  }
  return r;
}

void unhide_lua_value(lua_State *L, char *name, char *item, int r) {
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
	lua_rawgeti(L,LUA_REGISTRYINDEX,r);
	lua_setfield(L,-2,item);
	luaL_unref(L,LUA_REGISTRYINDEX,r);
  }
}


void 
luacall(int n, int s) {
  LoadS ls;
  int i, j, k ;
  char lua_id[12];
  if (Luas[n] == NULL) {
    luainterpreter(n);
  }
  luatex_load_init(s,&ls);
  if (ls.size>0) {
	snprintf((char *)lua_id,12,"luas[%d]",n);
	i = lua_load(Luas[n], getS, &ls, lua_id);
	if (i != 0) {
	  Luas[n] = luatex_error(Luas[n],(i == LUA_ERRSYNTAX ? 0 : 1));
	} else {
	  i = lua_pcall(Luas[n], 0, 0, 0);
	  if (i != 0) {
		Luas[n] = luatex_error(Luas[n],(i == LUA_ERRRUN ? 0 : 1));
	  }	 
	}
  }
}

void 
luatokencall(int n, int p) {
  LoadS ls;
  int i, j, k, l;
  char *s=NULL;
  char lua_id[12];
  if (Luas[n] == NULL) {
    luainterpreter(n);
  }
  l = 0;
  s = tokenlist_to_cstring(p,1,&l);
  ls.s = s;
  ls.size = l;
  if (ls.size>0) {
	snprintf((char *)lua_id,12,"luas[%d]",n);
	i = lua_load(Luas[n], getS, &ls, lua_id);
	xfree(s);
	if (i != 0) {
	  Luas[n] = luatex_error(Luas[n],(i == LUA_ERRSYNTAX ? 0 : 1));
	} else {
	  i = lua_pcall(Luas[n], 0, 0, 0);
	  if (i != 0) {
		Luas[n] = luatex_error(Luas[n],(i == LUA_ERRRUN ? 0 : 1));
	  }	 
	}
  }
}



void 
closelua(int n) {
  if (n!=0 && Luas[n] != NULL) {
    lua_close(Luas[n]);
	luastate_max--;
    Luas[n] = NULL;
  }
}


void 
luatex_load_init (int s, LoadS *ls) {
  ls->s = (const char *)&(strpool[strstart[s]]);
  ls->size = strstart[s + 1] - strstart[s];
}

lua_State *
luatex_error (lua_State * L, int is_fatal) {
  int i,j;
  size_t len;
  char *err;
  strnumber s;
  const char *luaerr = lua_tostring(L, -1);
  err = (char *)xmalloc(128);
  len = snprintf(err,128,"%s",luaerr);
  if (is_fatal>0) {
    /* Normally a memory error from lua. 
       The pool may overflow during the maketexstring(), but we 
       are crashing anyway so we may as well abort on the pool size */
    s = maketexstring(err);
    lua_fatal_error(s);
    /* never reached */
    xfree (err);
    lua_close(L);
	luastate_max--;
    return (lua_State *)NULL;
  } else {
    /* Here, the pool could be full already, but we can possibly escape from that 
     * condition, since the lua chunk that caused the error is the current string.
     */
    s = str_ptr-0x200000;
    //    fprintf(stderr,"poolinfo: %d: %d,%d out of %d\n",s,poolptr,strstart[(s-1)],poolsize);
    poolptr = strstart[(s-1)];
    strstart[s] = poolptr;
    if (poolptr+len>=poolsize) {
      lua_norm_error(' ');
    } else {
      s = maketexstring(err);
      lua_norm_error(s);
      flush_str(s);
    }
    xfree (err);
    return L;
  }
}

