/* c-dir.h: directory headers.

   Copyright 1992, 1993, 1994, 2008 Karl Berry.
   Copyright 1998, 2005 Olaf Weber.

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

#ifndef KPATHSEA_C_DIR_H
#define KPATHSEA_C_DIR_H

#ifdef WIN32

#include <direct.h>

#else /* not WIN32 */

/* Use struct dirent instead of struct direct.  */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen ((dirent)->d_name)
#else /* not DIRENT */
#define dirent direct
#define NAMLEN(dirent) ((dirent)->d_namlen)

#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif

#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif

#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif

#endif /* not DIRENT */

#endif /* not WIN32 */

#endif /* not KPATHSEA_C_DIR_H */
