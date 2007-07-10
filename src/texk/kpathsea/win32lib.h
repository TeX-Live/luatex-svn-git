#ifndef _GNU_W32_H_
#define _GNU_W32_H_

#pragma warning( disable : 4007 4096 4018 4244 )

#include <stdlib.h>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <dirent.h>
#include <direct.h>

#if defined(WIN32)
# if defined(KPSEDLL)
#  define GNUW32DLL KPSEDLL
# else
#  if defined(GNUW32_DLL) || defined(KPSE_DLL)
#   if defined(MAKE_GNUW32_DLL) || defined(MAKE_KPSE_DLL)
#    define GNUW32DLL __declspec( dllexport)
#   else
#    define GNUW32DLL __declspec( dllimport)
#   endif
#  else
#   define GNUW32DLL
#  endif
# endif
#else /* ! WIN32 */
# define GNUW32DLL
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN _MAX_PATH
#endif

#ifndef MAX_PIPES
#define MAX_PIPES 128
#endif

/* On DOS, it's good to allow both \ and / between directories.  */
#ifndef IS_DIR_SEP
#define IS_DIR_SEP(ch) ((ch) == '/' || (ch) == '\\')
#endif
#ifndef IS_DEVICE_SEP
#define IS_DEVICE_SEP(ch) ((ch) == ':')
#endif
#ifndef NAME_BEGINS_WITH_DEVICE
#define NAME_BEGINS_WITH_DEVICE(name) (*(name) && IS_DEVICE_SEP((name)[1]))
#endif
/* On win32, UNC names are authorized */
#ifndef IS_UNC_NAME
#define IS_UNC_NAME(name) (strlen(name)>=3 && IS_DIR_SEP(*name)  \
                            && IS_DIR_SEP(*(name+1)) && isalnum(*(name+2)))
#endif
#ifndef IS_UNC_NAME
#define IS_UNC_NAME(name) \
  (strlen(name)>=3 && IS_DIR_SEP(*name)  \
     && IS_DIR_SEP(*(name+1)) && isalnum(*(name+2)))
#endif

typedef struct volume_info_data {
  struct volume_info_data * next;

  /* time when info was obtained */
  DWORD     timestamp;

  /* actual volume info */
  char *    root_dir;
  DWORD     serialnum;
  DWORD     maxcomp;
  DWORD     flags;
  char *    name;
  char *    type;
} volume_info_data;

#endif
