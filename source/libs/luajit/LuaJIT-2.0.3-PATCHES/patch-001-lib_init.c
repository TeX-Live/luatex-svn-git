diff -u LuaJIT-2.0.3.orig/src/lib_init.c LuaJIT-2.0.3/src/lib_init.c
--- LuaJIT-2.0.3.orig/src/lib_init.c	2014-03-13 12:38:39.407900526 +0100
+++ LuaJIT-2.0.3/src/lib_init.c	2014-03-13 12:59:05.995951378 +0100
@@ -26,6 +26,7 @@
   { LUA_DBLIBNAME,	luaopen_debug },
   { LUA_BITLIBNAME,	luaopen_bit },
   { LUA_JITLIBNAME,	luaopen_jit },
+  { LUA_BITLIBNAME_32,	luaopen_bit32 },
   { NULL,		NULL }
 };
 
