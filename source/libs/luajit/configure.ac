dnl Process this file with autoconf to produce a configure script.
dnl
dnl   Copyright (C) 2012 Peter Breitenlohner <tex-live@tug.org>
dnl
dnl   This file is free software; the copyright holder
dnl   gives unlimited permission to copy and/or distribute it,
dnl   with or without modifications, as long as this notice is preserved.
dnl
m4_include([version.ac])[] dnl define luajit_version
AC_INIT([luajit for TeX Live], luajit_version, [luigi.scarso@gmail.com])
AC_PREREQ([2.65])
AC_CONFIG_SRCDIR([luajit-]luajit_version[/src/luajit.h])
AC_CONFIG_AUX_DIR([../../build-aux])
AC_CONFIG_MACRO_DIR([../../m4])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AC_PROG_CC

LT_INIT


AM_CONDITIONAL([build], [test "x$enable_build" != xno])
AM_CONDITIONAL([crosscompile_i586_mingw32msvc_gcc],[test "x$host_os" = xmingw32msvc])
AM_CONDITIONAL([crosscompile_x86_64_w64_mingw32_gcc],[test "x$host_os" = xmingw32])
AM_CONDITIONAL([clang_compiler],[test "x$CC" = xclang])
AC_SEARCH_LIBS([sqrt], [m])

if test "x$enable_build" != xno || test -f config.force; then


echo timestamp >config.force
fi

if test "x$host" = x; then
  config_args=
else
  config_args="--host=$host"
  if test "x$build" != x; then
    config_args="$config_args --build=$build"
  fi
fi
AC_SUBST([config_args])

AC_SUBST([LUAJIT_TREE], [luajit-]luajit_version)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
