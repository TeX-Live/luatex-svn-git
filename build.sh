#!/usr/bin/env bash
#
# Public Domain
#
# new script to build luatex binaries
# ----------
# Options:
#       --make      : only make, no make distclean; configure
#       --parallel  : make -j 2 -l 3.0
#       --nostrip   : do not strip binary
#       --mingw     : crosscompile for mingw32 from i-386linux
      

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
PPCCROSS=FALSE
JOBS_IF_PARALLEL=2
MAX_LOAD_IF_PARALLEL=3.0

while [ "$1" != "" ] ; do
  if [ "$1" = "--make" ] ;
  then ONLY_MAKE=TRUE ;
  elif [ "$1" = "--nostrip" ] ;
  then STRIP_LUATEX=FALSE ;
  elif [ "$1" = "--mingw" ] ;
  then MINGWCROSS=TRUE ;
  elif [ "$1" = "--ppc" ] ;
  then PPCCROSS=TRUE ;
  elif [ "$1" = "--parallel" ] ;
  then MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;
  fi ;
  shift ;
done
#
STRIP=strip

B=build
CONFHOST=

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  OLDPATH=$PATH
  PATH=/usr/mingw32/bin:$PATH
  CONFHOST="--host=mingw32 --build=i686-linux-gnu "
fi

if [ "$PPCCROSS" = "TRUE" ]
then
  B=ppc
  CFLAGS="-arch ppc $CFLAGS"
  XCFLAGS="-arch ppc $XCFLAGS"
  CXXFLAGS="-arch ppc $CXXFLAGS"
  LDFLAGS="-arch ppc $LDFLAGS" 
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
cd "$B"

if [ "$ONLY_MAKE" = "FALSE" ]
then
../source/configure  $CONFHOST \
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
    --disable-mp  \
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
    --without-system-kpathsea \
    --without-system-freetype2 \
    --without-system-gd \
    --without-system-libpng \
    --without-system-teckit \
    --without-system-zlib \
    --without-system-t1lib \
    --disable-shared    \
    --disable-largefile \
    || exit 1 
fi

$MAKE

# go back
cd ..

if [ "$STRIP_LUATEX" = "TRUE" ] ;
then
  $STRIP "$B"/texk/web2c/luatex
else
  echo "luatex binary not stripped"
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  PATH=$OLDPATH
fi

# show the results
ls -l "$B"/texk/web2c/luatex
 
