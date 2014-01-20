# KPSE_LUAJIT_FLAGS
# ----------------
# Set the make variables LUAJIT_INCLUDES and LUAJIT_LIBS to
# the CPPFLAGS and LIBS required for the `-lluajit' library in
# libs/luajit/ of the TL tree.
AC_DEFUN([KPSE_LUAJIT_FLAGS],
[_KPSE_LIB_FLAGS([luajit], [luajit], [lt],
                 [-IBLD/libs/luajit/luajit-build/src], [BLD/libs/luajit/luajit-build/src/libluajit.a], [],
                 [], [${top_builddir}/../../libs/luajit/luajit-build/src/luajit.h])[]dnl
]) # KPSE_LUAJIT_FLAGS



# KPSE_LUAJIT_OPTIONS([WITH-SYSTEM])
# --------------------------------
AC_DEFUN([KPSE_LUAJIT_OPTIONS],
[m4_ifval([$1],
          [AC_ARG_WITH([system-luajit],
                       AS_HELP_STRING([--with-system-luajit],
                                      [use installed luajit headers and library (requires pkg-config)]))])[]dnl
]) # KPSE_LUAJIT_OPTIONS

# KPSE_LUAJIT_SYSTEM_FLAGS
# ----------------------
AC_DEFUN([KPSE_LUAJIT_SYSTEM_FLAGS],
[AC_REQUIRE([_KPSE_CHECK_PKG_CONFIG])[]dnl
if $PKG_CONFIG luajit --atleast-version=2.0; then
  LUAJIT_INCLUDES=`$PKG_CONFIG luajit --cflags`
  LUAJIT_LIBS=`$PKG_CONFIG luajit --libs`
elif test "x$need_luajit:$with_system_luajit" = xyes:yes; then
  AC_MSG_ERROR([did not find luajit-2.0 or better])
fi
]) # KPSE_LUAJIT_SYSTEM_FLAGS


# KPSE_LUAJIT_DEFINES
# ------------------
# Set the make variable LUAJIT_DEFINES to the CPPFLAGS required when
# compiling or using the `-lluajit' library.
AC_DEFUN([KPSE_LUAJIT_DEFINES], [dnl
AC_REQUIRE([KPSE_CHECK_WIN32])[]dnl
AC_SUBST([LUAJIT_DEFINES], [-DLUA_COMPAT_MODULE])
if test "x$kpse_cv_have_win32" = xno; then
  LUAJIT_DEFINES="$LUAJIT_DEFINES -DLUA_USE_POSIX"
  AC_SEARCH_LIBS([dlopen], [dl])
  if test "x$ac_cv_search_dlopen" != xno; then
 AC_CHECK_HEADER([dlfcn.h],
                 [LUA52_DEFINES="$LUAJIT_DEFINES -DLUA_USE_DLOPEN"],
                 [], [AC_INCLUDES_DEFAULT])
  fi
fi
]) # KPSE_LUAJIT_DEFINES
