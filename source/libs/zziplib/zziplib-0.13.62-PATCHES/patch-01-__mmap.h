--- __mmap.h.orig	2014-07-09 15:34:11.605722375 +0200
+++ __mmap.h	2014-07-09 16:11:23.985628493 +0200
@@ -17,11 +17,13 @@
  *          of the Mozilla Public License 1.1
  */
 
+
+
 #ifdef _USE_MMAP
 #if    defined ZZIP_HAVE_SYS_MMAN_H
 #include <sys/mman.h>
 #define USE_POSIX_MMAP 1
-#elif defined ZZIP_HAVE_WINBASE_H || defined WIN32
+#elif (defined ZZIP_HAVE_WINBASE_H || defined WIN32) && !(defined __MINGW32__ || defined __MINGW64__) 
 #include <windows.h>
 #define USE_WIN32_MMAP 1
 #else
