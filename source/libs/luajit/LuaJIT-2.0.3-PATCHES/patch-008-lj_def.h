diff -u LuaJIT-2.0.3.orig/src/lj_def.h LuaJIT-2.0.3/src/lj_def.h
--- LuaJIT-2.0.3.orig/src/lj_def.h	2014-03-13 12:38:39.411900526 +0100
+++ LuaJIT-2.0.3/src/lj_def.h	2014-03-13 13:02:19.419959396 +0100
@@ -62,7 +62,7 @@
 #define LJ_MAX_BCINS	(1<<26)		/* Max. # of bytecode instructions. */
 #define LJ_MAX_SLOTS	250		/* Max. # of slots in a Lua func. */
 #define LJ_MAX_LOCVAR	200		/* Max. # of local variables. */
-#define LJ_MAX_UPVAL	60		/* Max. # of upvalues. */
+#define LJ_MAX_UPVAL	249		/* Max. # of upvalues. */
 
 #define LJ_MAX_IDXCHAIN	100		/* __index/__newindex chain limit. */
 #define LJ_STACK_EXTRA	5		/* Extra stack space (metamethods). */
