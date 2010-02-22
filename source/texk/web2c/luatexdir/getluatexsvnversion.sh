#! /bin/sh

# This script should be run within the source directory.

$DEBUG

FILE="texk/web2c/luatexdir/luatex_svnversion.h"

LANG=C
if [[ -x `which svnversion` && -d ./.svn ]]
then
  svn up > /dev/null
  svnversion -c . | sed -ne 's/^[0-9]*:*\([0-9]*\).*/#define luatex_svn_revision \1/p' > $FILE
else
   if [[ ! -r $FILE ]]
   then
     echo '#define luatex_svn_revision -1' > $FILE
   fi
fi
