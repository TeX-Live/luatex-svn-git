
#ifndef UTIL_POOL_H
#define UTIL_POOL_H

#include <stdlib.h>

/* pool structure */

typedef unsigned char pool_byte;
typedef struct pool_block pool_block;

typedef struct pool {
  size_t unitspace, blockspace;
  void * (*malloc)(size_t);
  void (*free)(void *);
  /* internals */
  pool_block *block, *endblock, *squot;
  struct { /* for iterators */
    pool_block *current;
    pool_byte *pointer, *endpointer;
  };
  int flags;
} pool;

#define POOL_INIT_STRUCT(unitspace, blockspace, pmalloc, pfree, flags) \
  unitspace, blockspace, pmalloc, pfree, NULL, NULL, NULL, {NULL, NULL, NULL}, flags

/* pool flags */

#define POOL_DEFAULTS 0
#define POOL_REFCOUNT (1<<0)

/* pool stat */

typedef struct pool_info {
  struct {
    size_t all, squots;
  } blocks;
  struct {
    size_t all, used, freed;
  } units;
  struct {
    size_t all, used;
  } memory;
} pool_info;

pool * pool_init (pool *p, size_t unitspace, size_t blockspace, void * (*pmalloc)(size_t), void (*pfree)(void *));
void pool_free (pool *p);
void * pool_out (pool *p);
void * pool_out0 (pool *p);
void pool_in (void *pointer);

void * pool_ref_out (pool *p);
void * pool_ref_out0 (pool *p);
#define pool_ref_in pool_in
int * pool_ref_count (void *pointer);
int pool_ref (void *pointer);
int pool_unref (void *pointer);

size_t pool_count (pool *p);
void * pool_first (pool *p);
void * pool_next (pool *p);
void * pool_ref_first (pool *p);
void * pool_ref_next (pool *p);

// pool * pool_parent (void *pointer);

void pool_stat (pool *p, pool_info *stat);
void pool_finalize (pool *p, const char *poolname);

#endif

