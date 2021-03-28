# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2012-2015 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_GRAPHITE2_FLAGS
# --------------------
# Provide the configure options '--with-system-graphite2' (if in the TL tree).
#
# Set the make variables GRAPHITE2_INCLUDES and GRAPHITE2_LIBS to the CPPFLAGS and
# LIBS required for the `-lgraphite2' library in libs/graphite2/ of the TL tree.
AC_DEFUN([KPSE_GRAPHITE2_FLAGS], [dnl
_KPSE_LIB_FLAGS([graphite2], [graphite2], [],
                [-IBLD/libs/graphite2/include -DGRAPHITE2_STATIC],
                [BLD/libs/graphite2/libgraphite2.a], [],
                [], [${top_builddir}/../../libs/graphite2/include/graphite2/Font.h])[]dnl
]) # KPSE_GRAPHITE2_FLAGS

# KPSE_GRAPHITE2_OPTIONS([WITH-SYSTEM])
# -------------------------------------
AC_DEFUN([KPSE_GRAPHITE2_OPTIONS], [_KPSE_LIB_OPTIONS([graphite2], [$1], [pkg-config])])

# KPSE_GRAPHITE2_SYSTEM_FLAGS
# ---------------------------
AC_DEFUN([KPSE_GRAPHITE2_SYSTEM_FLAGS], [dnl
_KPSE_PKG_CONFIG_FLAGS([graphite2], [graphite2])])
