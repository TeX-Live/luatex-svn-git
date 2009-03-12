# Public macros for the teTeX / TeX live (TL) tree.
# Copyright (C) 2009 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 0

# KPSE_MD5_FLAGS()
# ----------------
# Set the make variables MD5_INCLUDES and MD5_LIBS to the CPPFLAGS and
# LIBS required for the `-lmd5' library in libs/md5/ of the TL tree.
AC_DEFUN([KPSE_MD5_FLAGS],
[_KPSE_LIB_FLAGS([md5], [md5], [],
                 [-ISRC/libs/md5], [BLD/libs/md5/libmd5.a], [tree],
                 [${top_srcdir}/../../libs/md5/md5.[ch]])[]dnl
]) # KPSE_MD5_FLAGS
