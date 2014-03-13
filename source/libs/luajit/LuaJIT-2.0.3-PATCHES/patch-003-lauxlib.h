diff -u LuaJIT-2.0.3.orig/src/lauxlib.h LuaJIT-2.0.3/src/lauxlib.h
--- LuaJIT-2.0.3.orig/src/lauxlib.h	2014-03-13 12:38:39.411900526 +0100
+++ LuaJIT-2.0.3/src/lauxlib.h	2014-03-13 12:57:59.207948609 +0100
@@ -86,6 +86,32 @@
 				int level);
 
 
+
+/*
+** {======================================================
+** File handles for IO library
+** =======================================================
+*/
+
+/*
+** A file handle is a userdata with metatable 'LUA_FILEHANDLE' and
+** initial structure 'luaL_Stream' (it may contain other fields
+** after that initial structure).
+*/
+
+#define LUA_FILEHANDLE          "FILE*"
+
+
+typedef struct luaL_Stream {
+  FILE *f;  /* stream (NULL for incompletely created streams) */
+  lua_CFunction closef;  /* to close stream (NULL for closed streams) */
+} luaL_Stream;
+
+/* }====================================================== */
+
+
+
+
 /*
 ** ===============================================================
 ** some useful macros
