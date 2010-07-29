#!/usr/bin/env bash
#
# Public Domain
#
# new script to build luatex binaries
# ----------
# Options:
#      --make      : only make, no make distclean; configure
#      --parallel  : make -j 2 -l 3.0
#      --nostrip   : do not strip binary
#      --warnings= : enable compiler warnings
#      --mingw     : crosscompile for mingw32 from i-386linux
#      --host=     : target system for mingw32 cross-compilation
#      --build=    : build system for mingw32 cross-compilation
#      --arch=     : crosscompile for ARCH on OS X
      
# try to find bash, in case the standard shell is not capable of
# handling the generated configure's += variable assignments
if which bash >/dev/null
then
 CONFIG_SHELL=`which bash`
 export CONFIG_SHELL
fi

# try to find gnu make; we may need it
MAKE=make;
if make -v 2>&1| grep "GNU Make" >/dev/null
then 
  echo "Your make is a GNU-make; I will use that"
elif gmake -v >/dev/null 2>&1
then
  MAKE=gmake;
  echo "You have a GNU-make installed as gmake; I will use that"
else
  echo "I can't find a GNU-make; I'll try to use make and hope that works." 
  echo "If it doesn't, please install GNU-make."
fi

ONLY_MAKE=FALSE
STRIP_LUATEX=TRUE
WARNINGS=yes
MINGWCROSS=FALSE
CONFHOST=
CONFBUILD=
MACCROSS=FALSE
JOBS_IF_PARALLEL=2
MAX_LOAD_IF_PARALLEL=3.0

CFLAGS="$CFLAGS -Wdeclaration-after-statement"

until [ -z "$1" ]; do
  case "$1" in
    --make      ) ONLY_MAKE=TRUE     ;;
    --nostrip   ) STRIP_LUATEX=FALSE ;;
    --warnings=*) WARNINGS=`echo $1 | sed 's/--warnings=\(.*\)/\1/' `        ;;
    --mingw     ) MINGWCROSS=TRUE    ;;
    --host=*    ) CONFHOST="$1"      ;;
    --build=*   ) CONFBUILD="$1"     ;;
    --parallel  ) MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;;
    --arch=*    ) MACCROSS=TRUE; ARCH=`echo $1 | sed 's/--arch=\(.*\)/\1/' ` ;;
    *           ) echo "ERROR: invalid build.sh parameter: $1"; exit 1       ;;
  esac
  shift
done

#
STRIP=strip
LUATEXEXE=luatex

case `uname` in
  MINGW32*    ) LUATEXEXE=luatex.exe ;;
esac

WARNINGFLAGS=--enable-compiler-warnings=$WARNINGS

B=build

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  LUATEXEXE=luatex.exe
  OLDPATH=$PATH
  PATH=/usr/mingw32/bin:$PATH
  CFLAGS="-mtune=pentiumpro -msse2 -g -O2 $CFLAGS"
  CXXFLAGS="-mtune=pentiumpro -msse2 -g -O2 $CXXFLAGS"
  : ${CONFHOST:=--host=mingw32}
  : ${CONFBUILD:=--build=i686-linux-gnu}
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="-Wl,--large-address-aware $CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS
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


# ----------
# clean up, if needed
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
#
# get a new svn version header
( cd source  ; ./texk/web2c/luatexdir/getluatexsvnversion.sh )

cd "$B"

if [ "$ONLY_MAKE" = "FALSE" ]
then
TL_MAKE=$MAKE ../source/configure  $CONFHOST $CONFBUILD  $WARNINGFLAGS\
    --enable-cxx-runtime-hack \
    --disable-all-pkgs \
    --disable-shared    \
    --disable-largefile \
    --disable-ptex \
    --disable-ipc \
    --enable-dump-share  \
    --enable-mp  \
    --enable-luatex  \
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

(cd libs/zziplib; $MAKE all )
(cd libs/zlib; $MAKE all )
(cd libs/obsdcompat; $MAKE all )
(cd libs/libpng; $MAKE all )
(cd libs/xpdf; $MAKE all )

(cd texk/kpathsea; $MAKE )
(cd texk/web2c; $MAKE $LUATEXEXE )

# go back
cd ..

if [ "$STRIP_LUATEX" = "TRUE" ] ;
then
  $STRIP "$B"/texk/web2c/$LUATEXEXE
else
  echo "luatex binary not stripped"
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  PATH=$OLDPATH
fi

# show the results
ls -l "$B"/texk/web2c/$LUATEXEXE
