
#ifndef UTIL_MEM_POOL_H
#define UTIL_MEM_POOL_H

#include "utilmemallh.h"

typedef struct pile8 pile8;
typedef struct pile16 pile16;
typedef struct pile32 pile32;
typedef struct pile64 pile64;

struct pile8 {
  pile8 *prev, *next;
  pile8 *prevsquot, *nextsquot;
  union { uint8_t *data; ghost8 *dataghost; };
  uint16_t chunks;
  uint16_t left;
  uint8_t freeoffset;
#ifdef BIT32
  uint8_t dummy[3]; // 25->28
#else
  uint8_t dummy[3]; // 45->48
#endif
};

struct pile16 {
  pile16 *prev, *next;
  pile16 *prevsquot, *nextsquot;
  union { uint8_t *data; ghost16 *dataghost; };
  uint16_t freeoffset;
  uint16_t chunks;
  uint16_t left;
#ifdef BIT32
  uint8_t dummy[2]; // 26->28
#else
  uint8_t dummy[2]; // 46->48
#endif

};

struct pile32 {
  pile32 *prev, *next;
  pile32 *prevsquot, *nextsquot;
  union { uint8_t *data; ghost32 *dataghost; };
#ifdef BIT32
  ghost32 *freeghost;
#else
  uint32_t freeoffset;
#endif
  uint16_t chunks;
  uint16_t left;
#ifdef BIT32
  //uint8_t dummy[0]; // 28->28
#else
  //uint8_t dummy[0]; // 48->48
#endif

};

struct pile64 {
  pile64 *prev, *next;
  pile64 *prevsquot, *nextsquot;
  union { uint8_t *data; ghost64 *dataghost; };
  ghost64 *freeghost;
  uint16_t chunks;
  uint16_t left;
#ifdef BIT32
  //uint8_t dummy[0]; // 28->28
#else
  uint8_t dummy[4]; // 52->56
#endif
};

typedef struct pool8 pool8;
typedef struct pool16 pool16;
typedef struct pool32 pool32;
typedef struct pool64 pool64;

struct pool8 {
  pile8 *head, *tail, *squot;
  size_t unit;
  uint16_t units;
  uint8_t flags;
};

struct pool16 {
  pile16 *head, *tail, *squot;
  size_t unit;
  uint16_t units;
  uint8_t flags;
};

struct pool32 {
  pile32 *head, *tail, *squot;
  size_t unit;
  uint16_t units;
  uint8_t flags;
};

struct pool64 {
  pile64 *head, *tail, *squot;
  size_t unit;
  uint16_t units;
  uint8_t flags;
};

#define POOL_ZERO (1<<0)
#define POOL_KEEP (1<<1)

UTILAPI pool8 * pool8_init (pool8 *pool, uint16_t units, size_t unit, uint8_t flags);
UTILAPI pool16 * pool16_init (pool16 *pool, uint16_t units, size_t unit, uint8_t flags);
UTILAPI pool32 * pool32_init (pool32 *pool, uint16_t units, size_t unit, uint8_t flags);
UTILAPI pool64 * pool64_init (pool64 *pool, uint16_t units, size_t unit, uint8_t flags);

UTILAPI void pool8_head (pool8 *pool);
UTILAPI void pool16_head (pool16 *pool);
UTILAPI void pool32_head (pool32 *pool);
UTILAPI void pool64_head (pool64 *pool);

#define pool8_ensure_head(pool) ((void)((pool)->head != NULL || (pool8_head(pool), 0)))
#define pool16_ensure_head(pool) ((void)((pool)->head != NULL || (pool16_head(pool), 0)))
#define pool32_ensure_head(pool) ((void)((pool)->head != NULL || (pool32_head(pool), 0)))
#define pool64_ensure_head(pool) ((void)((pool)->head != NULL || (pool64_head(pool), 0)))

UTILAPI void pool8_free (pool8 *pool);
UTILAPI void pool16_free (pool16 *pool);
UTILAPI void pool32_free (pool32 *pool);
UTILAPI void pool64_free (pool64 *pool);

UTILAPI void pool8_clear (pool8 *pool);
UTILAPI void pool16_clear (pool16 *pool);
UTILAPI void pool32_clear (pool32 *pool);
UTILAPI void pool64_clear (pool64 *pool);

UTILAPI void * _pool8_take (pool8 *pool);
UTILAPI void * _pool16_take (pool16 *pool);
UTILAPI void * _pool32_take (pool32 *pool);
UTILAPI void * _pool64_take (pool64 *pool);

#define pool8_take(pool) (pool8_ensure_head(pool), _pool8_take(pool))
#define pool16_take(pool) (pool16_ensure_head(pool), _pool16_take(pool))
#define pool32_take(pool) (pool32_ensure_head(pool), _pool32_take(pool))
#define pool64_take(pool) (pool64_ensure_head(pool), _pool64_take(pool))

UTILAPI void pool8_back (pool8 *pool, void *data);
UTILAPI void pool16_back (pool16 *pool, void *data);
UTILAPI void pool32_back (pool32 *pool, void *data);
UTILAPI void pool64_back (pool64 *pool, void *data);

UTILAPI void pool8_pop (pool8 *pool, void *data);
UTILAPI void pool16_pop (pool16 *pool, void *data);
UTILAPI void pool32_pop (pool32 *pool, void *data);
UTILAPI void pool64_pop (pool64 *pool, void *data);

UTILAPI void pool8_stats (pool8 *pool, mem_info *info, int append);
UTILAPI void pool16_stats (pool16 *pool, mem_info *info, int append);
UTILAPI void pool32_stats (pool32 *pool, mem_info *info, int append);
UTILAPI void pool64_stats (pool64 *pool, mem_info *info, int append);

typedef struct {
  union { pile8 *p8; pile16 *p16; pile32 *p32; pile64 *p64; };
  union { ghost8 *g8; ghost16 *g16; ghost32 *g32; ghost64 *g64; uint8_t *data; };
  uint16_t units;
} pool_iter;

UTILAPI void * pool8_first (pool8 *pool, pool_iter *iter);
UTILAPI void * pool16_first (pool16 *pool, pool_iter *iter);
UTILAPI void * pool32_first (pool32 *pool, pool_iter *iter);
UTILAPI void * pool64_first (pool64 *pool, pool_iter *iter);

UTILAPI void * pool8_last (pool8 *pool, pool_iter *iter);
UTILAPI void * pool16_last (pool16 *pool, pool_iter *iter);
UTILAPI void * pool32_last (pool32 *pool, pool_iter *iter);
UTILAPI void * pool64_last (pool64 *pool, pool_iter *iter);

UTILAPI void * pool8_next (pool8 *pool, pool_iter *iter);
UTILAPI void * pool16_next (pool16 *pool, pool_iter *iter);
UTILAPI void * pool32_next (pool32 *pool, pool_iter *iter);
UTILAPI void * pool64_next (pool64 *pool, pool_iter *iter);

UTILAPI void * pool8_prev (pool8 *pool, pool_iter *iter);
UTILAPI void * pool16_prev (pool16 *pool, pool_iter *iter);
UTILAPI void * pool32_prev (pool32 *pool, pool_iter *iter);
UTILAPI void * pool64_prev (pool64 *pool, pool_iter *iter);

#endif