/* liolibext.c
   
   Copyright 2012 Taco Hoekwater <taco@luatex.org>

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

/* This extension only replaces one function: io.popen() and two
   metatable entries: f:read() and f:lines() but unfortunately
   it has to copy quite a bunch of lines from the original liolib.c
   to do so.
*/

#include "ptexlib.h"
#include "lua/luatex-api.h"

#include "lauxlib.h"
#include "lualib.h"

static const char _svn_version[] =
    "$Id $ $URL $";


/*
** {======================================================
** lua_popen spawns a new process connected to the current
** one through the file streams.
** =======================================================
*/

#if defined(_WIN32)

#define lua_popen(L,c,m)		((void)L, _popen(c,m))
#define lua_pclose(L,file)		((void)L, _pclose(file))

#else

#define lua_popen(L,c,m)	((void)L, fflush(NULL), popen(c,m))
#define lua_pclose(L,file)	((void)L, pclose(file))

#endif

/* }====================================================== */


typedef luaL_Stream LStream;


#define tolstream(L)	((LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE))

#define isclosed(p)	((p)->closef == NULL)

static FILE *tofile (lua_State *L) {
  LStream *p = tolstream(L);
  if (isclosed(p))
    luaL_error(L, "attempt to use a closed file");
  lua_assert(p->f);
  return p->f;
}


/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static LStream *newprefile (lua_State *L) {
  LStream *p = (LStream *)lua_newuserdata(L, sizeof(LStream));
  p->closef = NULL;  /* mark file handle as 'closed' */
  luaL_setmetatable(L, LUA_FILEHANDLE);
  return p;
}


static int aux_close (lua_State *L) {
  LStream *p = tolstream(L);
  lua_CFunction cf = p->closef;
  p->closef = NULL;  /* mark stream as closed */
  return (*cf)(L);  /* close it */
}


/*
** function to close 'popen' files
*/
static int io_pclose (lua_State *L) {
  LStream *p = tolstream(L);
  return luaL_execresult(L, lua_pclose(L, p->f));
}

static int io_popen(lua_State * L)
{
    int ret = 1;
    char *safecmd = NULL;
    char *cmdname = NULL;
    int allow = 0;
    const char *filename = luaL_checkstring(L, 1);
    const char *mode = luaL_optstring(L, 2, "rb");
    LStream *p = newprefile(L);

    if (shellenabledp <= 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "All command execution is disabled");
        return 2;
    }
    /* If restrictedshell == 0, any command is allowed. */
    if (restrictedshell == 0)
        allow = 1;
    else
        allow = shell_cmd_is_allowed(filename, &safecmd, &cmdname);

    if (allow == 1) {
        p->f = lua_popen(L, filename, mode);
	p->closef = &io_pclose;
        ret = (p->f == NULL) ? luaL_fileresult(L, 0, filename) : 1;
    } else if (allow == 2) {
        p->f = lua_popen(L, safecmd, mode);
	p->closef = &io_pclose;
        ret = (p->f == NULL) ? luaL_fileresult(L, 0, filename) : 1;
    } else if (allow == 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "Command execution disabled via shell_escape='p'");
        ret = 2;
    } else {
        lua_pushnil(L);
        lua_pushliteral(L, "Bad command line quoting");
        ret = 2;
    }
    return ret;
}


static int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int toclose) {
  int i;
  int n = lua_gettop(L) - 1;  /* number of arguments to read */
  /* ensure that arguments will fit here and into 'io_readline' stack */
  luaL_argcheck(L, n <= LUA_MINSTACK - 3, LUA_MINSTACK - 3, "too many options");
  lua_pushvalue(L, 1);  /* file handle */
  lua_pushinteger(L, n);  /* number of arguments to read */
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  for (i = 1; i <= n; i++) lua_pushvalue(L, i + 1);  /* copy arguments */
  lua_pushcclosure(L, io_readline, 3 + n);
}


static int f_lines (lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 0);
  return 1;
}



/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (lua_State *L, FILE *f) {
  lua_Number d;
  if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else {
   lua_pushnil(L);  /* "result" to be removed */
   return 0;  /* read fails */
  }
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  lua_pushlstring(L, NULL, 0);
  return (c != EOF);
}

/* this version does not care wether the file has 
   line endings using an 'alien' convention */

static int read_line(lua_State * L, FILE * f, int chop)
{
    luaL_Buffer buf;
    int c, d;
    luaL_buffinit(L, &buf);
    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            luaL_pushresult(&buf);      /* close buffer */
            return (lua_rawlen(L, -1) > 0);     /* check whether read something */
        };
        if (c == '\n') {
	    if (!chop)
		luaL_addchar(&buf, c);
            break;
        } else if (c == '\r') {
	    if (!chop)
		luaL_addchar(&buf, c);
            d = fgetc(f);
            if (d != EOF && d != '\n') {
                ungetc(d, f);
	    } else if (!chop) {
		luaL_addchar(&buf, d);
	    }
            break;
        } else {
            luaL_addchar(&buf, c);
        }
    }
    luaL_pushresult(&buf);      /* close buffer */
    return 1;
}

#define MAX_SIZE_T	(~(size_t)0)

static void read_all (lua_State *L, FILE *f) {
  size_t rlen = LUAL_BUFFERSIZE;  /* how much to read in each cycle */
  size_t old, nrlen = 0;  /* for testing file size */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  /* speed up loading of not too large files: */
  old = ftell(f);
  if ((fseek(f, 0, SEEK_END) >= 0) && 
      ((nrlen = ftell(f)) > 0) && nrlen < 1000 * 1000 * 100) {
      rlen = nrlen;
  }
  fseek(f, old, SEEK_SET);
  for (;;) {
    char *p = luaL_prepbuffsize(&b, rlen);
    size_t nr = fread(p, sizeof(char), rlen, f);
    luaL_addsize(&b, nr);
    if (nr < rlen) break;  /* eof? */
    else if (rlen <= (MAX_SIZE_T / 4))  /* avoid buffers too large */
      rlen *= 2;  /* double buffer size at each iteration */
  }
  luaL_pushresult(&b);  /* close buffer */
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t nr;  /* number of chars actually read */
  char *p;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  p = luaL_prepbuffsize(&b, n);  /* prepare buffer to read whole block */
  nr = fread(p, sizeof(char), n, f);  /* try to read 'n' chars */
  luaL_addsize(&b, nr);
  luaL_pushresult(&b);  /* close buffer */
  return (nr > 0);  /* true iff read something */
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f, 1);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f, 1);
            break;
          case 'L':  /* line with end-of-line */
            success = read_line(L, f, 0);
            break;
          case 'a':  /* file */
            read_all(L, f);  /* read entire file */
            success = 1; /* always success */
            break;
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return luaL_fileresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int f_read (lua_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (lua_State *L) {
  LStream *p = (LStream *)lua_touserdata(L, lua_upvalueindex(1));
  int i;
  int n = (int)lua_tointeger(L, lua_upvalueindex(2));
  if (isclosed(p))  /* file is already closed? */
    return luaL_error(L, "file is already closed");
  lua_settop(L , 1);
  for (i = 1; i <= n; i++)  /* push arguments to 'g_read' */
    lua_pushvalue(L, lua_upvalueindex(3 + i));
  n = g_read(L, p->f, 2);  /* 'n' is number of results */
  lua_assert(n > 0);  /* should return at least a nil */
  if (!lua_isnil(L, -n))  /* read at least one value? */
    return n;  /* return them */
  else {  /* first result is nil: EOF or error */
    if (n > 1) {  /* is there error information? */
      /* 2nd result is error message */
      return luaL_error(L, "%s", lua_tostring(L, -n + 1));
    }
    if (lua_toboolean(L, lua_upvalueindex(3))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


int open_iolibext (lua_State *L) {
  lua_getglobal(L,"io");
  lua_pushcfunction(L, io_popen);
  lua_setfield(L, -2, "popen");
  lua_pop(L, 1);  /* pop io table */ 
  lua_getfield(L, LUA_REGISTRYINDEX, LUA_FILEHANDLE);
  lua_pushcfunction(L, f_read);
  lua_setfield(L, -2,"read");
  lua_pushcfunction(L, f_lines);
  lua_setfield(L, -2,"lines");
  lua_pop(L, 1);  /* pop metatable */ 
  return 0;
}

