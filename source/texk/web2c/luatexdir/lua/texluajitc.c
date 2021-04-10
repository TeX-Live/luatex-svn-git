/*

    LuaJIT frontend. Runs commands, scripts, read-eval-print (REPL) etc.
    Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h

    Major portions taken verbatim or adapted from the Lua interpreter.
    Copyright (C) 1994-2008 Lua.org, PUC-Rio. See Copyright Notice in lua.h

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define luajit_c

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luajit.h"

#include "lj_arch.h"

#if LJ_TARGET_POSIX
#include <unistd.h>
#define lua_stdin_is_tty()	isatty(0)
#elif LJ_TARGET_WINDOWS
#include <io.h>
#ifdef __BORLANDC__
#define lua_stdin_is_tty()	isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()	_isatty(_fileno(stdin))
#endif
#else
#define lua_stdin_is_tty()	1
#endif

#if !LJ_TARGET_CONSOLE
#include <signal.h>
#endif

#include "lua/luatex-api.h"

static lua_State *globalL = NULL;
static const char *progname = LUA_PROGNAME;

#if !LJ_TARGET_CONSOLE

static void lstop(lua_State *L, lua_Debug *ar)
{
  /*tex Unused arg. */
  (void)ar;
  lua_sethook(L, NULL, 0, 0);
  /*tex Avoid luaL_error -- a C hook doesn't add an extra frame. */
  luaL_where(L, 0);
  lua_pushfstring(L, "%sinterrupted!", lua_tostring(L, -1));
  lua_error(L);
}

/*tex if another SIGINT happens before lstop, terminate process (default action) */

static void laction(int i)
{
  signal(i, SIG_DFL);
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

#endif

static void print_usage(void)
{
    fprintf(stderr,
    "usage: %s [options]... [script [args]...].\n"
    "Available options are:\n"
    "  -e chunk  Execute string " LUA_QL("chunk") ".\n"
    "  -l name   Require library " LUA_QL("name") ".\n"
    "  -b ...    Save or list bytecode.\n"
    "  -j cmd    Perform LuaJIT control command.\n"
    "  -O[opt]   Control LuaJIT optimizations.\n"
    "  -i        Enter interactive mode after executing " LUA_QL("script") ".\n"
    "  -v        Show version information.\n"
    "  -E        Ignore environment variables.\n"
    "  --        Stop handling options.\n"
    "  -         Execute stdin and stop handling options.\n"
    ,
    progname);
    fflush(stderr);
}

static void l_message(const char *pname, const char *msg)
{
    if (pname) fprintf(stderr, "%s: ", pname);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

static int report(lua_State *L, int status)
{
    if (status && !lua_isnil(L, -1)) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL)
            msg = "(error object is not a string)";
        l_message(progname, msg);
        lua_pop(L, 1);
    }
    return status;
}

static int traceback(lua_State *L)
{
    if (!lua_isstring(L, 1)) {
        /*tex Non-string error object? Try metamethod. */
        if (lua_isnoneornil(L, 1) || !luaL_callmeta(L, 1, "__tostring") || !lua_isstring(L, -1)) {
            /*tex Return non-string error object. */
            return 1;
        }
        /*tex Replace object by result of __tostring metamethod. */
        lua_remove(L, 1);
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

static int docall(lua_State *L, int narg, int clear)
{
    int status;
    /*tex function index */
    int base = lua_gettop(L) - narg;
    /*tex push traceback function */
    lua_pushcfunction(L, traceback);
    /*tex put it under chunk and args */
    lua_insert(L, base);
#if !LJ_TARGET_CONSOLE
    signal(SIGINT, laction);
#endif
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
#if !LJ_TARGET_CONSOLE
    signal(SIGINT, SIG_DFL);
#endif
    /*tex remove traceback function */
    lua_remove(L, base);
    /*tex force a complete garbage collection in case of errors */
    if (status != 0)
        lua_gc(L, LUA_GCCOLLECT, 0);
    return status;
}

static void print_version(void)
{
    fputs(LUAJIT_VERSION " -- " LUAJIT_COPYRIGHT ". " LUAJIT_URL "\n", stdout);
}

static void print_jit_status(lua_State *L)
{
    int n;
    const char *s;
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    /*tex Get jit.* module table. */
    lua_getfield(L, -1, "jit");
    lua_remove(L, -2);
    lua_getfield(L, -1, "status");
    lua_remove(L, -2);
    n = lua_gettop(L);
    lua_call(L, 0, LUA_MULTRET);
    fputs(lua_toboolean(L, n) ? "JIT: ON" : "JIT: OFF", stdout);
    for (n++; (s = lua_tostring(L, n)); n++) {
        putc(' ', stdout);
        fputs(s, stdout);
    }
    putc('\n', stdout);
}

static int getargs(lua_State *L, char **argv, int n)
{
    int narg = 0;
    int i;
    /*tex count total number of arguments */
    while (argv[argc]) argc++;
    /*tex number of arguments to the script */
    narg = argc - (n + 1);
    luaL_checkstack(L, narg + 3, "too many arguments to script");
    for (i = n+1; i < argc; i++) {
        lua_pushstring(L, argv[i]);
    }
    lua_createtable(L, narg, n + 1);
    for (i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - n);
    }
    return narg;
}

static int dofile(lua_State *L, const char *name)
{
    int status = luaL_loadfile(L, name) || docall(L, 0, 1);
    return report(L, status);
}

static int dostring(lua_State *L, const char *s, const char *name)
{
    int status = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
    return report(L, status);
}

static int dolibrary(lua_State *L, const char *name)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    return report(L, docall(L, 1, 1));
}

static void write_prompt(lua_State *L, int firstline)
{
    const char *p;
    lua_getfield(L, LUA_GLOBALSINDEX, firstline ? "_PROMPT" : "_PROMPT2");
    p = lua_tostring(L, -1);
    if (p == NULL) p = firstline ? LUA_PROMPT : LUA_PROMPT2;
    fputs(p, stdout);
    fflush(stdout);
    /*tex remove global */
    lua_pop(L, 1);
}

static int incomplete(lua_State *L, int status)
{
    if (status == LUA_ERRSYNTAX) {
        size_t lmsg;
        const char *msg = lua_tolstring(L, -1, &lmsg);
        const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
        if (strstr(msg, LUA_QL("<eof>")) == tp) {
            lua_pop(L, 1);
            return 1;
        }
    }
    return 0;
}

static int pushline(lua_State *L, int firstline)
{
    char buf[LUA_MAXINPUT];
    write_prompt(L, firstline);
    if (fgets(buf, LUA_MAXINPUT, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n')
            buf[len-1] = '\0';
        if (firstline && buf[0] == '=')
            lua_pushfstring(L, "return %s", buf+1);
        else
            lua_pushstring(L, buf);
        return 1;
    }
    return 0;
}

static int loadline(lua_State *L)
{
    int status;
    lua_settop(L, 0);
    if (!pushline(L, 1)) {
        /*tex no input */
        return -1;
    }
    for (;;) {
        /*tex repeat until gets a complete line */
        status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
        /*tex cannot try to add lines? */
        if (!incomplete(L, status)) 
            break;
        /*tex no more input? */
        if (!pushline(L, 0))
            return -1;
        /*tex add a new line... */
        lua_pushliteral(L, "\n");
        /*tex ...between the two lines */
        lua_insert(L, -2);
        /*tex join them */
        lua_concat(L, 3);
    }
    /*tex remove line */
    lua_remove(L, 1);
    return status;
}

static void dotty(lua_State *L)
{
    int status;
    const char *oldprogname = progname;
    progname = NULL;
    while ((status = loadline(L)) != -1) {
        if (status == 0)
            status = docall(L, 0, 0);
        report(L, status);
        if (status == 0 && lua_gettop(L) > 0) {
            /*tex any result to print? */
            lua_getglobal(L, "print");
            lua_insert(L, 1);
            if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
                l_message(progname,
            lua_pushfstring(L, "error calling " LUA_QL("print") " (%s)",
            lua_tostring(L, -1)));
        }
    }
    /*tex clear stack */
    lua_settop(L, 0);
    fputs("\n", stdout);
    fflush(stdout);
    progname = oldprogname;
}

static int handle_script(lua_State *L, char **argv, int n)
{
    int status;
    const char *fname;
    /*tex collect arguments */
    int narg = getargs(L, argv, n);
    lua_setglobal(L, "arg");
    fname = argv[n];
    if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0) {
        /*tex use stdin */
        fname = NULL;
    }
    status = luaL_loadfile(L, fname);
    lua_insert(L, -(narg+1));
    if (status == 0)
        status = docall(L, narg, 0);
    else
        lua_pop(L, narg);
    return report(L, status);
}

/*tex Load add-on module. */

static int loadjitmodule(lua_State *L)
{
    lua_getglobal(L, "require");
    lua_pushliteral(L, "jit.");
    lua_pushvalue(L, -3);
    lua_concat(L, 2);
    if (lua_pcall(L, 1, 1, 0)) {
        const char *msg = lua_tostring(L, -1);
        if (msg && !strncmp(msg, "module ", 7)) {
          err:
            l_message(progname,
            "unknown luaJIT command or jit.* modules not installed");
            return 1;
        } else {
            return report(L, 1);
        }
    }
    lua_getfield(L, -1, "start");
    if (lua_isnil(L, -1))
        goto err;
    /*tex Drop module table. */
    lua_remove(L, -2);
    return 0;
}

/*tex Run command with options. */

static int runcmdopt(lua_State *L, const char *opt)
{
    int narg = 0;
    if (opt && *opt) {
        /*tex Split arguments. */
        for (;;) {
            const char *p = strchr(opt, ',');
            narg++;
            if (!p)
                break;
            if (p == opt)
                lua_pushnil(L);
            else
                lua_pushlstring(L, opt, (size_t)(p - opt));
            opt = p + 1;
        }
        if (*opt)
            lua_pushstring(L, opt);
        else
            lua_pushnil(L);
    }
    return report(L, lua_pcall(L, narg, 0, 0));
}

/*tex JIT engine control command: try jit library first or load add-on module. */

static int dojitcmd(lua_State *L, const char *cmd)
{
    const char *opt = strchr(cmd, '=');
    lua_pushlstring(L, cmd, opt ? (size_t)(opt - cmd) : strlen(cmd));
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    /*tex Get jit.* module table. */
    lua_getfield(L, -1, "jit");
    lua_remove(L, -2);
    /*tex Lookup library function. */
    lua_pushvalue(L, -2);
    lua_gettable(L, -2);
    if (!lua_isfunction(L, -1)) {
        /*tex Drop non-function and jit.* table, keep module name. */
        lua_pop(L, 2);
        if (loadjitmodule(L))
            return 1;
    } else {
        /*tex Drop jit.* table. */
        lua_remove(L, -2);
    }
    /*tex Drop module name. */
    lua_remove(L, -2);
    return runcmdopt(L, opt ? opt+1 : opt);
}

/*tex Optimization flags. */

static int dojitopt(lua_State *L, const char *opt)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, "jit.opt");
    lua_remove(L, -2);
    lua_getfield(L, -1, "start");
    lua_remove(L, -2);
    return runcmdopt(L, opt);
}

/*tex Save or list bytecode. */

static int dobytecode(lua_State *L, char **argv)
{
    int narg = 0;
    lua_pushliteral(L, "bcsave");
    if (loadjitmodule(L))
        return 1;
    if (argv[0][2]) {
        narg++;
        argv[0][1] = '-';
        lua_pushstring(L, argv[0]+1);
    }
    for (argv++; *argv != NULL; narg++, argv++)
        lua_pushstring(L, *argv);
    return report(L, lua_pcall(L, narg, 0, 0));
}

/*tex Check that argument has no extra characters at the end */

#define notail(x)	{if ((x)[2] != '\0') return -1;}

#define FLAGS_INTERACTIVE  1
#define FLAGS_VERSION      2
#define FLAGS_EXEC         4
#define FLAGS_OPTION       8
#define FLAGS_NOENV       16

static int collectargs(char **argv, int *flags)
{
    int i;
    for (i = 1; argv[i] != NULL; i++) {
        /*tex Not an option? */
        if (argv[i][0] != '-')
            return i;
        /*tex Check option. */
        switch (argv[i][1]) {
            case '-':
                notail(argv[i]);
                return (argv[i+1] != NULL ? i+1 : 0);
            case '\0':
                return i;
            case 'i':
                notail(argv[i]);
                *flags |= FLAGS_INTERACTIVE;
                /*tex fallthrough */
            case 'v':
                notail(argv[i]);
                *flags |= FLAGS_VERSION;
                break;
            case 'e':
                *flags |= FLAGS_EXEC;
            case 'j':
                /*tex LuaJIT extension */
            case 'l':
                *flags |= FLAGS_OPTION;
                if (argv[i][2] == '\0') {
                    i++;
                    if (argv[i] == NULL)
                        return -1;
                }
                break;
            case 'O':
                /*tex LuaJIT extension */
                break;
            case 'b':
                /*tex LuaJIT extension */
                if (*flags) return -1;
                    *flags |= FLAGS_EXEC;
                return 0;
            case 'E':
                *flags |= FLAGS_NOENV;
                break;
            default:
                /*tex invalid option */
                return -1;
        }
    }
    return 0;
}

static int runargs(lua_State *L, char **argv, int n)
{
    int i;
    for (i = 1; i < n; i++) {
        if (argv[i] == NULL)
            continue;
        lua_assert(argv[i][0] == '-');
        switch (argv[i][1]) {
            /*tex Options */
            case 'e': {
                const char *chunk = argv[i] + 2;
                if (*chunk == '\0')
                    chunk = argv[++i];
                lua_assert(chunk != NULL);
                if (dostring(L, chunk, "=(command line)") != 0)
                    return 1;
                break;
            }
            case 'l': {
                const char *filename = argv[i] + 2;
                if (*filename == '\0')
                    filename = argv[++i];
                lua_assert(filename != NULL);
                if (dolibrary(L, filename)) {
                    /*tex stop if file fails */
                    return 1;
                }
                break;
            }
            case 'j': {
                /*tex LuaJIT extension */
                const char *cmd = argv[i] + 2;
                if (*cmd == '\0')
                    cmd = argv[++i];
                lua_assert(cmd != NULL);
                if (dojitcmd(L, cmd))
                    return 1;
                break;
            }
            case 'O':
                /*tex LuaJIT extension */
                if (dojitopt(L, argv[i] + 2))
                    return 1;
                break;
            case 'b':
                /*tex LuaJIT extension */
                return dobytecode(L, argv+i);
                default: break;
            }
    }
    return 0;
}

static int handle_luainit(lua_State *L)
{
#if LJ_TARGET_CONSOLE
    const char *init = NULL;
#else
    const char *init = getenv(LUA_INIT);
#endif
    if (init == NULL) {
        /*tex status OK */
        return 0;
    } else if (init[0] == 64) {
        return dofile(L, init+1);
    } else {
        return dostring(L, init, "=" LUA_INIT);
    }
}

struct Smain {
    char **argv;
    int argc;
    int status;
};

static int pmain(lua_State *L)
{
    struct Smain *s = (struct Smain *)lua_touserdata(L, 1);
    char **argv = s->argv;
    int script;
    int flags = 0;
    globalL = L;
    if (argv[0] && argv[0][0])
        progname = argv[0];
    /*tex linker-enforced version check */
    LUAJIT_VERSION_SYM();
    script = collectargs(argv, &flags);
    if (script < 0) {
        /*tex invalid args? */
        print_usage();
        s->status = 1;
        return 0;
    }
    if ((flags & FLAGS_NOENV)) {
        lua_pushboolean(L, 1);
        lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
    }
    /*tex stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    /*tex open libraries */
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, -1);
    if (!(flags & FLAGS_NOENV)) {
        s->status = handle_luainit(L);
        if (s->status != 0) return 0;
    }
    if ((flags & FLAGS_VERSION)) print_version();
    s->status = runargs(L, argv, (script > 0) ? script : s->argc);
    if (s->status != 0) return 0;
    if (script) {
        s->status = handle_script(L, argv, script);
        if (s->status != 0) return 0;
    }
    if ((flags & FLAGS_INTERACTIVE)) {
        print_jit_status(L);
        dotty(L);
    } else if (script == 0 && !(flags & (FLAGS_EXEC|FLAGS_VERSION))) {
        if (lua_stdin_is_tty()) {
            print_version();
            print_jit_status(L);
            dotty(L);
        } else {
            /*tex executes stdin as a file */
            dofile(L, NULL);
        }
    }
    return 0;
}

int luac_main(int argc, char **argv)
{
    int status;
    struct Smain s;
    lua_State *L = lua_open();  /* create state */
    if (L == NULL) {
        l_message(argv[0], "cannot create state: not enough memory");
        return EXIT_FAILURE;
    }
    s.argc = argc;
    s.argv = argv;
    status = lua_cpcall(L, pmain, &s);
    report(L, status);
    lua_close(L);
    return (status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS;
}

