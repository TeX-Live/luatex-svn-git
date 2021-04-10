/*

texluac.w

Copyright (C) 1994-2007 Lua.org, PUC-Rio.  All rights reserved.
Copyright 2006-2013 Taco Hoekwater <taco@@luatex.org>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This file is part of LuaTeX.

*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define luac_c
#define LUA_CORE

#include "lua.h"
#include "lauxlib.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"

#include "lua/luatex-api.h"

static void PrintFunction(const Proto* f, int full);

#define luaU_print PrintFunction

/*tex A fix for non-gcc compilation: */

#if !defined(__GNUC__) || (__GNUC__ < 2)
# define __attribute__(x)
#endif

/*tex default program name */

#define PROGNAME "texluac"

/*tex default output file */

#define OUTPUT PROGNAME ".out"

/*tex list bytecodes? */

static int listing = 0;

/*tex dump bytecodes? */

static int dumping = 1;

/*tex strip debug information? */

static int stripping = 0;

/*tex default output file name */

static char Output[] = { OUTPUT };

/*tex actual output file name */

static const char *output = Output;

/*tex actual program name */

static const char *progname = PROGNAME;

__attribute__ ((noreturn))
static void fatal(const char *message) {
    fprintf(stderr,"%s: %s\n",progname,message);
    exit(EXIT_FAILURE);
}

__attribute__ ((noreturn))
static void cannot(const char *what) {
    fprintf(stderr,"%s: cannot %s %s: %s\n",progname,what,output,strerror(errno));
    exit(EXIT_FAILURE);
}

__attribute__ ((noreturn))
static void usage(const char* message)
{
    if (*message=='-')
        fprintf(stderr,"%s: unrecognized option " LUA_QS "\n",progname,message);
    else
        fprintf(stderr,"%s: %s\n",progname,message);
    fprintf(stderr,
        "usage: %s [options] [filenames]\n"
        "Available options are:\n"
        "  -l       list (use -l -l for full listing)\n"
        "  -o name  output to file " LUA_QL("name") " (default is \"%s\")\n"
        "  -p       parse only\n"
        "  -s       strip debug information\n"
        "  -v       show version information\n"
        "  --       stop handling options\n"
        "  -        stop handling options and process stdin\n"
        ,progname,Output);
    exit(EXIT_FAILURE);
}

#define IS(s) (strcmp(argv[i],s)==0)

static int doargs(int argc, char* argv[])
{
    int i;
    int version=0;
    if (argv[0]!=NULL && *argv[0]!=0)
        progname=argv[0];
    for (i=1; i<argc; i++) {
        if (*argv[i]!='-') {
            /* end of options; keep it */
            break;
        } else if (IS("--")) {
            /* end of options; skip it */
            ++i;
            if (version) ++version;
                break;
        } else if (IS("-")) {
            /* end of options; use stdin */
            break;
        } else if (IS("-l")) {
            /* list */
            ++listing;
        } else if (IS("-o")) {
            /* output file */
            output=argv[++i];
            if (output==NULL || *output==0 || (*output=='-' && output[1]!=0))
                usage(LUA_QL("-o") " needs argument");
            if (IS("-"))
                output=NULL;
        } else if (IS("-p")) {
            /* parse only */
            dumping=0;
        } else if (IS("-s")) {
            /* strip debug information */
            stripping=1;
        } else if (IS("-v")) {
            /* show version */
            ++version;
        } else {
            /* unknown option */
            usage(argv[i]);
        }
    }
    if (i==argc && (listing || !dumping)) {
        dumping=0;
        argv[--i]=Output;
    }
    if (version) {
        printf("%s\n",LUA_COPYRIGHT);
        if (version==argc-1)
            exit(EXIT_SUCCESS);
    }
    return i;
}

#define FUNCTION "(function()end)();"

static const char* reader(lua_State *L, void *ud, size_t *size)
{
    UNUSED(L);
    if ((*(int*)ud)--) {
        *size=sizeof(FUNCTION)-1;
        return FUNCTION;
    } else {
        *size=0;
        return NULL;
    }
}

#define toproto(L,i) getproto(L->top+(i))

static const Proto* combine(lua_State* L, int n)
{
    if (n==1) {
        return toproto(L,-1);
    } else {
        Proto* f;
        int i=n;
        if (lua_load(L,reader,&i,"=(" PROGNAME ")",NULL)!=LUA_OK)
            fatal(lua_tostring(L,-1));
        f=toproto(L,-1);
        for (i=0; i<n; i++) {
            f->p[i]=toproto(L,i-n-1);
            if (f->p[i]->sizeupvalues>0)
                f->p[i]->upvalues[0].instack=0;
        }
        f->sizelineinfo=0;
        return f;
    }
}

static int writer(lua_State* L, const void* p, size_t size, void* u)
{
    UNUSED(L);
    return (fwrite(p,size,1,(FILE*)u)!=1) && (size!=0);
}

static int pmain(lua_State* L)
{
    int argc=(int)lua_tointeger(L,1);
    char** argv=(char**)lua_touserdata(L,2);
    const Proto* f;
    int i;
    if (!lua_checkstack(L,argc))
        fatal("too many input files");
    /*tex
        Open standard libraries: we need to to this to keep the symbol
        |luaL_openlibs|.
    */
    luaL_checkversion(L);
    /*tex stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    /*tex open libraries */
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, 0);
    for (i=0; i<argc; i++) {
        const char* filename=IS("-") ? NULL : argv[i];
    if (luaL_loadfile(L,filename)!=LUA_OK)
        fatal(lua_tostring(L,-1));
    }
    f=combine(L,argc);
    if (listing)
        luaU_print(f,listing>1);
    if (dumping) {
        FILE* D= (output==NULL) ? stdout : fopen(output,"wb");
        if (D==NULL)
                cannot("open");
        lua_lock(L);
        luaU_dump(L,f,writer,D,stripping);
        lua_unlock(L);
        if (ferror(D))
            cannot("write");
        if (fclose(D))
            cannot("close");
    }
    return 0;
}

int luac_main(int ac, char *av[])
{
    lua_State *L;
    int i = doargs(ac, av);
    ac -= i;
    av += i;
    if (ac <= 0)
        usage("no input files given");
    L=luaL_newstate();
    if (L == NULL)
        fatal("not enough memory for state");
    lua_pushcfunction(L,&pmain);
    lua_pushinteger(L,ac);
    lua_pushlightuserdata(L,av);
    if (lua_pcall(L,2,0,0)!=LUA_OK)
      fatal(lua_tostring(L,-1));
    lua_close(L);
    return EXIT_SUCCESS;
}

/*
    See Copyright Notice in |lua.h|.
*/

#define VOID(p)		((const void*)(p))

#if (defined(LuajitTeX)) || (LUA_VERSION_NUM == 502)
#define TSVALUE(o) rawtsvalue(o)
#endif
#if (LUA_VERSION_NUM == 503)
#define TSVALUE(o) tsvalue(o)
#endif

/* if ts is a const pointer, using Lua's getstr(ts) is not correct */
#define getconststr(ts)  \
  check_exp(sizeof((ts)->extra), cast(const char *, (ts)) + sizeof(UTString))


static void PrintString(const TString* ts)
{
    const char* s=getconststr(ts);
    size_t i,n;
#if (defined(LuajitTeX)) || (LUA_VERSION_NUM == 502)
    n=ts->tsv.len;
#endif
#if (LUA_VERSION_NUM == 503)
    n=(ts->tt == LUA_TSHRSTR ? ts->shrlen : ts->u.lnglen);
#endif
    printf("%c",'"');
    for (i=0; i<n; i++) {
        int c=(int)(unsigned char)s[i];
        switch (c) {
            case '"':
                printf("\\\"");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '\a':
                printf("\\a");
            break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\v':
                printf("\\v");
                break;
            default  :
                if (isprint(c))
                    printf("%c",c);
                else
                    printf("\\%03d",c);
                break;
        }
    }
    printf("%c",'"');
}

static void PrintConstant(const Proto* f, int i)
{
    const TValue* o=&f->k[i];
    switch (ttype(o)) {
        case LUA_TNIL:
            printf("nil");
            break;
        case LUA_TBOOLEAN:
            printf(bvalue(o) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            printf(LUA_NUMBER_FMT,nvalue(o));
            break;
        case LUA_TSTRING:
            PrintString(TSVALUE(o));
            break;
        default:
            /*tex This cannot happen. */
            printf("? type=%d",ttype(o));
            break;
    }
}

#define UPVALNAME(x) ((f->upvalues[x].name) ? getstr(f->upvalues[x].name) : "-")
#define MYK(x) (-1-(x))

static void PrintCode(const Proto* f)
{
    const Instruction* code=f->code;
    int pc,n=f->sizecode;
    for (pc=0; pc<n; pc++) {
        Instruction i=code[pc];
        OpCode o=GET_OPCODE(i);
        int a=GETARG_A(i);
        int b=GETARG_B(i);
        int c=GETARG_C(i);
        int ax=GETARG_Ax(i);
        int bx=GETARG_Bx(i);
        int sbx=GETARG_sBx(i);
        int line=getfuncline(f,pc);
        printf("\t%d\t",pc+1);
        if (line>0)
            printf("[%d]\t",line);
        else
            printf("[-]\t");
        printf("%-9s\t",luaP_opnames[o]);
        switch (getOpMode(o)) {
            case iABC:
                printf("%d",a);
                if (getBMode(o)!=OpArgN) printf(" %d",ISK(b) ? (MYK(INDEXK(b))) : b);
                if (getCMode(o)!=OpArgN) printf(" %d",ISK(c) ? (MYK(INDEXK(c))) : c);
                break;
            case iABx:
                printf("%d",a);
                if (getBMode(o)==OpArgK) printf(" %d",MYK(bx));
                if (getBMode(o)==OpArgU) printf(" %d",bx);
                break;
            case iAsBx:
                printf("%d %d",a,sbx);
                break;
            case iAx:
                printf("%d",MYK(ax));
                break;
        }
        switch (o) {
            case OP_LOADK:
                printf("\t; "); PrintConstant(f,bx);
                break;
            case OP_GETUPVAL:
            case OP_SETUPVAL:
                printf("\t; %s",UPVALNAME(b));
                break;
            case OP_GETTABUP:
                printf("\t; %s",UPVALNAME(b));
                if (ISK(c)) { printf(" "); PrintConstant(f,INDEXK(c)); }
                break;
            case OP_SETTABUP:
                printf("\t; %s",UPVALNAME(a));
                if (ISK(b)) { printf(" "); PrintConstant(f,INDEXK(b)); }
                if (ISK(c)) { printf(" "); PrintConstant(f,INDEXK(c)); }
                break;
            case OP_GETTABLE:
            case OP_SELF:
                if (ISK(c)) { printf("\t; "); PrintConstant(f,INDEXK(c)); }
                break;
            case OP_SETTABLE:
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_POW:
            case OP_EQ:
            case OP_LT:
            case OP_LE:
                if (ISK(b) || ISK(c)) {
                    printf("\t; ");
                    if (ISK(b)) PrintConstant(f,INDEXK(b)); else printf("-");
                    printf(" ");
                    if (ISK(c)) PrintConstant(f,INDEXK(c)); else printf("-");
                }
                break;
            case OP_JMP:
            case OP_FORLOOP:
            case OP_FORPREP:
            case OP_TFORLOOP:
                printf("\t; to %d",sbx+pc+2);
                break;
            case OP_CLOSURE:
                printf("\t; %p",VOID(f->p[bx]));
                break;
            case OP_SETLIST:
                if (c==0) printf("\t; %d",(int)code[++pc]); else printf("\t; %d",c);
                break;
            case OP_EXTRAARG:
                printf("\t; "); PrintConstant(f,ax);
                break;
            default:
                break;
        }
        printf("\n");
    }
}

#define SS(x) ((x==1)?"":"s")
#define S(x)  (int)(x),SS(x)

static void PrintHeader(const Proto* f)
{
    const char* s=f->source ? getstr(f->source) : "=?";
    if (*s=='@' || *s=='=')
        s++;
    else if (*s==LUA_SIGNATURE[0])
        s="(bstring)";
    else
        s="(string)";
    printf("\n%s <%s:%d,%d> (%d instruction%s at %p)\n",
        (f->linedefined==0)?"main":"function",s,
        f->linedefined,f->lastlinedefined,
        S(f->sizecode),VOID(f));
    printf("%d%s param%s, %d slot%s, %d upvalue%s, ",
        (int)(f->numparams),f->is_vararg?"+":"",SS(f->numparams),
        S(f->maxstacksize),S(f->sizeupvalues));
    printf("%d local%s, %d constant%s, %d function%s\n",
        S(f->sizelocvars),S(f->sizek),S(f->sizep));
}

static void PrintDebug(const Proto* f)
{
    int i,n;
    n=f->sizek;
    printf("constants (%d) for %p:\n",n,VOID(f));
    for (i=0; i<n; i++) {
        printf("\t%d\t",i+1);
        PrintConstant(f,i);
        printf("\n");
    }
    n=f->sizelocvars;
    printf("locals (%d) for %p:\n",n,VOID(f));
    for (i=0; i<n; i++) {
        printf("\t%d\t%s\t%d\t%d\n",
        i,getstr(f->locvars[i].varname),f->locvars[i].startpc+1,f->locvars[i].endpc+1);
    }
    n=f->sizeupvalues;
    printf("upvalues (%d) for %p:\n",n,VOID(f));
    for (i=0; i<n; i++) {
        printf("\t%d\t%s\t%d\t%d\n",
        i,UPVALNAME(i),f->upvalues[i].instack,f->upvalues[i].idx);
    }
}

static void PrintFunction(const Proto* f, int full)
{
    int i,n=f->sizep;
    PrintHeader(f);
    PrintCode(f);
    if (full)
        PrintDebug(f);
    for (i=0; i<n; i++)
        PrintFunction(f->p[i],full);
}
