--- ../luajit-2.0.2/src/lualib.h.orig	2014-01-10 02:46:45.561858000 +0100
+++ ../luajit-2.0.2/src/lualib.h	2014-01-10 02:46:45.561858000 +0100
@@ -22,6 +22,8 @@
 #define LUA_JITLIBNAME	"jit"
 #define LUA_FFILIBNAME	"ffi"
 
+#define LUA_BITLIBNAME_32  "bit32"
+
 LUALIB_API int luaopen_base(lua_State *L);
 LUALIB_API int luaopen_math(lua_State *L);
 LUALIB_API int luaopen_string(lua_State *L);
@@ -34,6 +36,8 @@
 LUALIB_API int luaopen_jit(lua_State *L);
 LUALIB_API int luaopen_ffi(lua_State *L);
 
+LUALIB_API int luaopen_bit32(lua_State *L);
+
 LUALIB_API void luaL_openlibs(lua_State *L);
 
 #ifndef lua_assert
