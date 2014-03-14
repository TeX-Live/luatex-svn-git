diff -u LuaJIT-2.0.3.orig/src/lua.h LuaJIT-2.0.3/src/lua.h
--- LuaJIT-2.0.3.orig/src/lua.h	2014-03-13 12:38:39.407900526 +0100
+++ LuaJIT-2.0.3/src/lua.h	2014-03-13 13:06:16.127969211 +0100
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
