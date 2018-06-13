
#ifndef UTIL_FLATEDEF_H
#define UTIL_FLATEDEF_H

#include <zlib.h>

struct flate_state {
  z_stream z;
  int flush;
  int status; /* status from inflate/deflate */
  int level; /* encoder compression level -1..9 */
};

#endif