#!/usr/bin/env bash
# $Id: build.sh 4999 2014-05-06 08:52:33Z taco $
#
# Copyright (c) 2005-2011 Martin Schröder <martin@luatex.org>
# Copyright (c) 2009-2014 Taco Hoekwater <taco@luatex.org>
# Copyright (c) 2012-2014 Luigi Scarso   <luigi@luatex.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# new script to build luatex binaries
# ----------
# Options:
#      --jit       : also build luajittex
#      --nojit     : don't build luajit 
#      --make      : only make, no make distclean; configure
#      --parallel  : make -j 2 -l 3.0
#      --nostrip   : do not strip binary
#      --warnings= : enable compiler warnings
#      --lua53     : build luatex  with luatex 53
#      --nolua53   : don't build luatex  with luatex 53
#      --mingw     : crosscompile for mingw32 from x86_64linux
#      --mingw32   : crosscompile for mingw32 from x86_64linux
#      --mingw64   : crosscompile for mingw64 from x86_64linux
#      --shared    : enable shared build (currently mingw only)
#      --host=     : target system for mingw32 cross-compilation
#      --build=    : build system for mingw32 cross-compilation
#      --arch=     : crosscompile for ARCH on OS X
#      --clang     : use clang & clang++
#      --debug     : CFLAGS="-g -O0" --warnings=max --nostrip
$DEBUG

# try to find bash, in case the standard shell is not capable of
# handling the generated configure's += variable assignments
if which bash >/dev/null
then
 CONFIG_SHELL=`which bash`
 export CONFIG_SHELL
fi

# try to find gnu make; we may need it
MAKE="make V=1";
if make -v 2>&1| grep "GNU Make" >/dev/null
then 
  echo "Your make is a GNU-make; I will use that"
elif gmake -v >/dev/null 2>&1
then
  MAKE=gmake;
  export MAKE;
  echo "You have a GNU-make installed as gmake; I will use that"
else
  echo "I can't find a GNU-make; I'll try to use make and hope that works." 
  echo "If it doesn't, please install GNU-make."
fi

BUILDJIT=FALSE
BUILDLUA53=TRUE
BUILDTAG=
ONLY_MAKE=FALSE
STRIP_LUATEX=TRUE
WARNINGS=yes
MINGW=FALSE
MINGWCROSS=FALSE
MINGWCROSS64=FALSE
MACCROSS=FALSE
CLANG=FALSE
ENABLESHARED=TRUE
CONFHOST=
CONFBUILD=
JOBS_IF_PARALLEL=${JOBS_IF_PARALLEL:-3}
MAX_LOAD_IF_PARALLEL=${MAX_LOAD_IF_PARALLEL:-2}
STRIPBIN=
TARGET_CC=gcc
TARGET_TCFLAGS=

CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS"

until [ -z "$1" ]; do
  case "$1" in
    --jit       ) BUILDJIT=TRUE     ;;
    --nojit     ) BUILDJIT=FALSE     ;;
    --make      ) ONLY_MAKE=TRUE     ;;
    --nostrip   ) STRIP_LUATEX=FALSE ;;
    --debug     ) STRIP_LUATEX=FALSE; WARNINGS=max ; CFLAGS="-g -O0 -ggdb3 $CFLAGS" ; CXXFLAGS="-g -O0 -ggdb3 $CXXFLAGS"  ;;
    --clang     ) export CC=clang; export CXX=clang++ ; TARGET_CC=$CC ; CLANG=TRUE ;;
    --warnings=*) WARNINGS=`echo $1 | sed 's/--warnings=\(.*\)/\1/' `        ;;
    --mingw     ) MINGWCROSS=TRUE    ;;
    --mingw32   ) MINGWCROSS=TRUE    ;;
    --mingw64   ) MINGWCROSS64=TRUE  ;;
    --shared    ) ENABLESHARED=TRUE  ;;
    --host=*    ) CONFHOST="$1"      ;;
    --build=*   ) CONFBUILD="$1"     ;;
    --parallel  ) MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;;
    --stripbin=*) STRIPBIN="$1"      ;;
    --arch=*    ) MACCROSS=TRUE; ARCH=`echo $1 | sed 's/--arch=\(.*\)/\1/' ` ;;
    *           ) echo "ERROR: invalid build.sh parameter: $1"; exit 1       ;;
  esac
  shift
done

#
STRIP=strip
LUATEXEXEJIT=luajittex
LUATEXEXE=luatex
LUATEXEXE53=luatex





case `uname` in
  MINGW64*   ) MINGW=TRUE ; LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex.exe ;;
  MINGW32*   ) MINGW=TRUE ; LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex.exe ;;
  CYGWIN*    ) LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex.exe ;;
esac


WARNINGFLAGS=--enable-compiler-warnings=$WARNINGS

B=build

if [ "$CLANG" = "TRUE" ]
then
  B=build-clang
fi

OLDPATH=$PATH
if [ "$MINGWCROSS64" = "TRUE" ]
then
  B=build-windows64
  LUATEXEXEJIT=luajittex.exe
  LUATEXEXE=luatex.exe
  LUATEXEXE53=luatex.exe
  PATH=/usr/mingw32/bin:$PATH
  PATH=`pwd`/extrabin/mingw/x86_64-linux:$PATH
  CFLAGS="-Wno-unknown-pragmas -mtune=nocona -g -O3 -fno-lto -fno-use-linker-plugin $CFLAGS"
  CXXFLAGS="-Wno-unknown-pragmas -mtune=nocona -g -O3 -fno-lto -fno-use-linker-plugin $CXXFLAGS"
  : ${CONFHOST:=--host=x86_64-w64-mingw32}
  : ${CONFBUILD:=--build=x86_64-unknown-linux-gnu}
  RANLIB="${CONFHOST#--host=}-ranlib"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="${LDFLAGS} -fno-lto -fno-use-linker-plugin -static-libgcc -static-libstdc++"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  LUATEXEXEJIT=luajittex.exe
  LUATEXEXE=luatex.exe
  LUATEXEXE53=luatex.exe
  PATH=/usr/mingw32/bin:$PATH
  PATH=`pwd`/extrabin/mingw/x86_64-linux:$PATH
  CFLAGS="-Wno-unknown-pragmas -m32 -mtune=nocona -g -O3 $CFLAGS"
  CXXFLAGS="-Wno-unknown-pragmas -m32 -mtune=nocona -g -O3 $CXXFLAGS"
  : ${CONFHOST:=--host=i686-w64-mingw32}
  : ${CONFBUILD:=--build=x86_64-unknown-linux-gnu}
  RANLIB="${CONFHOST#--host=}-ranlib"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="-Wl,--large-address-aware -Wl,--stack,2621440 -static-libgcc -static-libstdc++ $CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS BUILDCXX BUILDCC
fi


if [ "$MACCROSS" = "TRUE" ]
then
  # make sure that architecture parameter is valid
  case $ARCH in
    i386 | x86_64 | ppc | ppc64 ) ;;
    * ) echo "ERROR: architecture $ARCH is not supported"; exit 1;;
  esac
  B=build-$ARCH
  CFLAGS="-arch $ARCH -g -O2 $CFLAGS"
  CXXFLAGS="-arch $ARCH -g -O2 $CXXFLAGS"
  LDFLAGS="-arch $ARCH $LDFLAGS" 
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "x$STRIPBIN" != "x" ]
then
 STRIP="${STRIPBIN#--stripbin=}"
fi

if [ "$STRIP_LUATEX" = "FALSE" ]
then
    export CFLAGS
    export CXXFLAGS
fi

# ----------
# clean up, if needed

SHAREDENABLE="--disable-shared "
if [ "$ENABLESHARED" = "TRUE" ]
then
  SHAREDENABLE="--enable-shared -disable-native-texlive-build "
  if [ ! -r "$B-shared" ]
  then
   mkdir "$B-shared"
  fi
  B="$B-shared"
fi


if [ -r "$B"/Makefile -a $ONLY_MAKE = "FALSE" ]
then
  rm -rf "$B"
elif [ ! -r "$B"/Makefile ]
then
    ONLY_MAKE=FALSE
fi
if [ ! -r "$B" ]
then
  mkdir "$B"
fi


JITENABLE="--enable-luajittex=no --enable-mfluajit=no"
if [ "$BUILDJIT" = "TRUE" ]
then
  JITENABLE="--enable-luajittex --without-system-luajit "
fi

BUILDLUA53=TRUE
LUA53ENABLE="--enable-luatex"

cd "$B"


if [ "$ONLY_MAKE" = "FALSE" ]
then
TL_MAKE=$MAKE ../source/configure  $CONFHOST $CONFBUILD  $WARNINGFLAGS\
    --enable-cxx-runtime-hack \
    --enable-silent-rules \
    --disable-all-pkgs \
      $SHAREDENABLE    \
    --disable-largefile \
    --disable-xetex \
    --disable-ptex \
    --disable-ipc \
    --disable-dump-share \
    --enable-web2c  \
     $LUA53ENABLE  $JITENABLE \
    --without-system-ptexenc \
    --without-system-kpathsea \
    --without-system-xpdf \
    --without-system-freetype \
    --without-system-freetype2 \
    --without-system-gd \
    --without-system-libpng \
    --without-system-teckit \
    --without-system-zlib \
    --without-system-t1lib \
    --without-system-icu \
    --without-system-harfbuzz \
    --without-system-graphite \
    --without-system-zziplib \
    --without-mf-x-toolkit --without-x \
   || exit 1 
fi

$MAKE

# the fact that these makes inside libs/ have to be done manually for the cross
# compiler hints that something is wrong in the --enable/--disable switches above,
# but I am too lazy to look up what is wrong exactly.
# (perhaps more files needed to be copied from TL?)

(cd libs; $MAKE all )
(cd libs/zziplib; $MAKE all )
(cd libs/zlib; $MAKE all )
(cd libs/libpng; $MAKE all )
(cd texk; $MAKE web2c/Makefile)
(cd texk/kpathsea; $MAKE )
if [ "$BUILDJIT" = "TRUE" ]
then
  (cd libs/luajit; $MAKE all )
  (cd texk/web2c; $MAKE $LUATEXEXEJIT)
fi


if [ "$BUILDLUA53" = "TRUE" ]
then
  (cd texk/web2c; $MAKE $LUATEXEXE53 )
fi


# go back
cd ..

if [ "$STRIP_LUATEX" = "TRUE" ] 
then
    if [ "$BUILDJIT" = "TRUE" ]
    then
	$STRIP "$B"/texk/web2c/$LUATEXEXEJIT
    fi
    if [ "$BUILDLUA53" = "TRUE" ]
    then
	$STRIP "$B"/texk/web2c/$LUATEXEXE53
    fi
else
  echo "lua(jit)tex binary not stripped"
fi

if [ "$MINGWCROSS" = "TRUE" ] || [ "$MINGWCROSS64" = "TRUE" ] || [ "$MINGW" = "TRUE" ]
then
  PATH=$OLDPATH
  if [ "$ENABLESHARED" = "TRUE" ]
  then
    K=$(find "$B/texk/kpathsea" -name "libkpathsea*dll")
    L1=$(find "$B/libs" -name "texluajit.dll")
    L3=$(find "$B/libs" -name "texlua.dll")
    if [ "$STRIP_LUATEX" = "TRUE" ] 
    then 
      $STRIP  "$K" 
      if [ "$BUILDJIT" = "TRUE" ]
      then
        $STRIP "$B/texk/web2c/.libs/$LUATEXEXEJIT"  "$L1"
      fi
      if [ "$BUILDLUA53" = "TRUE" ]
      then
        $STRIP "$B/texk/web2c/.libs/$LUATEXEXE53"  "$L3"
      fi
    fi
    cp "$K" "$B"
    if [ "$BUILDJIT" = "TRUE" ]
    then
	cp "$B/texk/web2c/.libs/$LUATEXEXEJIT" "$B"
	cp "$L1" "$B"
    fi
    if [ "$BUILDLUA53" = "TRUE" ]
    then
	cp "$B/texk/web2c/.libs/$LUATEXEXE53" "$B"
	cp "$L3" "$B"
    fi

  fi
fi


# show the result
if [ -e "$B/$LUATEXEXEJIT" ]
then
    ls -l "$B/$LUATEXEXEJIT"
fi
if [ -e "$B/$LUATEXEXE" ]
then
    ls -l "$B/$LUATEXEXE"
fi
