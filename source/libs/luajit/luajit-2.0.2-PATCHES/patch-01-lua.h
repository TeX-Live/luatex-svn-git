--- ../luajit-2.0.2/src/lua.h.orig	2014-01-10 02:46:45.561858000 +0100
+++ ../luajit-2.0.2/src/lua.h	2014-01-10 02:46:45.561858000 +0100
@@ -348,6 +348,16 @@
 		       const char *chunkname, const char *mode);
 
 
+#define LUA_OPEQ 0
+#define LUA_OPLT 1
+#define LUA_OPLE 2
+#define LUA_OK  0
+
+/* see http://comments.gmane.org/gmane.comp.programming.swig/18673 */
+# define lua_rawlen lua_objlen 
+
+
+
 struct lua_Debug {
   int event;
   const char *name;	/* (n) */
