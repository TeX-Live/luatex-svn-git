/* kpathsea.c: creating and destroying instance structures.

   Copyright 2009 Taco Hoekwater.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#include <kpathsea/config.h>

kpathsea
kpathsea_new P1H(void)
{
  kpathsea ret;
  ret = xcalloc(1,sizeof(kpathsea_instance));
  return ret;
}

void
kpathsea_finish P1C(kpathsea, kpse)
{
  /* to be filled in */
  if (kpse!=NULL) free (kpse);
  return ;
}

