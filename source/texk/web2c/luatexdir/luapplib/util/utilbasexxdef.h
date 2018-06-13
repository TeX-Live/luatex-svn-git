#ifndef UTIL_BASEXXDEF_H
#define UTIL_BASEXXDEF_H

struct basexx_state {
  size_t line, maxline;
  size_t left;
  int tail[5];
  int flush;
};

struct runlength_state {
  int run;
  int flush;
  int c1, c2;
  uint8_t *pos;
};

struct eexec_state {
  int key;
  int flush;
  int binary;
  int c1;
  size_t line, maxline; /* ascii encoder only */
  const char *initbytes;
};

#endif