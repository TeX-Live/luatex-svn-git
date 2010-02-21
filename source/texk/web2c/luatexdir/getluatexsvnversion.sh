#! /bin/sh
$DEBUG
a=$1
shift
if [ "x$a" = "x" ] 
then 
  a=trunk
fi

FILE="source/texk/web2c/luatexdir/luatex_svnversion.h"

LANG=C
if [[ -x `which svnversion` ]]
then
  svn up > /dev/null
  svnversion | sed -ne "s/^\([0-9]*\).*$/\#define luatex_svn_revision \1/p" > $FILE
else
   if [[ ! -r $FILE ]]
   then
     echo '#define luatex_svn_revision -1' > $FILE
   fi
fi
