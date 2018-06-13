#ifndef UTIL_LZWDEF_H
#define UTIL_LZWDEF_H

typedef struct lzw_entry {
  char *data;
  int size;
} lzw_entry;

#define lzw_index short

typedef struct lzw_node lzw_node;

struct lzw_node {
  lzw_index index;
  unsigned char suffix;
  lzw_node *left;
  lzw_node *right;
  lzw_node *map;
};

struct lzw_state {
  union {
    lzw_node *lookup;                 /* encoder table */
    lzw_entry *table;                 /* decoder table */
  };
  lzw_index index;                    /* table index */
  union {
    lzw_node *lastnode;               /* previous encoder table node */
    struct {
      lzw_entry *lastentry;           /* previous decoder table entry */
      int tailbytes;                  /* num of bytes of lastentry not yet written out */
    };
  };
  int basebits;                       /* /UnitLength parameter (8) */
  int codebits;                       /* current code bits */
  int lastbyte;                       /* previosly read byte */
  int tailbits;                       /* lastbyte bits not yet consumed */
  int flush;                          /* encoder */
  int flags;                          /* options */
};

#endif