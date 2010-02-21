#! /bin/sh
a=$1
shift
if [ "x$a" = "x" ] 
then 
  a=trunk
fi

URL="http://foundry.supelec.fr/svn/luatex"
FILE="source/texk/web2c/luatexdir/luatex_svnversion.h"

if [[ -x `which svn` ]]
then
  svn info "$URL/$a/source" | 
    sed -ne "s/^Last Changed Rev:/\#define luatex_svn_revision/p" > $FILE
else
   if [[ ! -r $FILE ]]
   then
     echo '#define luatex_svn_revision -1' > $FILE
   fi
fi
