--- ../luajit-2.0.2/src/lauxlib.h.orig	2014-01-10 02:46:45.561858000 +0100
+++ ../luajit-2.0.2/src/lauxlib.h	2014-01-10 02:46:45.561858000 +0100
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
