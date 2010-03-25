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
#      --mingw     : crosscompile for mingw32 from i-386linux
#      --ppc       : crosscompile for ppc
      
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
MINGWCROSS=FALSE
MACCROSS=FALSE
JOBS_IF_PARALLEL=2
MAX_LOAD_IF_PARALLEL=3.0

CFLAGS="$CFLAGS -Wdeclaration-after-statement"

until [ -z "$1" ]; do
  case "$1" in
    --make     ) ONLY_MAKE=TRUE     ;;
    --nostrip  ) STRIP_LUATEX=FALSE ;;
    --mingw    ) MINGWCROSS=TRUE    ;;
    --parallel ) MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;;
    --arch=*   ) MACCROSS=TRUE; ARCH=`echo $1 | sed 's/--arch=\(.*\)/\1/' ` ;;
    *          ) echo "ERROR: invalid build.sh parameter: $1"; exit 1       ;;
  esac
  shift
done

#
STRIP=strip
LUATEXEXE=luatex

case `uname` in
  MINGW32*    ) LUATEXEXE=luatex.exe ;;
esac

B=build
CONFHOST=

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  STRIP=mingw32-strip
  LUATEXEXE=luatex.exe
  OLDPATH=$PATH
  PATH=/usr/mingw32/bin:$PATH
  CFLAGS="-mtune=pentiumpro -msse2 -g -O2 $CFLAGS"
  CXXFLAGS="-mtune=pentiumpro -msse2 -g -O2 $CXXFLAGS"
  CONFHOST="--host=mingw32 --build=i686-linux-gnu "
  export CFLAGS CXXFLAGS
fi

if [ "$MACCROSS" = "TRUE" ]
then
  # make sure that architecture parameter is valid
  case $ARCH in
    i386 | x86_64 | ppc | ppc64 ) ;;
    * ) echo "ERROR: architecture $ARCH is not supported"; exit 1;;
  esac
  B=build-$ARCH
  CFLAGS="-arch $ARCH $CFLAGS"
  XCFLAGS="-arch $ARCH $XCFLAGS"
  CXXFLAGS="-arch $ARCH $CXXFLAGS"
  LDFLAGS="-arch $ARCH $LDFLAGS" 
  export CFLAGS CXXFLAGS LDFLAGS XCFLAGS  
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
TL_MAKE=$MAKE ../source/configure  $CONFHOST \
    --enable-cxx-runtime-hack \
    --disable-afm2pl    \
    --disable-aleph  \
    --disable-bibtex   \
    --disable-bibtex8   \
    --disable-cfftot1 \
    --disable-cjkutils  \
    --disable-detex    \
    --disable-devnag   \
    --disable-dialog   \
    --disable-dtl      \
    --enable-dump-share  \
    --disable-dvi2tty  \
    --disable-dvidvi   \
    --disable-dviljk   \
    --disable-dvipdfm  \
    --disable-dvipdfmx \
    --disable-dvipos  \
    --disable-dvipsk  \
    --disable-gsftopk \
    --disable-lacheck \
    --disable-lcdf-typetools \
    --disable-makeindexk \
    --disable-mf  \
    --disable-mmafm \
    --disable-mmpfb \
    --disable-musixflx \
    --disable-otfinfo \
    --disable-otftotfm  \
    --disable-pdfopen  \
    --disable-pdftex  \
    --disable-ps2eps   \
    --disable-ps2pkm \
    --disable-psutils  \
    --disable-seetexk \
    --disable-t1dotlessj  \
    --disable-t1lint \
    --disable-t1rawafm \
    --disable-t1reencode \
    --disable-t1testpage \
    --disable-t1utils  \
    --disable-tex    \
    --disable-tex4htk \
    --disable-tpic2pdftex  \
    --disable-ttf2pk \
    --disable-ttfdump \
    --disable-ttftotype42 \
    --disable-vlna  \
    --disable-web-progs \
    --disable-xdv2pdf \
    --disable-xdvipdfmx \
    --disable-xetex \
    --without-graphite \
    --without-system-icu \
    --without-system-kpathsea \
    --without-system-freetype2 \
    --without-system-gd \
    --without-system-libpng \
    --without-system-teckit \
    --without-system-zlib \
    --without-system-t1lib \
    --disable-shared    \
    --disable-largefile \
    --disable-ipc \
    --without-mf-x-toolkit --without-x \
   || exit 1 
fi

$MAKE

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
