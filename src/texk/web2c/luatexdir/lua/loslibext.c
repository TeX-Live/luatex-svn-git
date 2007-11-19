/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"
#include <ptexlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

/* there could be more platforms that don't have these two,
 *  but win32 and sunos are for sure.
 * gettimeofday() for win32 is using an alternative definition
 */

#if (! defined(WIN32)) && (! defined(__SUNOS__))
#include <sys/time.h>   /* gettimeofday() */
#include <sys/times.h>  /* times() */
#endif 


#ifdef WIN32
#include <process.h>
#define exec_command(a,b,c) execvpe(a,(const char* const*)b,(const char* const*)c)
#else
#include <unistd.h>
#define DEFAULT_PATH 	"/bin:/usr/bin:."

static int 
exec_command(const char *file, char *const *argv, char *const *envp)
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

	errno = ENOENT;		/* Default errno, if execve() doesn't  change it */

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

static int ex_sleep(lua_State *L)
{
  lua_Number interval = luaL_checknumber(L, 1);
  lua_Number units = luaL_optnumber(L, 2, 1);
#ifdef WIN32
  Sleep(1e3 * interval / units);
#else /* assumes posix or bsd */
  usleep(1e6 * interval / units);
#endif
  return 0;
}

#if (! defined (WIN32))  && (! defined (__SUNOS__))
static int os_times (lua_State *L) {
  struct tms r;
  (void)times(&r);
  lua_newtable(L);
  lua_pushnumber(L, ((lua_Number)(r.tms_utime))/(lua_Number)sysconf(_SC_CLK_TCK));
  lua_setfield(L,-2,"utime");
  lua_pushnumber(L, ((lua_Number)(r.tms_stime))/(lua_Number)sysconf(_SC_CLK_TCK));
  lua_setfield(L,-2,"stime");
  lua_pushnumber(L, ((lua_Number)(r.tms_cutime))/(lua_Number)sysconf(_SC_CLK_TCK));
  lua_setfield(L,-2,"cutime");
  lua_pushnumber(L, ((lua_Number)(r.tms_cstime))/(lua_Number)sysconf(_SC_CLK_TCK));
  lua_setfield(L,-2,"cstime");
  return 1;
}
#endif

#if ! defined (__SUNOS__)

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

static int os_gettimeofday (lua_State *L) {
  lua_Number v;
#ifndef WIN32
  struct timeval tv;
  gettimeofday(&tv, NULL);
  v = (lua_Number)tv.tv_sec + (lua_Number)tv.tv_usec / 1000000.0;
#else
  FILETIME ft;
  unsigned __int64 tmpres = 0;

  GetSystemTimeAsFileTime(&ft);

  tmpres |= ft.dwHighDateTime;
  tmpres <<= 32;
  tmpres |= ft.dwLowDateTime;
  tmpres /= 10;
  tmpres -= DELTA_EPOCH_IN_MICROSECS; /*converting file time to unix epoch*/
  v = (lua_Number)tmpres / 1000000.0; 
#endif
  lua_pushnumber(L, v);
  return 1;
}
#endif

static const char repl[] = "0123456789abcdefghijklmnopqrstuvwxyz";

#define MAXTRIES 36*36*36

char *
do_mkdtemp (char *tmpl)
{
  int count;
  int value ;
  char *xes = &tmpl[strlen (tmpl) - 6];
  /* this is not really all that random, but it will do */
  srand(time(NULL));
  value = rand();
  for (count = 0; count < MAXTRIES; value += 8413, ++count) {
      int v = value;
      xes[0] = repl[v % 36];  v /= 36;
      xes[1] = repl[v % 36];  v /= 36;
      xes[2] = repl[v % 36];  v /= 36;
      xes[3] = repl[v % 36];  v /= 36;
      xes[4] = repl[v % 36];  v /= 36;
      xes[5] = repl[v % 36];
      if (mkdir (tmpl, S_IRUSR | S_IWUSR | S_IXUSR) >= 0) {
		return tmpl;
	  } else if (errno != EEXIST) {
		return NULL;
	  }
    }
  return NULL;
}

static int os_tmpdir (lua_State *L) {
  char *s; 
  char tempdir[] = "luatex.XXXXXX";
#ifdef HAVE_MKDTEMP
  s = mkdtemp(tempdir);
#else
  s = do_mkdtemp(tempdir);
#endif
  if (s==NULL) {
	int en = errno;  /* calls to Lua API may change this value */
	lua_pushnil(L);
	lua_pushfstring(L, "%s", strerror(en));
	return 2;
  } else {
	lua_pushstring(L,s);
	return 1;
  }
}


void
open_oslibext (lua_State *L, int safer_option) {

  find_env(L);

  lua_getglobal(L,"os");
  lua_pushcfunction(L, ex_sleep);
  lua_setfield(L,-2,"sleep");
#if (! defined (WIN32))  && (! defined (__SUNOS__))
  lua_getglobal(L,"os");
  lua_pushcfunction(L, os_times);
  lua_setfield(L,-2,"times");
#endif
#if ! defined (__SUNOS__)
  lua_getglobal(L,"os");
  lua_pushcfunction(L, os_gettimeofday);
  lua_setfield(L,-2,"gettimeofday");
#endif
  if (!safer_option) {
	lua_getglobal(L,"os");
	lua_pushcfunction(L, os_setenv);
	lua_setfield(L,-2,"setenv");
	lua_getglobal(L,"os");
	lua_pushcfunction(L, os_exec);
	lua_setfield(L,-2,"exec");

	lua_getglobal(L,"os");
	lua_pushcfunction(L, os_tmpdir);
	lua_setfield(L,-2,"tmpdir");

  }


}
