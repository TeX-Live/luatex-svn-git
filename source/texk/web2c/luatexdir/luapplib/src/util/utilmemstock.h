
#ifndef UTIL_MEM_STOCK_H
#define UTIL_MEM_STOCK_H

#include "utilmemallh.h"

typedef struct ream8 ream8;
typedef struct ream16 ream16;
typedef struct ream32 ream32;
typedef struct ream64 ream64;

struct ream8 {
  ream8 *next, *prev;
  union { uint8_t *data; ghost8 *dataghost; };
  uint8_t left;
  uint8_t chunks, unused;
#ifdef BIT32
  uint8_t dummy[1]; // 15->16
#else
  uint8_t dummy[5]; // 27->32
#endif
};

struct ream16 {
  ream16 *next, *prev;
  union { uint8_t *data; ghost16 *dataghost; };
  uint16_t left;
  uint16_t chunks, unused;
#ifdef BIT32
  uint8_t dummy[2]; // 18->20
#else
  uint8_t dummy[2]; // 30->32
#endif
};

struct ream32 {
  ream32 *next, *prev;
  union { uint8_t *data; ghost32 *dataghost; };
  uint32_t left;
  uint32_t chunks, unused;
#ifdef BIT32
  //uint8_t dummy[0]; // 24->24
#else
  uint8_t dummy[4]; // 36->40
#endif
};

struct ream64 {
  ream64 *next, *prev;
  union { uint8_t *data; ghost64 *dataghost; };
  uint64_t left;
  uint64_t chunks, unused;
#ifdef BIT32
  //uint8_t dummy[4]; // 36->40.. for 32bit we don't need 8-bytes aligned, as we need a dynamic check anyway; see ALIGN64ON32()
#else
  //uint8_t dummy[0]; // 48->48
#endif
};

/* stocks */

typedef struct stock8 stock8;
typedef struct stock16 stock16;
typedef struct stock32 stock32;
typedef struct stock64 stock64;

struct stock8 {
  ream8 *head;
  uint8_t space;
  uint8_t large;
  uint8_t flags;
};

struct stock16 {
  ream16 *head;
  uint16_t space;
  uint16_t large;
  uint8_t flags;
};

struct stock32 {
  ream32 *head;
  uint32_t space;
  uint32_t large;
  uint8_t flags;
};

struct stock64 {
  ream64 *head;
  uint64_t space;
  uint64_t large;
  uint8_t flags;
};

#define STOCK_ZERO (1 << 0)
#define STOCK_KEEP (1 << 1)
#define STOCK_DEFAULTS 0

#define STOCK8_INIT(space, large, flags) { NULL, space, large, flags }
#define STOCK16_INIT(space, large, flags) { NULL, space, large, flags }
#define STOCK32_INIT(space, large, flags) { NULL, space, large, flags }
#define STOCK64_INIT(space, large, flags) { NULL, space, large, flags }

UTILAPI stock8 * stock8_init (stock8 *stock, uint8_t space, uint8_t large, uint8_t flags);
UTILAPI stock16 * stock16_init (stock16 *stock, uint16_t space, uint16_t large, uint8_t flags);
UTILAPI stock32 * stock32_init (stock32 *stock, uint32_t space, uint32_t large, uint8_t flags);
UTILAPI stock64 * stock64_init (stock64 *stock, uint64_t space, uint64_t large, uint8_t flags);

UTILAPI void stock8_head (stock8 *stock);
UTILAPI void stock16_head (stock16 *stock);
UTILAPI void stock32_head (stock32 *stock);
UTILAPI void stock64_head (stock64 *stock);

#define stock8_ensure_head(stock) ((void)((stock)->head != NULL || (stock8_head(stock), 0)))
#define stock16_ensure_head(stock) ((void)((stock)->head != NULL || (stock16_head(stock), 0)))
#define stock32_ensure_head(stock) ((void)((stock)->head != NULL || (stock32_head(stock), 0)))
#define stock64_ensure_head(stock) ((void)((stock)->head != NULL || (stock64_head(stock), 0)))

UTILAPI void stock8_free (stock8 *stock);
UTILAPI void stock16_free (stock16 *stock);
UTILAPI void stock32_free (stock32 *stock);
UTILAPI void stock64_free (stock64 *stock);

UTILAPI void stock8_clear (stock8 *stock);
UTILAPI void stock16_clear (stock16 *stock);
UTILAPI void stock32_clear (stock32 *stock);
UTILAPI void stock64_clear (stock64 *stock);

UTILAPI void * _stock8_take (stock8 *stock, size_t size);
UTILAPI void * _stock16_take (stock16 *stock, size_t size);
UTILAPI void * _stock32_take (stock32 *stock, size_t size);
UTILAPI void * _stock64_take (stock64 *stock, size_t size);

#define stock8_take(stock, size) (stock8_ensure_head(stock), _stock8_take(stock, size))
#define stock16_take(stock, size) (stock16_ensure_head(stock), _stock16_take(stock, size))
#define stock32_take(stock, size) (stock32_ensure_head(stock), _stock32_take(stock, size))
#define stock64_take(stock, size) (stock64_ensure_head(stock), _stock64_take(stock, size))

UTILAPI void stock8_pop (stock8 *stock, void *taken, size_t size);
UTILAPI void stock16_pop (stock16 *stock, void *taken, size_t size);
UTILAPI void stock32_pop (stock32 *stock, void *taken, size_t size);
UTILAPI void stock64_pop (stock64 *stock, void *taken, size_t size);

UTILAPI void stock8_back (stock8 *stock, void *data);
UTILAPI void stock16_back (stock16 *stock, void *data);
UTILAPI void stock32_back (stock32 *stock, void *data);
UTILAPI void stock64_back (stock64 *stock, void *data);

UTILAPI void * _stock8_some (stock8 *stock, size_t size, size_t *pspace);
UTILAPI void * _stock16_some (stock16 *stock, size_t size, size_t *pspace);
UTILAPI void * _stock32_some (stock32 *stock, size_t size, size_t *pspace);
UTILAPI void * _stock64_some (stock64 *stock, size_t size, size_t *pspace);

#define stock8_some(stock, size, pspace) (stock8_ensure_head(stock), _stock8_some(stock, size, pspace))
#define stock16_some(stock, size, pspace) (stock16_ensure_head(stock), _stock16_some(stock, size, pspace))
#define stock32_some(stock, size, pspace) (stock32_ensure_head(stock), _stock32_some(stock, size, pspace))
#define stock64_some(stock, size, pspace) (stock64_ensure_head(stock), _stock64_some(stock, size, pspace))

UTILAPI void * stock8_more (stock8 *stock, void *taken, size_t written, size_t size);
UTILAPI void * stock16_more (stock16 *stock, void *taken, size_t written, size_t size);
UTILAPI void * stock32_more (stock32 *stock, void *taken, size_t written, size_t size);
UTILAPI void * stock64_more (stock64 *stock, void *taken, size_t written, size_t size);

UTILAPI void stock8_done (stock8 *stock, void *taken, size_t written);
UTILAPI void stock16_done (stock16 *stock, void *taken, size_t written);
UTILAPI void stock32_done (stock32 *stock, void *taken, size_t written);
UTILAPI void stock64_done (stock64 *stock, void *taken, size_t written);

UTILAPI void stock8_giveup (stock8 *stock, void *taken);
UTILAPI void stock16_giveup (stock16 *stock, void *taken);
UTILAPI void stock32_giveup (stock32 *stock, void *taken);
UTILAPI void stock64_giveup (stock64 *stock, void *taken);

UTILAPI int stock8_empty (stock8 *stock);
UTILAPI int stock16_empty (stock16 *stock);
UTILAPI int stock32_empty (stock32 *stock);
UTILAPI int stock64_empty (stock64 *stock);

UTILAPI void stock8_stats (stock8 *stock, mem_info *info, int append);
UTILAPI void stock16_stats (stock16 *stock, mem_info *info, int append);
UTILAPI void stock32_stats (stock32 *stock, mem_info *info, int append);
UTILAPI void stock64_stats (stock64 *stock, mem_info *info, int append);

#endif