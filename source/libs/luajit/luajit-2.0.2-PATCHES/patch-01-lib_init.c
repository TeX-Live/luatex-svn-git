--- ../luajit-2.0.2/src/lib_init.c.orig	2014-01-10 02:46:45.561858000 +0100
+++ ../luajit-2.0.2/src/lib_init.c	2014-01-10 02:46:45.561858000 +0100
@@ -26,6 +26,7 @@
   { LUA_DBLIBNAME,	luaopen_debug },
   { LUA_BITLIBNAME,	luaopen_bit },
   { LUA_JITLIBNAME,	luaopen_jit },
+  { LUA_BITLIBNAME_32,	luaopen_bit32 },
   { NULL,		NULL }
 };
 
