# $Id: kpse-icu-flags.m4 49495 2018-12-24 23:17:30Z karl $
# ICU support in the TeX Live (TL) tree.
# Copyright 2017-2018 Karl Berry <tex-live@tug.org>
# Copyright 2009-2015 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_ICU_FLAGS([MORE-ICU-LIBS], [ICU-CONFIG-ARGS])
# --------------------------------------------------
# Provide the configure option '--with-system-icu' (if in the TL tree).
#
# MORE-ICU-LIBS: icu libraries from TL tree in addition to icuuc and icudata.
# ICU-CONFIG-ARGS: icu-config arguments for additional icu libraries. 
#
# Set the make variables ICU_INCLUDES and ICU_LIBS to the CPPFLAGS and
# LIBS required for the icu libraries in libs/icu/ of the TL tree.
AC_DEFUN([KPSE_ICU_FLAGS],
[echo 'tldbg:[$0] called: more-icu-libs=[$1], icu-config-args=[$2].' >&AS_MESSAGE_LOG_FD
m4_pushdef([kpse_icu_config_args], [$2])[]dnl
_KPSE_ICU_FLAGS([icuxxx], [], [$1])[]dnl
m4_popdef([kpse_icu_config_args])[]dnl
]) # KPSE_ICU_FLAGS

# _KPSE_ICU_FLAGS(LIBNAME, OPTIONS, [MORE-ICU-LIBS])
# --------------------------------------------------
# Internal subroutine.
#
# LIBNAME and OPTIONS as for _KPSE_LIB_FLAGS().
# MORE-ICU-LIBS: as above.
m4_define([_KPSE_ICU_FLAGS], [dnl
_KPSE_LIB_FLAGS([icu], [$1], [$2],
                [-DU_STATIC_IMPLEMENTATION -IBLD/libs/icu/include],
                m4_bpatsubst([$3 icuuc icudata],
                             [icu\([18a-z]*\)],
                             [BLD/libs/icu/icu-build/lib/libicu\1.a]),
                [],
                [], [${top_builddir}/../../libs/icu/include/unicode/uversion.h])[]dnl
# checking for openbsd to add -lpthread for icu.
case $build_os in
openbsd*)
  eval AS_TR_CPP($1)_LIBS=\"$[]AS_TR_CPP($1)_LIBS -lpthread\"
  ;;
esac
]) # _KPSE_ICU_FLAGS

# KPSE_ICU_OPTIONS([WITH-SYSTEM])
# -------------------------------
AC_DEFUN([KPSE_ICU_OPTIONS], [dnl
m4_ifval([$1],
         [AC_ARG_WITH([system-icu],
                      AS_HELP_STRING([--with-system-icu],
                                     [use installed ICU headers and libraries (requires pkg-config or icu-config)]))])[]dnl
]) # KPSE_ICU_OPTIONS

# KPSE_ICU_SYSTEM_FLAGS
# ---------------------
AC_DEFUN([KPSE_ICU_SYSTEM_FLAGS], [dnl
AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
echo 'tldbg:[$0] called.' >&AS_MESSAGE_LOG_FD
AC_CHECK_TOOL([ICU_CONFIG], [icu-config], [false])[]dnl
AC_CHECK_TOOL([PKG_CONFIG], [pkg-config], [false])[]dnl
if $ICU_CONFIG --version >/dev/null 2>&1; then
  ICU_INCLUDES=`$ICU_CONFIG --cppflags`
  ICU_LIBS=`$ICU_CONFIG --ldflags-searchpath m4_ifset([kpse_icu_config_args],
                                                      [kpse_icu_config_args ])--ldflags-libsonly --ldflags-system`
elif $PKG_CONFIG --libs icu-uc icu-io >/dev/null 2>&1; then
  ICU_INCLUDES=`$PKG_CONFIG --cflags icu-uc icu-io`
  ICU_LIBS=`$PKG_CONFIG --libs icu-uc icu-io`
elif test "x$need_icu:$with_system_icu" = xyes:yes; then
  AC_MSG_ERROR([did not find either pkg-config or icu-config; one is required for system icu library support])
fi
]) # KPSE_ICU_SYSTEM_FLAGS
